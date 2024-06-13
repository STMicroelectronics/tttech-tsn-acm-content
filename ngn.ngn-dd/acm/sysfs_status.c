// SPDX-License-Identifier: GPL-2.0
/*
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file sysfs_status.c
 * @brief ACM Driver SYSFS - Status Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfsstatusimpl ACM SYSFS Status Implementation
 * @brief Implementation of SYSFS Status Section
 *
 * @{
 */
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/mutex.h>

#include "sysfs.h"
#include "sysfs_status.h"
#include "acm-module.h"
#include "bypass.h"
#include "acmbitops.h"
#include "commreg.h"
#include "redundancy.h"

/**
 * @brief Represents a register partitioned by bit fields
 */
struct partitioned_reg {
	const u32	*bitmasks;   /**< list of bit masks */
	u32		*values;     /**< cache for each bit field */
	int		no_bitmasks; /**< number of bit masks/bit fields */
	struct mutex	lock;        /**< access lock */
};
/**
 * @brief extended device attribute for status attributes
 */
struct acm_status_attribute {
	u32 offset;	/**< access offset */
	u32 bitmask;	/**< access bit mask */
	int index;	/**< bypass module index */
	struct partitioned_reg *partreg; /**< optional: assoc. part. reg. */

	struct visible_device_attribute vattr; /**< visible device attribute */
};

/**
 * @brief convert struct device_attribute pointer to struct acm_status_attribute
 *        pointer
 */
#define dattr_to_sattr(_dattr)	\
	container_of(_dattr, struct acm_status_attribute, vattr.dattr)

/**
 * @def ACM_PART_STATUS_REG
 * @brief Macro for instantiation of the control struct for a partitioned
 *        register
 *
 * This macro instantiates the constants and structures required to read
 * a partition register, which is cleared on read.
 *
 * @param _offset offset name
 * @param _bitmask0 bit mask of the first bit field
 */
#define ACM_PART_STATUS_REG(_offset, _bitmask0, ...)			\
	static const u32 bitmasks_##_offset[] = {			\
		_bitmask0, __VA_ARGS__					\
	};								\
	static u32 cache_##_offset##_M1[ARRAY_SIZE(bitmasks_##_offset)] = {0};\
	static u32 cache_##_offset##_M0[ARRAY_SIZE(bitmasks_##_offset)] = {0};\
	static struct partitioned_reg partreg_##_offset##_M1 = {	\
		.bitmasks = bitmasks_##_offset,				\
		.values = cache_##_offset##_M1,				\
		.no_bitmasks = ARRAY_SIZE(bitmasks_##_offset),		\
		.lock = __MUTEX_INITIALIZER(partreg_##_offset##_M1.lock),\
	};								\
	static struct partitioned_reg partreg_##_offset##_M0 = {	\
		.bitmasks = bitmasks_##_offset,				\
		.values = cache_##_offset##_M0,				\
		.no_bitmasks = ARRAY_SIZE(bitmasks_##_offset),		\
		.lock = __MUTEX_INITIALIZER(partreg_##_offset##_M0.lock),\
	}

/**
 * @brief extended device attribute for status attributes with 64bit values
 *        from two consecutive 32bit registers
 */
struct acm_status64_attribute {
	u32 offset;	/**< access offset */
	u64 bitmask;	/**< access bit mask */
	int index;	/**< bypass module index */

	struct visible_device_attribute vattr; /**< visible device attribute */
};

/**
 * @brief convert struct device_attribute pointer to
 *        struct acm_status64_attribute pointer
 */
#define dattr_to_sattr64(_dattr)	\
	container_of(_dattr, struct acm_status64_attribute, vattr.dattr)

/**
 * @def ACM_PART_STATUS_ATTR_RO
 * @brief Macro for instantiation of extendend status_attributes referring
 *        a partitioned register
 *
 * This macro instantiates two device attributes, one for each bypass
 * module with the respective canonical name extensions _M0 and _M1.
 * The attribute refers to a partition of a register and need the definition
 * via ACM_PART_STATUS_REG.
 *
 * @param _name base name of the attribute
 * @param _offset register offset to get the value from
 * @param _bitmask bitmask of the value
 * @param _min minimum valid interface version
 * @param _max maximum valid interface version
 * @param _dbg flag indicatiing this is a debug only attribute
 */
#define ACM_PART_STATUS_ATTR_RO(_name, _offset, _bitmask, _min, _max, _dbg) \
	static struct acm_status_attribute status_attr_##_name##_M1 = {	\
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 1,						\
		.partreg = &partreg_##_offset##_M1,			\
		.vattr = __VATTR(_name##_M1, 0444,			\
				 acm_status_show, NULL, _min, _max,	\
				 _dbg),					\
	};								\
	static struct acm_status_attribute status_attr_##_name##_M0 = {	\
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 0,						\
		.partreg = &partreg_##_offset##_M0,			\
		.vattr = __VATTR(_name##_M0, 0444,			\
				 acm_status_show, NULL, _min, _max,	\
				 _dbg),					\
	}

/**
 * @def ACM_STATUS_ATTR_RO
 * @brief Macro for instantiation of extendend status_attributes
 *
 * This macro instantiates two device attributes, one for each bypass
 * module with the respective canonical name extensions _M0 and _M1.
 *
 * @param _name base name of the attribute
 * @param _offset register offset to get the value from
 * @param _bitmask bitmask of the value
 * @param _min minimum valid interface version
 * @param _max maximum valid interface version
 * @param _dbg flag indicatiing this is a debug only attribute
 */
#define ACM_STATUS_ATTR_RO(_name, _offset, _bitmask, _min, _max, _dbg)	\
	static struct acm_status_attribute status_attr_##_name##_M1 = {	\
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 1,						\
		.partreg = NULL,					\
		.vattr = __VATTR(_name##_M1, 0444,			\
				 acm_status_show, NULL, _min, _max,	\
				 _dbg),					\
	};								\
	static struct acm_status_attribute status_attr_##_name##_M0 = {	\
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 0,						\
		.partreg = NULL,					\
		.vattr = __VATTR(_name##_M0, 0444,			\
				 acm_status_show, NULL, _min, _max,	\
				 _dbg),					\
	}

/**
 * @def ACM_STATUS64_ATTR_RO
 * @brief Macro for instantiation of extendend status_attributes (64bit)
 *
 * This macro instantiates two device attributes, one for each bypass
 * module with the respective canonical name extensions _M0 and _M1.
 *
 * @param _name base name of the attribute
 * @param _offset register offset to get the value from
 * @param _bitmask bitmask of the value
 * @param _min minimum valid interface version
 * @param _max maximum valid interface version
 * @param _dbg flag indicatiing this is a debug only attribute
 */
#define ACM_STATUS64_ATTR_RO(_name, _offset, _bitmask, _min, _max, _dbg) \
	static struct acm_status64_attribute status_attr_##_name##_M1 = { \
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 1,						\
		.vattr = __VATTR(_name##_M1, 0444, acm_status64_show,	\
				 NULL, _min, _max, _dbg),		\
	};								\
	static struct acm_status64_attribute status_attr_##_name##_M0 = { \
		.offset = (_offset),					\
		.bitmask = _bitmask,					\
		.index = 0,						\
		.vattr = __VATTR(_name##_M0, 0444, acm_status64_show,	\
				 NULL, _min, _max, _dbg),		\
	}

/**
 * @brief Get value from the referenced bit field of a register
 *
 * This implements the read access to (part of) a register via the respective
 * bit fields for clear on read registers, where the bit field values can be
 * accumulated and thus be cached for further read accesses to another
 * bit filed of the same register. The accesses bit field itself is cleared
 * in this read procedure.
 *
 * @param acm ACM instance pointer
 * @param attr Extended status attribute pointer
 */
static u32 get_partreg_val(struct acm *acm, struct acm_status_attribute *attr)
{
	u32 val;
	int maskidx, i;
	struct partitioned_reg *partreg = attr->partreg;

	/* search appropriate index for bit mask */
	for (maskidx = 0; maskidx < partreg->no_bitmasks; ++maskidx)
		if (partreg->bitmasks[maskidx] == attr->bitmask)
			break;

	if (maskidx == partreg->no_bitmasks) {
		dev_err(&acm->dev, "%s: bitmask 0x%x not defined",
				attr->vattr.dattr.attr.name,
				attr->bitmask);
		return 0;
	}

	mutex_lock(&partreg->lock);

	val = bypass_status_area_read(acm->bypass[attr->index],
			attr->offset);

	/* accumulate on respective partition values */
	for (i = 0; i < partreg->no_bitmasks; ++i)
		partreg->values[i] += read_bitmask(&val, partreg->bitmasks[i]);

	/* get value of associated field */
	val = partreg->values[maskidx];
	/* clear on read */
	partreg->values[maskidx] = 0;

	mutex_unlock(&partreg->lock);

	return val;
}

/**
 * @brief Read respective register from ACM status area
 *
 * Generic function that reads the respective register, mask and shifts its
 * value to access the required bit range and formats it into the given buffer.
 *
 * @param dev Standard parameter not used in here
 * @param attr attribute value contained in an acm_status_attribute
 * @param buf Data buffer to write the register value.
 * @return return number of bytes written into the buffer.
 */
static ssize_t acm_status_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	u32 val;
	struct acm *acm;
	struct acm_status_attribute *stat_attr;

	acm = dev_to_acm(dev);
	stat_attr = dattr_to_sattr(attr);

	/* check if register is partitioned in bitfields */
	if (stat_attr->partreg)
		val = get_partreg_val(acm, stat_attr);
	else {
		val = bypass_status_area_read(acm->bypass[stat_attr->index],
				stat_attr->offset);
		val = read_bitmask(&val, stat_attr->bitmask);
	}

	return scnprintf(buf, PAGE_SIZE, "0x%X", val);
}

/**
 * @brief Read two consecutive 32bit register from ACM status area as
 *        64bit value
 *
 * Generic function that reads the respective registers, mask and shifts its
 * value to access the required bit range and formats it into the given buffer.
 *
 * @param dev Standard parameter not used in here
 * @param attr attribute value contained in an acm_status_attribute
 * @param buf Data buffer to write the register value.
 * @return return number of bytes written into the buffer.
 */
static ssize_t acm_status64_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	u32 lo, hi;
	u64 val;
	struct acm *acm;
	struct acm_status64_attribute *stat_attr;
	char fmt[32] = { 0 };
	int nr_nibbles;

	acm = dev_to_acm(dev);
	stat_attr = dattr_to_sattr64(attr);

	lo = bypass_status_area_read(acm->bypass[stat_attr->index],
				     stat_attr->offset);
	hi = bypass_status_area_read(acm->bypass[stat_attr->index],
				     stat_attr->offset + 4);
	val = hi;
	val <<= 32;
	val |= lo;
	val = read_bitmask64(&val, stat_attr->bitmask);

	nr_nibbles = ALIGN(__sw_hweight64(stat_attr->bitmask), 4) / 4;
	snprintf(fmt, sizeof(fmt) - 1, "0X%%0%dllX\n", nr_nibbles);
	return scnprintf(buf, PAGE_SIZE, fmt, val);
}

/**
 * @brief Status attribute device_id show function
 */
static ssize_t device_id_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	u32 id;
	struct acm *acm;

	acm = dev_to_acm(dev);

	id = commreg_read_device_id(acm->commreg);
	return scnprintf(buf, PAGE_SIZE, "0x%X\n", id);
}

/**
 * @brief Status attribute version_id show function
 */
static ssize_t version_id_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 commreg_read_version(acm->commreg));
}

/**
 * @brief Status attribute revision_id show function
 */
static ssize_t revision_id_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 commreg_read_revision(acm->commreg));
}

/**
 * @brief Status attribute extended_status show function
 */
static ssize_t extended_status_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 commreg_read_extended_status(acm->commreg));
}

/**
 * @brief Status attribute testmodule_enable show function
 */
static ssize_t testmodule_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 commreg_read_testmodule_enable(acm->commreg));
}
/**
 * @brief Status attribute cfg_read_back show function
 */
static ssize_t cfg_read_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 commreg_read_cfg_read_back(acm->commreg));
}

/**
 * @brief Status attribute individual_recovery show function
 */
static ssize_t individual_recovery_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 commreg_read_individual_recovery(acm->commreg));
}

/**
 * @brief Status attribute rx_redundancy show function
 */
static ssize_t rx_redundancy_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 commreg_read_rx_redundancy(acm->commreg));
}

/**
 * @brief Status attribute time_freq show function
 */
static ssize_t time_freq_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 commreg_read_time_freq(acm->commreg));
}

/**
 * @brief Status attribute msgbuf_memsize show function
 */
static ssize_t msgbuf_memsize_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 commreg_read_msgbuf_memsize(acm->commreg));
}

/**
 * @brief Status attribute msgbuf_count show function
 */
static ssize_t msgbuf_count_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 commreg_read_msgbuf_count(acm->commreg));
}

/**
 * @brief Status attribute msgbuf_datawidth show function
 */
static ssize_t msgbuf_datawidth_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 commreg_read_msgbuf_datawidth(acm->commreg));
}

/**
 * @brief Status attribute redund_frames_produced_M0 show function
 */
static ssize_t redund_frames_produced_M0_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 redundancy_get_redund_frames_produced(acm->redundancy,
				 0));
}

/**
 * @brief Status attribute redund_frames_produced_M0 show function
 */
static ssize_t redund_frames_produced_M1_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct acm *acm;

	acm = dev_to_acm(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 redundancy_get_redund_frames_produced(acm->redundancy,
				 1));
}

/* debug IP register interface */
/**
 * @brief Status attribute rx_bytes
 */
ACM_STATUS_ATTR_RO(rx_bytes, ACM_BYPASS_STATUS_AREA_RX_BYTES, GENMASK(15, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute rx_frames
 */
ACM_STATUS_ATTR_RO(rx_frames, ACM_BYPASS_STATUS_AREA_RX_FRAMES, GENMASK(15, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute fcs_errors
 */
ACM_STATUS_ATTR_RO(fcs_errors, ACM_BYPASS_STATUS_AREA_FCS_ERRORS,
		GENMASK(7, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute size_errors
 */
ACM_STATUS_ATTR_RO(size_errors, ACM_BYPASS_STATUS_AREA_SIZE_ERRORS,
		GENMASK(7, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute lookup_match_vec
 */
ACM_STATUS_ATTR_RO(lookup_match_vec, ACM_BYPASS_STATUS_AREA_LOOKUP_MATCH_VEC,
		GENMASK(16, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute layer7_match_vec
 */
ACM_STATUS_ATTR_RO(layer7_match_vec, ACM_BYPASS_STATUS_AREA_L7_MATCH_VEC,
		GENMASK(15, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute last_stream_trigger
 */
ACM_STATUS_ATTR_RO(last_stream_trigger,
		ACM_BYPASS_STATUS_AREA_LAST_STREAM_TRIGGER,
		GENMASK(31, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute ingress_win_stat
 */
ACM_STATUS_ATTR_RO(ingress_win_stat,
		ACM_BYPASS_STATUS_AREA_INGRESS_WINDOW_STATUS, GENMASK(15, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute sched_trig_cnt_prev_cyc
 */
ACM_STATUS_ATTR_RO(sched_trig_cnt_prev_cyc,
		ACM_BYPASS_STATUS_AREA_SCHED_TRIG_CNT_PREV, GENMASK(15, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute sched_1st_cond_trig_cnt_prev_cyc
 */
ACM_STATUS_ATTR_RO(sched_1st_cond_trig_cnt_prev_cyc,
		ACM_BYPASS_STATUS_AREA_SCHED_1STCOND_TRIG_CNT, GENMASK(15, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute pending_req_max_num
 */
ACM_STATUS_ATTR_RO(pending_req_max_num,
		ACM_BYPASS_STATUS_AREA_PENDING_REQ_MAXNUM, GENMASK(2, 0),
		ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute scatter_DMA_frames_cnt_curr
 */
ACM_STATUS_ATTR_RO(scatter_DMA_frames_cnt_curr,
		ACM_BYPASS_STATUS_AREA_SCATTER_DMA_FRAMES,
		SCATTER_DMA_FRAMES_CURR, ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute scatter_DMA_bytes_cnt_prev
 */
ACM_STATUS_ATTR_RO(scatter_DMA_bytes_cnt_prev,
		ACM_BYPASS_STATUS_AREA_SCATTER_DMA_BYTES,
		SCATTER_DMA_BYTES_PREV, ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute scatter_DMA_bytes_cnt_curr
 */
ACM_STATUS_ATTR_RO(scatter_DMA_bytes_cnt_curr,
		ACM_BYPASS_STATUS_AREA_SCATTER_DMA_BYTES,
		SCATTER_DMA_BYTES_CURR, ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute tx_bytes_prev
 */
ACM_STATUS_ATTR_RO(tx_bytes_prev, ACM_BYPASS_STATUS_AREA_TX_BYTES,
		TX_BYTES_PREV, ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute tx_bytes_curr
 */
ACM_STATUS_ATTR_RO(tx_bytes_curr, ACM_BYPASS_STATUS_AREA_TX_BYTES,
		TX_BYTES_CURR, ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute tx_frames_cyc_1st_change
 */
ACM_STATUS64_ATTR_RO(tx_frames_cyc_1st_change,
		ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_1STCHANGE_LO,
		GENMASK_ULL(63, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute tx_frames_cyc_last_change
 */
ACM_STATUS64_ATTR_RO(tx_frames_cyc_last_change,
		ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_LASTCHANGE_LO,
		GENMASK_ULL(63, 0), ACM_IF_2_0, ACM_IF_DONT_CARE, true);
/**
 * @brief Status attribute rx_frames_curr
 */
ACM_STATUS_ATTR_RO(rx_frames_curr, ACM_BYPASS_STATUS_AREA_RX_FRAMES_CRITICAL,
		RX_FRAMES_CRITICAL_CURR, ACM_IF_3_0, ACM_IF_DONT_CARE, true);

/**
 * @brief partitioning of FRAME_ERRORS in respective bitfields
 */
ACM_PART_STATUS_REG(ACM_BYPASS_STATUS_AREA_FRAME_ERRORS,
		FRAME_RUNT_FRAMES, FRAME_MII_ERRORS, FRAME_SOF_ERRORS);
/**
 * @brief Status attribute runt_frames
 */
ACM_PART_STATUS_ATTR_RO(runt_frames, ACM_BYPASS_STATUS_AREA_FRAME_ERRORS,
		FRAME_RUNT_FRAMES, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute mii_errors
 */
ACM_PART_STATUS_ATTR_RO(mii_errors, ACM_BYPASS_STATUS_AREA_FRAME_ERRORS,
		FRAME_MII_ERRORS, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute sof_errors
 */
ACM_PART_STATUS_ATTR_RO(sof_errors, ACM_BYPASS_STATUS_AREA_FRAME_ERRORS,
		FRAME_SOF_ERRORS, ACM_IF_3_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute layer7_missmatch_cnt
 */
ACM_STATUS_ATTR_RO(layer7_missmatch_cnt, ACM_BYPASS_STATUS_AREA_L7_MISMATCH_CNT,
		GENMASK(7, 0), ACM_IF_DONT_CARE, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute rx_frames_prev
 */
ACM_STATUS_ATTR_RO(rx_frames_prev, ACM_BYPASS_STATUS_AREA_RX_FRAMES_CRITICAL,
		RX_FRAMES_CRITICAL_PREV, ACM_IF_3_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute rx_frames_cycle_change
 */
ACM_STATUS_ATTR_RO(rx_frames_cycle_change,
		ACM_BYPASS_STATUS_AREA_RX_FRAMES_CRIT_CYC_CHNG, GENMASK(15, 0),
		ACM_IF_3_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute drop_frames_cnt_prev
 */
ACM_STATUS_ATTR_RO(drop_frames_cnt_prev, ACM_BYPASS_STATUS_AREA_DROPPED_FRAMES,
		DROPPED_FRAMES_PREV_CYC, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE,
		false);
/**
 * @brief Status attribute scatter_DMA_frames_cnt_prev
 */
ACM_STATUS_ATTR_RO(scatter_DMA_frames_cnt_prev,
		ACM_BYPASS_STATUS_AREA_SCATTER_DMA_FRAMES,
		SCATTER_DMA_FRAMES_PREV, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE,
		false);
/**
 * @brief Status attribute tx_frames_prev
 */
ACM_STATUS_ATTR_RO(tx_frames_prev, ACM_BYPASS_STATUS_AREA_TX_FRAMES,
		TX_FRAMES_PREV, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute gmii_errors_set_prev
 *
 * Has been substituted by gmii_errors_set_prev
 */
ACM_STATUS_ATTR_RO(gmii_errors_set_prev, ACM_BYPASS_STATUS_AREA_GMII_ERROR,
		GMII_ERROR_PREV, ACM_IF_DONT_CARE, ACM_IF_2_0, false);
/**
 * @brief Status attribute gmii_error_prev_cycle
 *
 * Substitutes gmii_errors_set_prev since ttt,acm-3.0
 */
ACM_STATUS_ATTR_RO(gmii_error_prev_cycle, ACM_BYPASS_STATUS_AREA_GMII_ERROR,
		GMII_ERROR_PREV, ACM_IF_3_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute disable_overrun_prev
 */
ACM_STATUS_ATTR_RO(disable_overrun_prev, ACM_BYPASS_STATUS_AREA_DISABLE_OVERRUN,
		DISABLE_OVERRUN_PREV, ACM_IF_DONT_CARE, ACM_IF_DONT_CARE,
		false);
/**
 * @brief Status attribute tx_frame_cycle_change
 */
ACM_STATUS_ATTR_RO(tx_frame_cycle_change,
		ACM_BYPASS_STATUS_AREA_TX_FRAMES_CYC_CHANGE, GENMASK(15, 0),
		ACM_IF_DONT_CARE, ACM_IF_DONT_CARE, false);

/**
 * @brief Status attribute ifc_version
 */
ACM_STATUS_ATTR_RO(ifc_version, ACM_BYPASS_STATUS_AREA_IFC_VERSION,
		GENMASK(31, 0), ACM_IF_DONT_CARE, ACM_IF_1_0, false);
/**
 * @brief Status attribute config_version
 */
ACM_STATUS_ATTR_RO(config_version, ACM_BYPASS_STATUS_AREA_CONFIG_VERSION,
		GENMASK(31, 0), ACM_IF_DONT_CARE, ACM_IF_1_0, false);

/**
 * @brief Status attribute redund_frame_produced_M0
 */
DEVICE_VATTR_RO(redund_frames_produced_M0, ACM_IF_4_0, ACM_IF_DONT_CARE, false);

/**
 * @brief Status attribute redund_frame_produced_M1
 */
DEVICE_VATTR_RO(redund_frames_produced_M1, ACM_IF_4_0, ACM_IF_DONT_CARE, false);

/**
 * @brief Status attribute device_id
 */
static DEVICE_VATTR_RO(device_id, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute version_id
 */
static DEVICE_VATTR_RO(version_id, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute revision_id
 */
static DEVICE_VATTR_RO(revision_id, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute extended_status
 */
static DEVICE_VATTR_RO(extended_status, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute testmodule_enable
 */
static DEVICE_VATTR_RO(testmodule_enable, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute cfg_read_back
 */
static DEVICE_VATTR_RO(cfg_read_back, ACM_IF_4_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute individual_recovery
 */
static DEVICE_VATTR_RO(individual_recovery, ACM_IF_4_0, ACM_IF_DONT_CARE,
	false);
/**
 * @brief Status attribute rx_redundancy
 */
static DEVICE_VATTR_RO(rx_redundancy, ACM_IF_4_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute time_freq
 */
static DEVICE_VATTR_RO(time_freq, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute msgbuf_memsize
 */
static DEVICE_VATTR_RO(msgbuf_memsize, ACM_IF_2_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute msgbuf_count
 */
static DEVICE_VATTR_RO(msgbuf_count, ACM_IF_4_0, ACM_IF_DONT_CARE, false);
/**
 * @brief Status attribute msgbuf_datawidth
 */
static DEVICE_VATTR_RO(msgbuf_datawidth, ACM_IF_4_0, ACM_IF_DONT_CARE, false);

/**
 * @brief Status attributes for ACM IP
 */
static struct attribute *status_attributes[] = {
	&status_attr_ifc_version_M0.vattr.dattr.attr,
	&status_attr_ifc_version_M1.vattr.dattr.attr,
	&status_attr_config_version_M0.vattr.dattr.attr,
	&status_attr_config_version_M1.vattr.dattr.attr,
	&status_attr_runt_frames_M0.vattr.dattr.attr,
	&status_attr_runt_frames_M1.vattr.dattr.attr,
	&status_attr_mii_errors_M0.vattr.dattr.attr,
	&status_attr_mii_errors_M1.vattr.dattr.attr,
	&status_attr_sof_errors_M0.vattr.dattr.attr,
	&status_attr_sof_errors_M1.vattr.dattr.attr,
	&status_attr_layer7_missmatch_cnt_M0.vattr.dattr.attr,
	&status_attr_layer7_missmatch_cnt_M1.vattr.dattr.attr,
	&status_attr_rx_frames_prev_M0.vattr.dattr.attr,
	&status_attr_rx_frames_prev_M1.vattr.dattr.attr,
	&status_attr_rx_frames_cycle_change_M0.vattr.dattr.attr,
	&status_attr_rx_frames_cycle_change_M1.vattr.dattr.attr,
	&status_attr_drop_frames_cnt_prev_M0.vattr.dattr.attr,
	&status_attr_drop_frames_cnt_prev_M1.vattr.dattr.attr,
	&status_attr_scatter_DMA_frames_cnt_prev_M0.vattr.dattr.attr,
	&status_attr_scatter_DMA_frames_cnt_prev_M1.vattr.dattr.attr,
	&status_attr_tx_frames_prev_M0.vattr.dattr.attr,
	&status_attr_tx_frames_prev_M1.vattr.dattr.attr,
	&status_attr_gmii_errors_set_prev_M0.vattr.dattr.attr,
	&status_attr_gmii_errors_set_prev_M1.vattr.dattr.attr,
	&status_attr_gmii_error_prev_cycle_M0.vattr.dattr.attr,
	&status_attr_gmii_error_prev_cycle_M1.vattr.dattr.attr,
	&status_attr_disable_overrun_prev_M0.vattr.dattr.attr,
	&status_attr_disable_overrun_prev_M1.vattr.dattr.attr,
	&status_attr_tx_frame_cycle_change_M0.vattr.dattr.attr,
	&status_attr_tx_frame_cycle_change_M1.vattr.dattr.attr,
	&vdev_attr_device_id.dattr.attr,
	&vdev_attr_version_id.dattr.attr,
	&vdev_attr_revision_id.dattr.attr,
	&vdev_attr_extended_status.dattr.attr,
	&vdev_attr_testmodule_enable.dattr.attr,
	&vdev_attr_cfg_read_back.dattr.attr,
	&vdev_attr_individual_recovery.dattr.attr,
	&vdev_attr_rx_redundancy.dattr.attr,
	&vdev_attr_time_freq.dattr.attr,
	&vdev_attr_msgbuf_memsize.dattr.attr,
	&vdev_attr_msgbuf_count.dattr.attr,
	&vdev_attr_msgbuf_datawidth.dattr.attr,
	&vdev_attr_redund_frames_produced_M0.dattr.attr,
	&vdev_attr_redund_frames_produced_M1.dattr.attr,
	&status_attr_rx_bytes_M0.vattr.dattr.attr,
	&status_attr_rx_bytes_M1.vattr.dattr.attr,
	&status_attr_rx_frames_M0.vattr.dattr.attr,
	&status_attr_rx_frames_M1.vattr.dattr.attr,
	&status_attr_fcs_errors_M0.vattr.dattr.attr,
	&status_attr_fcs_errors_M1.vattr.dattr.attr,
	&status_attr_size_errors_M0.vattr.dattr.attr,
	&status_attr_size_errors_M1.vattr.dattr.attr,
	&status_attr_lookup_match_vec_M0.vattr.dattr.attr,
	&status_attr_lookup_match_vec_M1.vattr.dattr.attr,
	&status_attr_layer7_match_vec_M0.vattr.dattr.attr,
	&status_attr_layer7_match_vec_M1.vattr.dattr.attr,
	&status_attr_last_stream_trigger_M0.vattr.dattr.attr,
	&status_attr_last_stream_trigger_M1.vattr.dattr.attr,
	&status_attr_ingress_win_stat_M0.vattr.dattr.attr,
	&status_attr_ingress_win_stat_M1.vattr.dattr.attr,
	&status_attr_sched_trig_cnt_prev_cyc_M0.vattr.dattr.attr,
	&status_attr_sched_trig_cnt_prev_cyc_M1.vattr.dattr.attr,
	&status_attr_sched_1st_cond_trig_cnt_prev_cyc_M0.vattr.dattr.attr,
	&status_attr_sched_1st_cond_trig_cnt_prev_cyc_M1.vattr.dattr.attr,
	&status_attr_pending_req_max_num_M0.vattr.dattr.attr,
	&status_attr_pending_req_max_num_M1.vattr.dattr.attr,
	&status_attr_scatter_DMA_frames_cnt_curr_M0.vattr.dattr.attr,
	&status_attr_scatter_DMA_frames_cnt_curr_M1.vattr.dattr.attr,
	&status_attr_scatter_DMA_bytes_cnt_prev_M0.vattr.dattr.attr,
	&status_attr_scatter_DMA_bytes_cnt_prev_M1.vattr.dattr.attr,
	&status_attr_scatter_DMA_bytes_cnt_curr_M0.vattr.dattr.attr,
	&status_attr_scatter_DMA_bytes_cnt_curr_M1.vattr.dattr.attr,
	&status_attr_tx_bytes_prev_M0.vattr.dattr.attr,
	&status_attr_tx_bytes_prev_M1.vattr.dattr.attr,
	&status_attr_tx_bytes_curr_M0.vattr.dattr.attr,
	&status_attr_tx_bytes_curr_M1.vattr.dattr.attr,
	&status_attr_tx_frames_cyc_1st_change_M0.vattr.dattr.attr,
	&status_attr_tx_frames_cyc_1st_change_M1.vattr.dattr.attr,
	&status_attr_tx_frames_cyc_last_change_M0.vattr.dattr.attr,
	&status_attr_tx_frames_cyc_last_change_M1.vattr.dattr.attr,
	&status_attr_rx_frames_curr_M0.vattr.dattr.attr,
	&status_attr_rx_frames_curr_M1.vattr.dattr.attr,
	NULL
};

/**
 * @brief Helper to control visibility of status attributes
 */

static umode_t acm_status_is_visible(struct kobject *kobj,
				     struct attribute *attr, int index)
{
	struct visible_device_attribute *vattr;
	struct acm *acm = kobj_to_acm(kobj);

	vattr = container_of(attr, struct visible_device_attribute, dattr.attr);

	/* restrict debug only attributes */
	if (vattr->debug_only && !commreg_read_extended_status(acm->commreg))
		return 0;

	/* check minimum interface version */
	if ((vattr->min != ACM_IF_DONT_CARE) && (acm->if_id < vattr->min))
		return 0;

	/* check maximum interface version */
	if ((vattr->max != ACM_IF_DONT_CARE) && (acm->if_id > vattr->max))
		return 0;

	return attr->mode;
}

/**
 * @brief Linux attribute group for status section
 */
static const struct attribute_group status_group = {
	.name = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
	.attrs = status_attributes,
	.is_visible = acm_status_is_visible,
};

/**
 * @brief initialize sysfs status section
 *
 * @param acm ACM instance
 */
const struct attribute_group *sysfs_status_init(struct acm *acm)
{
	return &status_group;
}

/**
 * exit sysfs status section
 *
 * @param acm ACM instance
 */
void sysfs_status_exit(struct acm *acm)
{
	/* nothing to do */
}

/**@} acmsysfsstatusimpl */

