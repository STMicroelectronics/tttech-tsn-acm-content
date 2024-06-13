/* SPDX-License-Identifier: GPL-2.0
 *
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file acmdrv.h
 * @brief ACM Driver Userspace API
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACMDRV_H_
#define ACMDRV_H_

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#ifndef NBBY
#define NBBY	8
#endif

#else	/* __KERNEL__ */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <asm/bitsperlong.h>
#include <string.h>
#include <sys/param.h>

/**
 * @brief Helper to provide bit mask per bit number
 * @{
 */
#define BIT(nr)		(1UL << (nr))
#define BIT_ULL(nr)	(1ULL << (nr))
/**@} */

/**
 * @def GENMASK
 * @brief Helper to create  bitmask starting at bit position \p l and ending at
 *        position \p h.
 *
 * @param h high bit position
 * @param l low bit position
 * @{
 */
#define BITS_PER_LONG		(sizeof(long) * 8)
#define BITS_PER_LONG_LONG	(sizeof(long long) * 8)
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))
/**@} */

/**
 * @brief Provide macro for packing structures
 */
#ifndef __packed
#define __packed	__attribute__((packed))
#endif

/**
 * @brief Macro to enforce checking the return values of each a function.
 */
#ifndef __must_check
#define __must_check __attribute__((warn_unused_result))
#endif

#endif	/* __KERNEL__ */

#ifndef howmany
/**
 * @brief Round quotient to the next integer
 */
#define howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif

/**
 * @brief Helper to create correctly shifted value for writing
 */
#define WVAL(_name, _v)	\
	(((_v) << _name##_L) & GENMASK(_name##_H, _name##_L))

/**
 * @brief Helper to create correctly shifted value for reading
 */
#define RVAL(_name, _v)	\
	(((_v) & GENMASK(_name##_H, _name##_L)) >> _name##_L)

/**
 * @brief Helper to create bitmask per macroname
 */
#define BITMASK(_name)	\
	GENMASK(_name##_H, _name##_L)

/**
 * @brief data representation for a single bit field array element
 */
typedef uint32_t acmdrv_bitarray_elem_t;

/**
 * @brief Helper macro to define functions respecting the size of
 *        acmdrv_bitarray_elem_t
 *
 * Using the GCC's builtin versions of the respective functions keeps
 * compiler from linking in GLIBC implementations which, of course,
 * does not work when compiling the kernel module.
 *
 * @param func Function name to be used for generation
 */
#define ACMDRV_BITARRAY_DEF_ELEM_FUNC(func)				\
static inline								\
unsigned int acmdrv_bitarray_elem_ ## func(acmdrv_bitarray_elem_t elem)	\
{									\
	_Static_assert(sizeof(acmdrv_bitarray_elem_t)			\
			== sizeof(unsigned int) ||			\
		       sizeof(acmdrv_bitarray_elem_t)			\
			== sizeof(unsigned long) ||			\
		       sizeof(acmdrv_bitarray_elem_t)			\
			== sizeof(unsigned long long),			\
		      "sizeof(acmdrv_bitarray_elem_t)");		\
									\
	if (sizeof(acmdrv_bitarray_elem_t) == sizeof(unsigned int))	\
		return __builtin_ ## func(elem);			\
	else if (sizeof(acmdrv_bitarray_elem_t) == sizeof(unsigned long)) \
		return __builtin_ ## func ## l(elem);			\
	else								\
		return __builtin_ ## func ## ll(elem);			\
}

ACMDRV_BITARRAY_DEF_ELEM_FUNC(ffs)
ACMDRV_BITARRAY_DEF_ELEM_FUNC(popcount)

/**
 * @brief helper to support bit field arrays
 */
struct acmdrv_bitarray {
	acmdrv_bitarray_elem_t bits[0]; /**< bit field elements */
};

/**
 * @brief number of bit per bit field array element
 */
#define ACMDRV_BITARRAY_NMASKBITS (NBBY * sizeof(acmdrv_bitarray_elem_t))

/**
 * @brief element index containing respective bit
 *
 * @param bno bit number
 * @return bit field array element index
 */
static inline size_t acmdrv_bitarray_idx(unsigned int bno)
{
	return bno / ACMDRV_BITARRAY_NMASKBITS;
}

/**
 * @brief bit field mask within the respective bit field array element
 *
 * @param bno bit number
 * @return value representing the bit field element specific mask
 */
static inline acmdrv_bitarray_elem_t acmdrv_bitarray_mask(unsigned int bno)
{
	return (1 << (bno % ACMDRV_BITARRAY_NMASKBITS));
}

/**
 * @brief zero a bit field array
 *
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 *
 * @return returns the bit field array pointer
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_zero(size_t size,
	struct acmdrv_bitarray *array)
{
	memset(array, 0, size);

	return array;
}
/**
 * @brief set a bit in a bit field array
 *
 * @param bno bit number
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 *
 * @return returns the bit field array pointer
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_set(unsigned int bno,
	size_t size, struct acmdrv_bitarray *array)
{
	if (bno / NBBY < size)
		array->bits[acmdrv_bitarray_idx(bno)] |=
			acmdrv_bitarray_mask(bno);

	return array;
}

/**
 * @brief clear a bit in a bit field array
 *
 * @param bno bit number
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 *
 * @return returns the bit field array pointer
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_clr(unsigned int bno,
	size_t size, struct acmdrv_bitarray *array)
{
	if (bno / NBBY < size)
		array->bits[acmdrv_bitarray_idx(bno)] &=
			~acmdrv_bitarray_mask(bno);

	return array;
}

/**
 * @brief query if bit in a bit field array is set
 *
 * @param bno bit number
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 *
 * @return true if bit set.
 */
static inline bool acmdrv_bitarray_isset(unsigned int bno, size_t size,
	const struct acmdrv_bitarray *array)
{
	if (bno / NBBY < size)
		return (array->bits[acmdrv_bitarray_idx(bno)] &
			acmdrv_bitarray_mask(bno)) != 0;
	return false;
}

/**
 * @brief count all set bits of a bit field array
 *
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 *
 * @return number of all bits set within given bit field array
 */
static inline unsigned int acmdrv_bitarray_count(size_t size,
	const struct acmdrv_bitarray *array)
{
	int i;
	unsigned int sum = 0;

	_Static_assert(sizeof(acmdrv_bitarray_elem_t)
			== sizeof(unsigned int) ||
		       sizeof(acmdrv_bitarray_elem_t)
			== sizeof(unsigned long) ||
		       sizeof(acmdrv_bitarray_elem_t)
			== sizeof(unsigned long long),
		      "sizeof(acmdrv_bitarray_elem_t)");

	for (i = 0; i < howmany(size, sizeof(acmdrv_bitarray_elem_t)); ++i)
		sum += acmdrv_bitarray_elem_popcount(array->bits[i]);

	return sum;
}

/**
 *
 * @brief compare two bit field arrays
 *
 * @param size size of real bit field structure
 * @param array1 pointer to first bit field array
 * @param array2 pointer to second bit field array
 *
 * @return returns true if bit field arrays are identical
 */
static inline bool acmdrv_bitarray_equal(size_t size,
	const struct acmdrv_bitarray *array1,
	const struct acmdrv_bitarray *array2)
{
	return memcmp(array1, array2, size) == 0;
}

/**
 * @brief logical AND operation for bit field arrays
 *
 * @param size size of real bit field structure
 * @param dst_array pointer to destination bit field array
 * @param src_array1 pointer to first operand bit field array
 * @param src_array2 pointer to second operand bit field array
 *
 * @return returns the destination bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_and(size_t size,
	struct acmdrv_bitarray *dst_array,
	const struct acmdrv_bitarray *src_array1,
	const struct acmdrv_bitarray *src_array2)
{
	size_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));

	for (i = 0; i < elno; ++i)
		dst_array->bits[i] = src_array1->bits[i] & src_array2->bits[i];

	return dst_array;
}

/**
 * @brief logical OR operation for bit field arrays
 *
 * @param size size of real bit field structure
 * @param dst_array pointer to destination bit field array
 * @param src_array1 pointer to first operand bit field array
 * @param src_array2 pointer to second operand bit field array
 *
 * @return returns the destination bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_or(size_t size,
	struct acmdrv_bitarray *dst_array,
	const struct acmdrv_bitarray *src_array1,
	const struct acmdrv_bitarray *src_array2)
{
	size_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));

	for (i = 0; i < elno; ++i)
		dst_array->bits[i] = src_array1->bits[i] | src_array2->bits[i];

	return dst_array;
}

/**
 * @brief logical XOR operation for bit field arrays
 *
 * @param size size of real bit field structure
 * @param dst_array pointer to destination bit field array
 * @param src_array1 pointer to first operand bit field array
 * @param src_array2 pointer to second operand bit field array
 *
 * @return returns the destination bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_xor(size_t size,
	struct acmdrv_bitarray *dst_array,
	const struct acmdrv_bitarray *src_array1,
	const struct acmdrv_bitarray *src_array2)
{
	size_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));

	for (i = 0; i < elno; ++i)
		dst_array->bits[i] = src_array1->bits[i] ^ src_array2->bits[i];

	return dst_array;
}

/**
 * @brief logical NOT operation for bit field array
 *
 * @param size size of real bit field structure
 * @param dst_array pointer to destination bit field array
 * @param src_array pointer to first operand bit field array
 *
 * @return returns the destination bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_not(size_t size,
	struct acmdrv_bitarray *dst_array,
	const struct acmdrv_bitarray *src_array)
{
	size_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));

	for (i = 0; i < elno; ++i)
		dst_array->bits[i] = ~src_array->bits[i];

	return dst_array;
}
/**
 * @brief Find the first least significant bit set
 *
 * @param size size of real bit field structure
 * @param array pointer to bit field array
 * @return one plus the index of the least significant 1-bit of x for x<>0l
 *         else 0.
 */
static inline unsigned int acmdrv_bitarray_ffs(size_t size,
	const struct acmdrv_bitarray *array)
{
	_Static_assert(sizeof(acmdrv_bitarray_elem_t) ==
			sizeof(unsigned int) ||
		       sizeof(acmdrv_bitarray_elem_t) ==
			sizeof(unsigned long) ||
		       sizeof(acmdrv_bitarray_elem_t) ==
			sizeof(unsigned long long),
		       "sizeof(acmdrv_bitarray_elem_t)");

	unsigned int count = 0;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));
	size_t i;

	for (i = 0; i < elno; ++i) {
		int elem_cnt;

		elem_cnt = acmdrv_bitarray_elem_ffs(array->bits[i]);
		if (elem_cnt == 0)
			count += ACMDRV_BITARRAY_NMASKBITS;
		else {
			count += elem_cnt - 1;
			break;
		}
	}
	if (count >= elno * ACMDRV_BITARRAY_NMASKBITS)
		count = 0;
	else
		count++;

	return count;
}

/**
 * @brief SHIFT RIGHT operator for bit field array
 *
 * @param size size of real bit field structure
 * @param shift number of bits to shift right
 * @param array pointer to bit field array
 *
 * @return returns pointer to bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_shr(size_t size,
	unsigned int shift, struct acmdrv_bitarray *array)
{
	size_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));
	const size_t mshift = (shift) % ACMDRV_BITARRAY_NMASKBITS;
	const size_t delta = (shift) / ACMDRV_BITARRAY_NMASKBITS;

	for (i = 0; i < elno; ++i) {
		acmdrv_bitarray_elem_t v;

		if (i + delta >= elno)
			v = 0;
		else {
			v = array->bits[i + delta] >> mshift;
			if (mshift > 0 && i + delta + 1 < elno)
				v |= (array->bits[i + delta + 1] <<
				(ACMDRV_BITARRAY_NMASKBITS - mshift));
		}
		array->bits[i] = v;
	}

	return array;
}

/**
 * @brief SHIFT LEFT operator for bit field array
 *
 * @param size size of real bit field structure
 * @param shift number of bits to shift left
 * @param array pointer to bit field array
 *
 * @return returns pointer to bit field array
 */
static inline struct acmdrv_bitarray *acmdrv_bitarray_shl(size_t size,
	unsigned int shift, struct acmdrv_bitarray *array)
{
	ssize_t i;
	size_t elno = howmany((size), sizeof(acmdrv_bitarray_elem_t));
	const size_t mshift = (shift) % ACMDRV_BITARRAY_NMASKBITS;
	const size_t delta = (shift) / ACMDRV_BITARRAY_NMASKBITS;

	for (i = elno - 1; i >= 0; --i) {
		acmdrv_bitarray_elem_t v;

		if (i < delta)
			v = 0;
		else {
			v = array->bits[i - delta] << mshift;
			if (mshift > 0 && i > delta)
				v |= (array->bits[i - delta - 1] >>
					(ACMDRV_BITARRAY_NMASKBITS - mshift));
		}
		array->bits[i] = v;
	}

	return array;
}

/**
 * @brief Get a field (i.e. range of bits) within the bit field array using a
 * mask value
 *
 * @param size size of real bit field structure
 * @param mask pointer to a mask bit field array
 * @param array pointer to data bit field array
 *
 * @return returns value of the chosen mask range
 */
static inline acmdrv_bitarray_elem_t acmdrv_bitmask_field_get(size_t size,
	const struct acmdrv_bitarray *mask, const struct acmdrv_bitarray *array)
{
	/* ensure proper alignment */
	char buffer[size]
		    __attribute__((aligned(sizeof(acmdrv_bitarray_elem_t))));
	struct acmdrv_bitarray *tmp = (void *)buffer;

	return acmdrv_bitarray_shr(size, acmdrv_bitarray_ffs(size, mask) - 1,
		acmdrv_bitarray_and(size, tmp, mask, array))->bits[0];
}

/**
 * @brief Set a field (i.e. range of bits) within the bit field array using a
 *        mask value
 *
 * @param size size of real bit field structure
 * @param mask pointer to a mask bit field array
 * @param array pointer to data bit field array
 * @param value Value to be set in the given field
 *
 * @return returns value of the chosen mask range
 */
static inline struct acmdrv_bitarray *acmdrv_bitmask_field_set(size_t size,
	const struct acmdrv_bitarray *mask, struct acmdrv_bitarray *array,
	acmdrv_bitarray_elem_t value)
{
	/* ensure proper alignment */
	char buffer[size]
		    __attribute__((aligned(sizeof(acmdrv_bitarray_elem_t))));
	char mbuffer[size]
		     __attribute__((aligned(sizeof(acmdrv_bitarray_elem_t))));

	struct acmdrv_bitarray *tmp = (void *)buffer;
	struct acmdrv_bitarray *nmask = (void *)mbuffer;

	acmdrv_bitarray_zero(size, tmp)->bits[0] = value;

	return acmdrv_bitarray_or(size, array,
		acmdrv_bitarray_and(size, array, array,
			acmdrv_bitarray_not(size, nmask, mask)),
		acmdrv_bitarray_shl(size,
			acmdrv_bitarray_ffs(size, mask) - 1, tmp));
}

/**
 * @brief assign bit field array to another
 *
 * @param size size of real bit field structure
 * @param lh pointer to left-hand bit field array
 * @param rh pointer to right-hand bit field array
 *
 * @return returns the result, i.e. left-hand bit field array pointer
 */
static inline struct acmdrv_bitarray *acmdrv_bitmask_assign(size_t size,
	struct acmdrv_bitarray *lh, const struct acmdrv_bitarray *rh)
{
	memcpy(lh->bits, rh->bits, size);
	return lh;
}

/**
 * @brief assign bit field array to another
 *
 * @param size size of real bit field structure
 * @param mask pointer to mask bit field array to be generated
 * @param hi most significant bit of mask
 * @param lo least significant bit of mask
 *
 * if lo > hi resulting mask will be 0 at all bits.
 *
 * @return returns the result, i.e. mask bit field array pointer
 */
static inline struct acmdrv_bitarray *acmdrv_bitmask_genmask(size_t size,
	struct acmdrv_bitarray *mask, unsigned int hi, unsigned int lo)
{
	/* ensure proper alignment */
	char buffer1[size]
		     __attribute__((aligned(sizeof(acmdrv_bitarray_elem_t))));
	char buffer2[size]
		     __attribute__((aligned(sizeof(acmdrv_bitarray_elem_t))));

	struct acmdrv_bitarray *tmp1 = (void *)buffer1;
	struct acmdrv_bitarray *tmp2 = (void *)buffer2;

	acmdrv_bitarray_zero(size, mask);
	if (lo > hi)
		return mask;

	/* tmp1 = tmp2 = ~0 */
	acmdrv_bitarray_not(size, tmp2, acmdrv_bitarray_zero(size, tmp2));
	acmdrv_bitmask_assign(size, tmp1, tmp2);

	/* ((~0) << lo) & ((~0) >> (size - 1 - hi))*/
	return acmdrv_bitarray_and(size, mask,
		acmdrv_bitarray_shl(size, lo, tmp1),
		acmdrv_bitarray_shr(size, size * NBBY - 1 - hi, tmp2));
}
/******************************************************************************/
/**
 * @defgroup acmapi ACM Userspace API
 * @brief The ACM kernel module's userspace API
 *
 * The API the ACM kernel module provides consists of the follow parts:
 * * @ref acmsysfs "The ACM kernel module's SYSFS interface"
 * * @ref acmmsgbuf "The ACM kernel module's message buffer interface"
 * * @ref acmdrvdata "The ACM kernel module's driver data items"
 * * @ref acmdevtree "The ACM kernel module's devicetree configuration"
 * * @ref acmmodparam "The ACM kernel module's module parameters"
 * @{
 */

/******************************************************************************/

/**
 * @defgroup acmsysfs ACM SYSFS
 * @brief The ACM kernel module's SYSFS interface
 *
 * @{
 *
 * The ACM kernel module exposes its control and status interface as part of the
 * <a href="https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt">
 * Linux SYSFS filesystem</a>. This is provided by Linux kernel device
 * attributes. The ACM device name is determined by the node name of the ACM
 * device in the @ref acmdevtree "Linux devicetree", so using e.g.
 *
 *     ngn {
 *         compatible = "ttt,acm-x.y";
 *         ...
 *     };
 *
 * will result in <b>`/sys/devices/ngn`</b> as the base location for the ACM
 * kernel module's SYSFS interface.
 *
 * There are five different sections:
 * * @ref acmsysfscontrol "The control attributes of the ACM"
 * * @ref acmsysfsconfig "The configuration attributes of the ACM"
 * * @ref acmsysfserror "The error attributes of the ACM"
 * * @ref acmsysfsstatus "The status attributes of the ACM"
 * * @ref acmsysfsdiag "The diagnostic data of the ACM"
 */

/**
 * @brief number of bypass modules within the ACM IP
 */
#define ACMDRV_BYPASS_MODULES_COUNT	2

/**
 * @defgroup acmsysfscontrol ACM SYSFS Control Section
 * @brief The control attributes of the ACM
 *
 * The SYSFS control section provides the following binary attributes under
 * subdirectory specified by #ACMDRV_SYSFS_CONTROL_GROUP:
 * - *lock_msg_bufs*: one acmdrv_msgbuf_lock_ctrl field to lock message buffers
 * - *unlock_msg_bufs*: one acmdrv_msgbuf_lock_ctrl field to unlock message
 *                      buffers
 * - *overwritten*: read-only array of acmdrv_msgbuf_overwritten message buffer
 *                  overwrite counters. Represents the minimum value a message
 *                  buffer overflow (in read or write direction) has occurred.
 *                  Each array element is cleared on read.
 *
 *                  Remark: this counter array is not functional in early ACM IP
 *                  versions.
 *
 * @{
 */

/**
 * @brief ACM Control section SYSFS directory
 */
#define ACMDRV_SYSFS_CONTROL_GROUP	control_bin


/** @brief
 * maximum supported lock control bit field size
 *  (i.e. number of message buffers)
 *
 * According to ACM IP ICD, ch. 9. Message Buffer
 */
#define ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE	64

/**
 * @struct acmdrv_msgbuf_lock_ctrl
 * @brief Message Buffer Locking Control
 *
 * Due to the dynamic message buffer count the lock control bit fields might
 * exceed the natively supported bit widths. So lock control bit fields are
 * implemented as bit arrays and must therefore only be manipulated by
 * ACMDRV_MSGBUF_LOCK_CTRL_* macros.
 *
 * @var acmdrv_msgbuf_lock_ctrl::bits
 * @brief array of elements of the bit field array
 */
struct acmdrv_msgbuf_lock_ctrl {
	acmdrv_bitarray_elem_t bits[ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE /
				(sizeof(acmdrv_bitarray_elem_t) * NBBY)];
};

/**
 * @name ACM Message Buffer Lock Control Macros
 * @brief Wrapper macros used to manipulate a struct acmdrv_msgbuf_lock_ctrl
 *
 * @{
 */
#define ACMDRV_MSGBUF_LOCK_CTRL_ZERO(lcp)				\
	((__typeof__(lcp))						\
	acmdrv_bitarray_zero(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(lcp)))
#define ACMDRV_MSGBUF_LOCK_CTRL_SET(bno, lcp)				\
	((__typeof__(lcp))						\
	acmdrv_bitarray_set(bno, sizeof(struct acmdrv_msgbuf_lock_ctrl),\
		(struct acmdrv_bitarray *)(lcp)))
#define ACMDRV_MSGBUF_LOCK_CTRL_CLR(bno, lcp)				\
	((__typeof__(lcp))						\
	acmdrv_bitarray_clr(bno, sizeof(struct acmdrv_msgbuf_lock_ctrl),\
		(struct acmdrv_bitarray *)(lcp)))
#define ACMDRV_MSGBUF_LOCK_CTRL_ISSET(bno, lcp)				\
	acmdrv_bitarray_isset(bno, sizeof(struct acmdrv_msgbuf_lock_ctrl),\
		(const struct acmdrv_bitarray *)(lcp))
#define ACMDRV_MSGBUF_LOCK_CTRL_COUNT(lcp)				\
	acmdrv_bitarray_count(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(const struct acmdrv_bitarray *)(lcp))
#define ACMDRV_MSGBUF_LOCK_CTRL_EQUAL(lcp1, lcp2)			\
	acmdrv_bitarray_equal(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(const struct acmdrv_bitarray *)(lcp1),			\
		(const struct acmdrv_bitarray *)(lcp2))
#define ACMDRV_MSGBUF_LOCK_CTRL_AND(dlcp, slcp1, slcp2)			\
	((__typeof__(dlcp))						\
	acmdrv_bitarray_and(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(dlcp),			\
		(const struct acmdrv_bitarray *)(slcp1),		\
		(const struct acmdrv_bitarray *)(slcp2)))
#define ACMDRV_MSGBUF_LOCK_CTRL_OR(dlcp, slcp1, slcp2)			\
	((__typeof__(dlcp))						\
	acmdrv_bitarray_or(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(dlcp),			\
		(const struct acmdrv_bitarray *)(slcp1),		\
		(const struct acmdrv_bitarray *)(slcp2)))
#define ACMDRV_MSGBUF_LOCK_CTRL_XOR(dlcp, slcp1, slcp2)			\
	((__typeof__(dlcp))						\
	acmdrv_bitarray_xor(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(dlcp),			\
		(const struct acmdrv_bitarray *)(slcp1),		\
		(const struct acmdrv_bitarray *)(slcp2)))
#define ACMDRV_MSGBUF_LOCK_CTRL_NOT(dlcp, slcp)				\
	((__typeof__(dlcp))						\
	acmdrv_bitarray_not(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(dlcp),			\
		(const struct acmdrv_bitarray *)(slcp)))
#define ACMDRV_MSGBUF_LOCK_CTRL_FIELD_GET(mask, lcp)			\
	acmdrv_bitmask_field_get(sizeof(struct acmdrv_msgbuf_lock_ctrl),\
		(const struct acmdrv_bitarray *)(mask),			\
		(const struct acmdrv_bitarray *)(lcp))
#define ACMDRV_MSGBUF_LOCK_CTRL_FIELD_SET(mask, lcp, value)		\
	((__typeof__(lcp))						\
	acmdrv_bitmask_field_set(sizeof(struct acmdrv_msgbuf_lock_ctrl),\
		(const struct acmdrv_bitarray *)(mask),			\
		(struct acmdrv_bitarray *)(lcp),			\
		(acmdrv_bitarray_elem_t)(value)))
#define ACMDRV_MSGBUF_LOCK_CTRL_ASSIGN(to_lcp, from_lcp)		\
	((__typeof__(to_lcp))						\
	acmdrv_bitmask_assign(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(to_lcp),			\
		(const struct acmdrv_bitarray *)(from_lcp)))
#define ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(mask, hi, lo)			\
	((__typeof__(mask))						\
	acmdrv_bitmask_genmask(sizeof(struct acmdrv_msgbuf_lock_ctrl),	\
		(struct acmdrv_bitarray *)(mask), (hi), (lo)))

/** @} acmmsgbuflockctrl */

/**
 * @brief Overwritten Counter for Message Buffers
 */
struct acmdrv_msgbuf_overwritten {
	uint32_t count;	/**< counter value */
};

/**@} acmsysfscontrol*/

/******************************************************************************/
/**
 * @defgroup acmsysfsconfig ACM SYSFS Configuration Section
 * @brief The configuration attributes of the ACM
 *
 * All config SYSFS interfaces generally have the following array-like format
 * and map to the ACM IP registers or block ranges as outlined. The creation and
 * access helpers in this file provide common access to such structures. Sysfs
 * data item may be padded, fortunately most of them are not, so padding is
 * usually reduce to none. Items can be accessed individually (item per item)
 * or as entire block of items. These accesses are required to be aligned to
 * item size boundaries.
 *
 * Remark: interval might be degenerated to item size.
 *
 *     SYSFS representation                     Address map of ACM-IP
 *     of 2*n items in data array X
 *                                     +~~~~~~~~~~~+--+ <- base (in bypass 0)
 *                                    /| item 0    |  |
 *                                   / +~~~~~~~~~~~+  |
 *                                  / /               |
 *     data array X -> +~~~~~~~~~~~+ /             interval
 *     bypass module 0 | item 0    |/                 |
 *                     +-----------+                  |
 *                     | padding   |                  |
 *             offs -->+~~~~~~~~~~~+---+~~~~~~~~~~~+<-+ <- base + interval
 *                     | item 1    |   | item 1    |  |
 *                     +-----------+---+~~~~~~~~~~~+  |
 *                     | padding   |                  |
 *                     +~~~~~~~~~~~+                  |
 *                     |    ...    |            (n-1)*interval
 *                     +~~~~~~~~~~~+                  |
 *                     | item n-1  |\                 |
 *                     +-----------+ \                |
 *                     | padding   |\ \               |
 *     data array X -> +~~~~~~~~~~~+ \ +~~~~~~~~~~~+<-+ <- base + (n-1)*interval
 *     bypass module 1 | item n    |\ \| item n-1  |
 *                     +-----------+ \ +~~~~~~~~~~~+
 *                     | padding   |\ \
 *                     +~~~~~~~~~~~+ \ +~~~~~~~~~~~+ <---- base (in bypass 1)
 *                     |    ...    |  \| item n    |
 *                     +~~~~~~~~~~~+   +~~~~~~~~~~~+
 *                     | item 2n-1 |
 *                     +-----------+
 *                     | padding   |
 *                     +~~~~~~~~~~~+
 *
 * The SYSFS config section provides the following binary attributes under
 * subdirectory specified by #ACMDRV_SYSFS_CONFIG_GROUP:
 * - *configuration_id*: user id to identify an IP configuration
 * - *config_state*: state of the ACM. Contains a value according to enum
 *                   #acmdrv_status
 * - *msg_buff_desc*: ACM message buffer descriptor array of struct
 *                    acmdrv_buff_desc items, one for each message buffer
 * - *msg_buff_alias*: ACM message buffer alias array of
 *                     struct acmdrv_buff_alias data items, one for each message
 *                     buffer
 * - *lookup_pattern*: Access to #ACMDRV_BYPASS_NR_RULES bypass module lookup
 *                     patterns of struct acmdrv_bypass_lookup for both bypass
 *                     modules
 * - *lookup_mask*: Access to #ACMDRV_BYPASS_NR_RULES bypass module lookup masks
 *                  of struct acmdrv_bypass_lookup for both bypass modules
 * - *layer7_pattern*: Access to #ACMDRV_BYPASS_NR_RULES bypass module layer7
 *                     check patterns of struct acmdrv_bypass_layer7_check for
 *                     both bypass modules
 * - *layer7_mask*: Access to #ACMDRV_BYPASS_NR_RULES bypass module layer7 check
 *                     masks of struct acmdrv_bypass_layer7_check for both
 *                     bypass modules
 * - *stream_trigger*: Access to #ACMDRV_BYPASS_NR_RULES + 1 bypass module
 *                     stream trigger words of
 *                     struct acmdrv_bypass_stream_trigger for both bypass
 *                     modules
 * - *scatter_dma*: Access to #ACMDRV_BYPASS_SCATTER_DMA_CMD_COUNT bypass module
 *                  scatter DMA commands of struct acmdrv_bypass_dma_command for
 *                  both bypass modules
 * - *prefetch_dma*: Access to #ACMDRV_BYPASS_PREFETCH_DMA_CMD_COUNT bypass
 *                   module prefetch DMA commands of
 *                   struct acmdrv_bypass_dma_command for both bypass modules
 * - *gather_dma*: Access to #ACMDRV_BYPASS_GATHER_DMA_CMD_COUNT bypass module
 *                 gather DMA commands of struct acmdrv_bypass_dma_command for
 *                 both bypass modules
 * - *const_buffer*: Access to const buffer area (struct
 *                   acmdrv_bypass_const_buffer) for both bypass modules
 * - *redund_cnt_tab*: Redundancy control table access. Contains an array of
 *                     struct acmdrv_redun_ctrl_entry redundancy control
 *                     table entries, #ACMDRV_REDUN_TABLE_ENTRY_COUNT items
 *                     for each of the #ACMDRV_REDUN_CTRLTAB_COUNT tables
 * - *redund_status_tab*: Redundancy status table readonly access. Contains an
 *                        array of #ACMDRV_REDUN_TABLE_ENTRY_COUNT
 *                        redundancy struct acmdrv_redun_status status
 *                        table entries
 * - *redund_intseqnum_tab*: Redundancy Internal Sequence Number table access.
 *                           Contains an array of
 *                           #ACMDRV_REDUN_TABLE_ENTRY_COUNT redundancy
 *                           struct acmdrv_redun_intseqnum table entries
 * - *sched_down_counter*: Access to via struct acmdrv_scheduler_down_counter to
 *                         both schedulers
 * - *sched_tab_row*: Access to the struct acmdrv_sched_tbl_row scheduler
 *                    table rows. There are #ACMDRV_SCHED_TBL_COUNT
 *                    tables per scheduler each containing
 *                    #ACMDRV_SCHED_TBL_ROW_COUNT rows
 * - *table_status*: Array of struct acmdrv_sched_tbl_status scheduler
 *                   table status values (#ACMDRV_SCHED_TBL_COUNT tables
 *                    (readonly) for each of the #ACMDRV_SCHEDULER_COUNT
 *                    schedulers)
 * - *sched_cycle_time*: Array of struct acmdrv_sched_cycle_time data of
 *                       scheduler table cycle times
 * - *sched_start_table*: Array of struct acmdrv_timespec64 data of
 *                        scheduler table start times
 * - *emergency_disable*: Access to both scheduler's struct
 *                        acmdrv_sched_emerg_disable emergency disable
 *                        registers
 * - *cntl_ngn_enable*: Access to both 32bit bypass control enable registers
 * - *cntl_lookup_enable*: Access to both 32bit_bypass lookup_enable registers
 * - *cntl_layer7_enable*: Access to both 32bit layer7 enable registers
 * - *cntl_ingress_policing_enable*: Access to both 32bit ingress policing
 *                                   enable registers
 * - *cntl_connection_mode*: Access to both 32bit connection mode registers
 * - *cntl_output_disable*: Access to both 32bit output disable registers
 * - *cntl_layer7_length*: Access to both 32bit connection mode registers
 * - *cntl_speed*: Access to both 32bit speed registers
 * - *cntl_gather_delay*: Access to both 32bit gather delay registers
 * - *cntl_ingress_policing_control*: Access to both 32bit ingress policing
 *                                    control registers
 * - *clear_all_fpga*: Clear/initialize/reset entire ACM IP by writing
 *                     #ACMDRV_CLEAR_ALL_PATTERN
 * - *individual_recovery*: enable/disable individual recovery per module and
 *                          set receive timeouts using
 *                          struct acmdrv_redun_individual_recovery
 * - *base_recovery*: enable/disable base recovery per IntSeqNUm table index
 *                          and set receive timeouts using
 *                          struct acmdrv_redun_base_recovery
 * @{
 */

/**
 * @brief ACM Configuration section SYSFS directory
 */
#define ACMDRV_SYSFS_CONFIG_GROUP	config_bin

/**
 * @brief ACM configuration state control
 *
 * enum acmdrv_status is used to control the configuration process of the ACM
 * module at the `config_bin/config_state` sysfs interface.
 */
enum acmdrv_status {
	ACMDRV_UNKNOWN_STATE = 0,	/**< ACM not yet initialized */
	ACMDRV_INIT_STATE,		/**< ACM ready for configuration */
	ACMDRV_CONFIG_START_STATE,	/**< ACM start configuration */
	ACMDRV_CONFIG_END_STATE,	/**< ACM configuration done */
	ACMDRV_RUN_STATE,		/**< ACM is running */
	ACMDRV_DESYNC_STATE,	/**< ACM stop running */
	ACMDRV_RESTART_STATE,	/**< ACM resume running */

	ACMDRV_MAX_STATE
};

/**
 * @struct acmdrv_buff_desc
 * @brief ACM Message Buffer Descriptor
 *
 * @var acmdrv_buff_desc::desc
 * @brief message buffer descriptor value
 *
 * see @ref acmdrv_buff_desc_desc_details "Details of acmdrv_buff_desc.desc"
 */
struct acmdrv_buff_desc {
	uint32_t desc;
};

/**
 * @name bit structure of acmdrv_buff_desc.desc
 * @{
 */
#define ACMDRV_BUFF_DESC_BUFF_OFFSET_BIT_L	0
#define ACMDRV_BUFF_DESC_BUFF_OFFSET_BIT_H	15
#define ACMDRV_BUFF_DESC_BUFF_RST_BIT_L		19
#define ACMDRV_BUFF_DESC_BUFF_RST_BIT_H		19
#define ACMDRV_BUFF_DESC_BUFF_TYPE_BIT_L	20
#define ACMDRV_BUFF_DESC_BUFF_TYPE_BIT_H	20
#define ACMDRV_BUFF_DESC_SUB_BUFF_SIZE_BIT_L	21
#define ACMDRV_BUFF_DESC_SUB_BUFF_SIZE_BIT_H	29
#define ACMDRV_BUFF_DESC_HAS_TIMESTAMP_BIT_L	30
#define ACMDRV_BUFF_DESC_HAS_TIMESTAMP_BIT_H	30
#define ACMDRV_BUFF_DESC_VALID_BIT_L		31
#define ACMDRV_BUFF_DESC_VALID_BIT_H		31
/** @} */

/**
 * @name Details of acmdrv_buff_desc.desc
 * @anchor acmdrv_buff_desc_desc_details
 * @{
 */
/**
 * @def ACMDRV_BUFF_DESC_BUFF_OFFSET
 * @brief Buffer Offset
 *
 * Buffer Offset indicates DWORD (i. e., 4-byte granularity) offset in the
 * Message Buffer Memory, where a particular Message Buffer starts.
 */
#define ACMDRV_BUFF_DESC_BUFF_OFFSET	\
	BITMASK(ACMDRV_BUFF_DESC_BUFF_OFFSET_BIT)

/**
 * @def ACMDRV_BUFF_DESC_BUFF_RST
 * @brief Buffer Reset
 *
 * Buffer Reset indicates whether the buffer should be emptied upon loss of
 * synchronization
 */
#define ACMDRV_BUFF_DESC_BUFF_RST	\
	BITMASK(ACMDRV_BUFF_DESC_BUFF_RST_BIT)

/**
 * @def ACMDRV_BUFF_DESC_BUFF_TYPE
 * @brief Buffer Type
 *
 * * 0 = RX buffer (Scatter DMA -> Message Buffer -> Host)
 * * 1 = TX buffer (Host -> Message Buffer -> Gather DMA)
 */
#define ACMDRV_BUFF_DESC_BUFF_TYPE	BITMASK(ACMDRV_BUFF_DESC_BUFF_TYPE_BIT)

/**
 * @brief ACM Message Buffer Descriptor Type enumeration
 */
enum acmdrv_buff_desc_type {
	ACMDRV_BUFF_DESC_BUFF_TYPE_RX = 0, /**< RX buffer */
	ACMDRV_BUFF_DESC_BUFF_TYPE_TX = 1, /**< TX buffer */
};

/**
 * @def ACMDRV_BUFF_DESC_SUB_BUFF_SIZE
 * @brief Sub-Buffer Size
 *
 * Sub-Buffer Size indicates the Sub-buffer size in DWORDS decreased by 1
 */
#define ACMDRV_BUFF_DESC_SUB_BUFF_SIZE	\
	BITMASK(ACMDRV_BUFF_DESC_SUB_BUFF_SIZE_BIT)

/**
 * @def ACMDRV_BUFF_DESC_HAS_TIMESTAMP
 * @brief Timestamp Flag
 *
 * Timestamp Flag indicates time stamp use
 */
#define ACMDRV_BUFF_DESC_HAS_TIMESTAMP	\
	BITMASK(ACMDRV_BUFF_DESC_HAS_TIMESTAMP_BIT)

/**
 * @def ACMDRV_BUFF_DESC_VALID
 * @brief Valid Flag
 *
 * Message buffer is valid only if this flag is set to 1
 */
#define ACMDRV_BUFF_DESC_VALID	BITMASK(ACMDRV_BUFF_DESC_VALID_BIT)

/**
 * @brief Helper to create message buffer descriptor value
 *
 * @param offset Buffer Offset
 * @param reset Buffer Reset
 * @param type Buffer Type
 * @param buffer_size Buffer Size in msgbuf entry granularity (e.g. DWORD)
 * @param timestamp Timestamp flag
 * @param valid valid flag
 * @return value for acmdrv_buff_desc.desc
 */
static inline uint32_t acmdrv_buff_desc_create(uint16_t offset,
					      bool reset,
					      enum acmdrv_buff_desc_type type,
					      uint16_t buffer_size,
					      bool timestamp,
					      bool valid)
{
	return WVAL(ACMDRV_BUFF_DESC_BUFF_OFFSET_BIT, offset) |
	       WVAL(ACMDRV_BUFF_DESC_BUFF_RST_BIT, reset) |
	       WVAL(ACMDRV_BUFF_DESC_BUFF_TYPE_BIT, type) |
	       WVAL(ACMDRV_BUFF_DESC_SUB_BUFF_SIZE_BIT, buffer_size - 1) |
	       WVAL(ACMDRV_BUFF_DESC_HAS_TIMESTAMP_BIT, timestamp) |
	       WVAL(ACMDRV_BUFF_DESC_VALID_BIT, valid);
}

/**
 * @brief Helper to read buffer offset from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return buffer offset
 */
static inline uint16_t acmdrv_buff_desc_offset_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_BUFF_OFFSET_BIT, desc->desc);
}

/**
 * @brief Helper to read buffer reset flag from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return reset flag
 */
static inline bool acmdrv_buff_desc_reset_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_BUFF_RST_BIT, desc->desc);
}

/**
 * @brief Helper to read buffer type from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return buffer type
 */
static inline enum acmdrv_buff_desc_type acmdrv_buff_desc_type_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_BUFF_TYPE_BIT, desc->desc);
}

/**
 * @brief Helper to read raw sub-buffer size from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return buffer size, i.e. number of entries in respective granularity
 */
static inline uint16_t acmdrv_buff_desc_sub_buffer_size_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_SUB_BUFF_SIZE_BIT, desc->desc) + 1;
}

/**
 * @brief Helper to read timestamp from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return timestamp
 */
static inline bool acmdrv_buff_desc_timestamp_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_HAS_TIMESTAMP_BIT, desc->desc);
}

/**
 * @brief Helper to read valid flag from given buffer descriptor
 *
 * @param desc pointer to buffer descriptor
 * @return valid flag
 */
static inline bool acmdrv_buff_desc_valid_read(
		const struct acmdrv_buff_desc *desc)
{
	return RVAL(ACMDRV_BUFF_DESC_VALID_BIT, desc->desc);
}
/** @} */

/**
 * @brief message buffer alias representation
 *
 * Provides the association of message buffer index to its name
 */
struct acmdrv_buff_alias {
	uint8_t		idx;	/**< message buffer identifier */
	uint64_t	id;	/**< deprecated: stream identifier */
	char		alias[55]; /**< alias name: only 55 (pads to 64) */
} __packed;


/**
 * @brief Helper to initialize message buffer alias
 *
 * @param alias pointer to message buffer alias to be initialized
 * @param idx message buffer identifier
 * @param name message buffer namse
 * @return 0 on success, negative error value otherwise
 */
static inline int __must_check acmdrv_buff_alias_init(
	struct acmdrv_buff_alias *alias, uint8_t idx, const char *name)
{
	int ret;

	if (!alias || !name)
		return -EINVAL;

	alias->idx = idx;
	alias->id = 0;
	ret = snprintf(alias->alias, sizeof(alias->alias), "%s", name);

	if (ret >= sizeof(alias->alias))
		return -EINVAL;

	return ret < 0 ? ret : 0;
}

/**
 * @brief Helper to read message buffer identifier from message buffer alias
 *
 * @param alias pointer to message buffer alias
 * @return message buffer identifier
 */
static inline uint8_t acmdrv_buff_alias_idx_read(
	const struct acmdrv_buff_alias *alias)
{
	return alias->idx;
}

/**
 * @brief Helper to read message buffer name from message buffer alias
 *
 * @param alias pointer to message buffer alias
 * @return message buffer name
 */
static inline const char *acmdrv_buff_alias_alias_read(
	const struct acmdrv_buff_alias *alias)
{
	return alias->alias;
}

/**
 * @brief number of lookup rules (rule id & layer7)
 */
#define ACMDRV_BYPASS_NR_RULES	0x10

/**
 * @brief Bypass module rule id data structure
 */
struct acmdrv_bypass_lookup {
	uint8_t	dmac[6];	/**< 0:5 : Destination MAC */
	uint8_t	smac[6];	/**< 6:11 : Source MAC */
	uint8_t	vlan[4];	/**< 12:15 : VLAN ID */
} __packed;

/**
 * @brief Bypass module access structure for layer7_pattern and layer7_mask
 *        interface
 */
struct acmdrv_bypass_layer7_check {
	uint8_t	data[0x70];	/**< layer7 pattern/mask data */
	uint8_t	padding[0x10];	/**< padded to 128 bytes */
} __packed;

/**
 * @struct acmdrv_bypass_stream_trigger
 * @brief access structure for stream_trigger interface
 *
 * @var acmdrv_bypass_stream_trigger::trigger
 * @brief trigger data value
 *
 * see @ref acmdrv_bypass_stream_trigger_trigger_details "Details"
 */
struct acmdrv_bypass_stream_trigger {
	uint32_t trigger;
} __packed;

/**
 * @name bit structure of acmdrv_bypass_stream_trigger.trigger
 * @{
 */
#define ACMDRV_BYPASS_STREAM_TRIG_VALID_BIT_L		0
#define ACMDRV_BYPASS_STREAM_TRIG_VALID_BIT_H		0
#define ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP_BIT_L	3
#define ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP_BIT_H	3
#define ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR_BIT_L	8
#define ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR_BIT_H	15
#define ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR_BIT_L	17
#define ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR_BIT_H	24
#define ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID_BIT_L	26
#define ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID_BIT_H	30
/** @} */

/**
 * @name Details of acmdrv_bypass_stream_trigger.trigger
 * @anchor acmdrv_bypass_stream_trigger_trigger_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_STREAM_TRIG_VALID
 * @brief Valid Flag
 *
 * Indicates whether command is valid.
 * * 0 - invalid
 * * 1 - valid
 */
#define ACMDRV_BYPASS_STREAM_TRIG_VALID	\
	BITMASK(ACMDRV_BYPASS_STREAM_TRIG_VALID_BIT)

/**
 * @def ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP
 * @brief Force Drop
 *
 * When this flag is set, both the Scatter and the Gather DMAs are forced to
 * drop the actual frame and ignore the values in the Scatter DMA Chain Pointer
 * and Gather DMA Chain Pointer fields.
 * * 0 - Normal operation (DMA lists are processed)
 * * 1 - Force Drop (remove actual frame for both frame buffers)
 */
#define ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP	\
	BITMASK(ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP_BIT)

/**
 * @def ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR
 * @brief Gather DMA Chain Pointer
 *
 * Pointer to both Gather DMA Prefetch Chain and Gather DMA Execution Chain
 */
#define ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR	\
	BITMASK(ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR_BIT)

/**
 * @def ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR
 * @brief Scatter DMA Chain Pointer
 *
 * Pointer to Scatter DMA Chain
 */
#define ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR	\
	BITMASK(ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR_BIT)

/**
 * @def ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID
 * @brief Redundancy Group ID
 *
 * ID of the redundancy group inside the NGN Redundancy module
 */
#define ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID	\
	BITMASK(ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID_BIT)

/**
 * @brief Helper to create trigger value
 *
 * @param valid valid flag
 * @param force_drop force drop flag
 * @param gdma_chain_ptr Gather DMA Chain Pointer
 * @param sdma_chain_ptr Scatter DMA Chain Pointer
 * @param red_grp_id Redundancy Group ID
 */
static inline uint32_t acmdrv_bypass_stream_trigger_create(
	bool valid,
	bool force_drop,
	uint8_t gdma_chain_ptr,
	uint8_t sdma_chain_ptr,
	uint8_t red_grp_id)
{
	return WVAL(ACMDRV_BYPASS_STREAM_TRIG_VALID_BIT, valid) |
	       WVAL(ACMDRV_BYPASS_STREAM_TRIG_FORCE_DROP_BIT, force_drop) |
	       WVAL(ACMDRV_BYPASS_STREAM_TRIG_GDMA_CHAIN_PTR_BIT,
		    gdma_chain_ptr) |
	       WVAL(ACMDRV_BYPASS_STREAM_TRIG_SDMA_CHAIN_PTR_BIT,
		    sdma_chain_ptr) |
	       WVAL(ACMDRV_BYPASS_STREAM_TRIG_RED_GRP_ID_BIT, red_grp_id);
}

/** @} */

/**
 * @struct acmdrv_bypass_dma_command
 * @brief access structure for gather/scatter DMA interface
 *
 * @var acmdrv_bypass_dma_command::cmd
 * @brief DMA command value
 *
 * see @ref acmdrv_bypass_s_dma_cmds_details "Details for Scatter DMA
 * commands"\n
 * see @ref acmdrv_bypass_p_dma_cmds_details "Details for Prefetch DMA
 * commands"\n
 * see @ref acmdrv_bypass_g_dma_cmds_details "Details for Gather DMA commands"
 */
struct acmdrv_bypass_dma_command {
	uint32_t cmd;
};

/**
 * @brief number of scatter DMA commands
 */
#define ACMDRV_BYPASS_SCATTER_DMA_CMD_COUNT	0x100
/**
 * @brief number of gather DMA prefetch commands
 */
#define ACMDRV_BYPASS_PREFETCH_DMA_CMD_COUNT	0x100
/**
 * @brief number of gather DMA execution commands
 */
#define ACMDRV_BYPASS_GATHER_DMA_CMD_COUNT	0x100

/* scatter dma commands */
/**
 * @name bit structure for scatter dma commands
 * @{
 */
#define ACMDRV_BYPASS_DMA_CMD_S_LAST_BIT_L		0
#define ACMDRV_BYPASS_DMA_CMD_S_LAST_BIT_H		0
#define ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT_L		1
#define ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT_H		2
#define ACMDRV_BYPASS_DMA_CMD_S_IRQ_BIT_L		3
#define ACMDRV_BYPASS_DMA_CMD_S_IRQ_BIT_H		3
#define ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS_BIT_L	4
#define ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS_BIT_H	14
#define ACMDRV_BYPASS_DMA_CMD_S_LENGTH_BIT_L		15
#define ACMDRV_BYPASS_DMA_CMD_S_LENGTH_BIT_H		25
#define ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID_BIT_L	26
#define ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID_BIT_H	30
/** @} */

/**
 * @name Details of ACM bypass Scatter DMA Commands
 * @anchor acmdrv_bypass_s_dma_cmds_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_LAST
 * @brief Last
 *
 * Last indicates whether the command is last command in the chain
 * * 0 - More commands follow
 * * 1 - Last (it will also drop the frame from the frame buffer)
 */
#define ACMDRV_BYPASS_DMA_CMD_S_LAST	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_LAST_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_COMMAND
 * @brief Command Type
 */
#define ACMDRV_BYPASS_DMA_CMD_S_COMMAND	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT)

/**
 * @enum acmdrv_bypass_dma_s_cmd
 * @brief ACM Bypass Scatter DMA Command Types
 *
 * @var acmdrv_bypass_dma_s_cmd::ACMDRV_BYPASS_DMA_CMD_S_COMMAND_INVALID
 * @brief Invalid command
 *
 * @var acmdrv_bypass_dma_s_cmd::ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE
 * @brief Move Data Without Timestamp
 *
 * @var acmdrv_bypass_dma_s_cmd::ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE_WITH_TS
 * @brief Move Data with Timestamp
 */
enum acmdrv_bypass_dma_s_cmd {
	ACMDRV_BYPASS_DMA_CMD_S_COMMAND_INVALID = 0,
	ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE = 1,
	ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE_WITH_TS = 2
};

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_IRQ
 * @brief Interrupt Flag
 *
 * If this bit is set, each time the command is executed and the Scatter write
 * to the Message Buffer is finished, the Message Buffer IRQ Status register
 * will raise a corresponding interrupt
 */
#define ACMDRV_BYPASS_DMA_CMD_S_IRQ	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_IRQ_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS
 * @brief Frame Buffer Offset
 */
#define ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_LENGTH
 * @brief Length
 *
 * Length of the data to move.(min is 4 Bytes, maximum 1528 if with R-Tag)
 *
 * Special case:
 * * 0 - NOP
 * * 0x7FF - Read whole buffer
 */
#define ACMDRV_BYPASS_DMA_CMD_S_LENGTH	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_LENGTH_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID
 * @brief RX Message buffer ID
 */
#define ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID_BIT)

/**
 * @brief Helper to create Scatter DMA Invalid Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_s_invalid_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_S_COMMAND_INVALID);
}

/**
 * @brief Helper to create Scatter DMA Move Command
 *
 * @param last Last flag
 * @param irq Interrupt flag
 * @param frm_buff_offs Frame Buffer Offset
 * @param length Length
 * @param rx_msgbuf_id RX message buffer id
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_s_move_create(
	bool last,
	bool irq,
	uint16_t frm_buff_offs,
	int16_t length,
	uint8_t rx_msgbuf_id)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_S_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_IRQ_BIT, irq) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS_BIT, frm_buff_offs) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID_BIT, rx_msgbuf_id);
}

/**
 * @brief Helper to create Scatter DMA Move With Timestamp Command
 *
 * @param last Last flag
 * @param irq Interrupt flag
 * @param frm_buff_offs Frame Buffer Offset
 * @param length Length
 * @param rx_msgbuf_id RX message buffer id
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_s_move_with_timestamp_create(
	bool last,
	bool irq,
	uint16_t frm_buff_offs,
	uint16_t length,
	uint8_t rx_msgbuf_id)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_S_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_S_COMMAND_MOVE_WITH_TS) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_IRQ_BIT, irq) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_FRM_BUFF_OFFS_BIT, frm_buff_offs) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_S_RX_MSGBUF_ID_BIT, rx_msgbuf_id);
}
/** @} acmdrv_bypass_s_dma_cmds_details */

/**
 * @name bit structure for prefetch dma commands
 * @{
 */
#define ACMDRV_BYPASS_DMA_CMD_P_LAST_BIT_L		0
#define ACMDRV_BYPASS_DMA_CMD_P_LAST_BIT_H		0
#define ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT_L		1
#define ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT_H		2
#define ACMDRV_BYPASS_DMA_CMD_P_IRQ_BIT_L		4
#define ACMDRV_BYPASS_DMA_CMD_P_IRQ_BIT_H		4
#define ACMDRV_BYPASS_DMA_CMD_P_LENGTH_BIT_L		5
#define ACMDRV_BYPASS_DMA_CMD_P_LENGTH_BIT_H		15
#define ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP_BIT_L		8
#define ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP_BIT_H		9
#define ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK_BIT_L		12
#define ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK_BIT_H		12
#define ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID_BIT_L		16
#define ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID_BIT_H		20
#define ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR_BIT_L	16
#define ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR_BIT_H	31
/** @} */

/**
 * @name Details of ACM bypass Prefetch DMA Commands
 * @anchor acmdrv_bypass_p_dma_cmds_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_LAST
 * @brief Last
 *
 * Last indicates whether the command is last command in the chain
 * * 0 - More commands follow
 * * 1 - Last (it will also drop the frame from the frame buffer)
 */
#define ACMDRV_BYPASS_DMA_CMD_P_LAST	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_LAST_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_COMMAND
 * @brief Command Type
 */
#define ACMDRV_BYPASS_DMA_CMD_P_COMMAND	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT)

/**
 * @enum acmdrv_bypass_dma_p_cmd
 * @brief ACM Bypass Prefetch DMA Command Types
 *
 * @var acmdrv_bypass_dma_p_cmd::ACMDRV_BYPASS_DMA_CMD_P_COMMAND_INVALID
 * @brief Invalid command
 *
 * @var acmdrv_bypass_dma_p_cmd::ACMDRV_BYPASS_DMA_CMD_P_COMMAND_NOP
 * @brief No Operation (NOP) command
 *
 * @var acmdrv_bypass_dma_p_cmd::ACMDRV_BYPASS_DMA_CMD_P_COMMAND_MOV_MSG_BUFF
 * @brief Move Data from Message Buffer to Prefetch FIFO
 *
 * @var acmdrv_bypass_dma_p_cmd::ACMDRV_BYPASS_DMA_CMD_P_COMMAND_LOCK_MSG_BUFF
 * @brief Lock Message Buffers (Multilock)
 */
enum acmdrv_bypass_dma_p_cmd {
	ACMDRV_BYPASS_DMA_CMD_P_COMMAND_INVALID = 0,
	ACMDRV_BYPASS_DMA_CMD_P_COMMAND_NOP = 1,
	ACMDRV_BYPASS_DMA_CMD_P_COMMAND_MOV_MSG_BUFF = 2,
	ACMDRV_BYPASS_DMA_CMD_P_COMMAND_LOCK_MSG_BUFF = 3
};

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_IRQ
 * @brief Interrupt
 *
 * If this bit is set, each time the command is executed and the Prefetch read
 * from the Message Buffer is finished, the Message Buffer IRQ Status register
 * will raise a corresponding interrupt
 */
#define ACMDRV_BYPASS_DMA_CMD_P_IRQ	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_IRQ_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_LENGTH
 * @brief Length
 *
 * Length of the data to move. Minimum data to be read is 3 Bytes.
 */
#define ACMDRV_BYPASS_DMA_CMD_P_LENGTH	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_LENGTH_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID
 * @brief Message buffer ID
 */
#define ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP
 * @brief MB Group helper macros
 *
 * If this bit is set to 0, the Lock Vector field operates on message
 * buffers 0 to 15; if this bit is set 1, the Lock Vector field operates on
 * message buffers 16 to 31.
 */
#define ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK
 * @brief Dual Lock
 *
 * All the buffers with asserted bit in the Lock Vector will be Dual-Locked,
 * i. e., the buffers have to be unlocked twice to become unlocked. This is used
 * during redundancy sequence generation to ensure consistency of both redundant
 * copies. If a second DualLock is issued to buffers that have already been
 * Dual-Locked, the lock will be ignored and finally the buffers will be
 * unlocked when access is finished. This guarantees that both Gather DMAs will
 * transmit exactly the same frame (even if their transmits don’t overlap).
 */
#define ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR
 * @brief Lock Vector
 *
 * Each bit represents a lock control for one of the targeted 16 Message
 * Buffers. In case the MB group is set to 0, bits 16 to 31 control locking of
 * buffers 0 to 15; if MB bit is set to 1, bits 16 to 31 control locking of
 * buffers 16 to 31.
 */
#define ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR_BIT)

/**
 * @brief Helper to create Prefetch DMA Invalid Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_p_invalid_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_P_COMMAND_INVALID);
}

/**
 * @brief Helper to create Prefetch DMA NOP Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_p_nop_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_P_COMMAND_NOP);
}

/**
 * @brief Helper to create Prefetch DMA Move Message Buffer Command
 *
 * @param last Last flag
 * @param irq Interrupt flag
 * @param length Length
 * @param msg_buf_id Message buffer id
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_p_mov_msg_buff_create(
		bool last,
		bool irq,
		uint16_t length,
		uint8_t msg_buf_id)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_P_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_P_COMMAND_MOV_MSG_BUFF) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_IRQ_BIT, irq) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_MSGBUF_ID_BIT, msg_buf_id);

}

/**
 * @brief Helper to create Prefetch DMA Lock Message Buffer Command
 *
 * @param mb_group Message Buffer group
 * @param dual_lock Dual lock flag
 * @param lock_vector Message Buffer lock vector
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_p_lock_msg_buff_create(
		uint8_t mb_group,
		bool dual_lock,
		uint16_t lock_vector)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_P_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_P_COMMAND_LOCK_MSG_BUFF) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_MB_GROUP_BIT, mb_group) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_DUAL_LOCK_BIT, dual_lock) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_P_LOCK_VECTOR_BIT, lock_vector);
}
/** @} acmdrv_bypass_p_dma_cmds_details */

/**
 * @name bit structure for gather dma commands
 * @{
 */
#define ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT_L		0
#define ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT_H		0
#define ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT_L		1
#define ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT_H		3
#define ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH_BIT_L	5
#define ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH_BIT_H	15
#define ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT_L		5
#define ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT_H		15
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET_BIT_L	16
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET_BIT_H	27
#define ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET_BIT_L	16
#define ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET_BIT_H	27
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA_BIT_L	16
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA_BIT_H	23
/** @} */

/**
 * @name Details of ACM bypass Gather DMA Commands
 * @anchor acmdrv_bypass_g_dma_cmds_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_LAST
 * @brief Last
 *
 * Last indicates whether the command is last command in the chain
 * * 0 - More commands follow
 * * 1 - Last (it will also drop the frame from the frame buffer)
 */
#define ACMDRV_BYPASS_DMA_CMD_G_LAST	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_COMMAND
 * @brief Command Type
 */
#define ACMDRV_BYPASS_DMA_CMD_G_COMMAND	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT)

/**
 * @enum acmdrv_bypass_dma_g_cmd
 * @brief ACM Bypass Gather DMA Command Types
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INVALID
 * @brief Invalid command
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_FORWARD
 * @brief Forward Complete Frame
 *
 * The complete frame stored in the Frame Buffer is forwarded without changes
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_PREF
 * @brief Move Data from Prefetch FIFO to TX MAC
 *
 * One complete message is read from the Prefetch FIFO
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INSERT_DELAY
 * @brief Insert delay
 *
 * Insertion of an additional delay before the Execution DMA chain processing
 * starts.
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_CNST_BUFF
 * @brief Move Data from Constant Buffer to TX MAC
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_FR_BUFF
 * @brief Move Data from Frame Buffer to TX MAC
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_CONST_BYTE
 * @brief Insert constant byte data
 *
 * @var acmdrv_bypass_dma_g_cmd::ACMDRV_BYPASS_DMA_CMD_G_COMMAND_R_TAG
 * @brief R-Tag to TX MAC
 *
 * R-Tag provided by the ACM Redundancy module is moved to the TX MAC
 */
enum acmdrv_bypass_dma_g_cmd {
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INVALID = 0,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_FORWARD = 1,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_PREF = 2,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INSERT_DELAY = 3,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_CNST_BUFF = 4,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_FR_BUFF = 5,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_CONST_BYTE = 6,
	ACMDRV_BYPASS_DMA_CMD_G_COMMAND_R_TAG = 7
};

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH
 * @brief Delay Length
 *
 * The delay to be inserted before the Gather DMA processes the rest of the
 * Execution DMA chain. This delay is added to the delay defined in the Static
 * Gather Delay (STATIC_GATHER_DELAY) register.
 */
#define ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_LENGTH
 * @brief Length
 *
 * Length of the data to be read from the Constant Buffer
 */
#define ACMDRV_BYPASS_DMA_CMD_G_LENGTH	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET
 * @brief Constant Buffer Offset
 *
 * Byte offset in the constant buffer from where the data reading starts
 */
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET
 * @brief Frame Buffer Offset
 *
 * Offset in Frame Buffer
 */
#define ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET_BIT)

/**
 * @def ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA
 * @brief Constant Byte Data
 */
#define ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA	\
	BITMASK(ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA_BIT)

/**
 * @brief Helper to create Gather DMA Invalid Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_invalid_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INVALID);
}

/**
 * @brief Helper to create Gather DMA Forward Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_forward_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_FORWARD);
}

/**
 * @brief Helper to create Gather DMA Move Pref Command
 *
 * @param last Last flag
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_move_pref_create(bool last)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_PREF);
}

/**
 * @brief Helper to create Gather DMA Insert Delay Command
 *
 * @param delay Delay
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline
uint32_t acmdrv_bypass_dma_cmd_g_insert_delay_create(uint16_t delay)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_INSERT_DELAY) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_DELAY_LENGTH_BIT, delay);
}

/**
 * @brief Helper to create Gather DMA Lock Move Constant Buffer Command
 *
 * @param last Last flag
 * @param length Length
 * @param const_buff_offset Constant Buffer Offset
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_move_cnst_buff_create(
	bool last,
	uint16_t length,
	uint16_t const_buff_offset)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_CNST_BUFF) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_CONST_BUFF_OFFSET_BIT,
			const_buff_offset);
}

/**
 * @brief Helper to create Gather DMA Lock Move Frame Buffer Command
 *
 * @param last Last flag
 * @param length Length
 * @param fr_buff_offset Frame Buffer Offset
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_move_fr_buff_create(
	bool last,
	uint16_t length,
	uint16_t fr_buff_offset)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_MOVE_FR_BUFF) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_FR_BUFF_OFFSET_BIT, fr_buff_offset);
}

/**
 * @brief Helper to create Gather DMA Lock Const Byte Command
 *
 * @param last Last flag
 * @param length Length
 * @param const_byte_data Byte Data
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_const_byte_create(
	bool last,
	uint16_t length,
	uint8_t const_byte_data)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_LAST_BIT, last) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_CONST_BYTE) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_LENGTH_BIT, length) |
	       WVAL(ACMDRV_BYPASS_DMA_CMD_G_CONST_BYTE_DATA_BIT,
			const_byte_data);
}

/**
 * @brief Helper to create Gather DMA R-Tag Command
 *
 * @return value for acmdrv_bypass_dma_command.cmd
 */
static inline uint32_t acmdrv_bypass_dma_cmd_g_r_tag_create(void)
{
	return WVAL(ACMDRV_BYPASS_DMA_CMD_G_COMMAND_BIT,
			ACMDRV_BYPASS_DMA_CMD_G_COMMAND_R_TAG);
}
/** @} acmdrv_bypass_g_dma_cmds_details */

/**
 * @brief Size of Constant Data buffer
 */
#define ACMDRV_BYBASS_CONST_BUFFER_SIZE	0x1000

/**
 * @brief access structure for const_buffer interface
 */
struct acmdrv_bypass_const_buffer {
	uint8_t data[ACMDRV_BYBASS_CONST_BUFFER_SIZE]; /**< const buffer data */
} __packed;

/**
 * @brief number of redundancy control tables
 */
#define ACMDRV_REDUN_CTRLTAB_COUNT	2
/**
 * @brief number of redundancy table entries
 */
#define ACMDRV_REDUN_TABLE_ENTRY_COUNT	32

/**
 * @struct acmdrv_redun_ctrl_entry
 * @brief redundancy control entry data representation
 *
 * @var acmdrv_redun_ctrl_entry::ctrl
 * @brief control data
 *
 * see @ref acmdrv_redun_ctrl_details "Details of ACM Redundancy Control"
 */
struct acmdrv_redun_ctrl_entry {
	uint32_t ctrl;
} __packed;

/**
 * @name bit structure for acmdrv_redun_ctrl_entry.ctrl
 * @{
 */
#define ACMDRV_REDUN_CTLTAB_SRC_BIT_L			0
#define ACMDRV_REDUN_CTLTAB_SRC_BIT_H			2
#define ACMDRV_REDUN_CTLTAB_UPD_BIT_L			8
#define ACMDRV_REDUN_CTLTAB_UPD_BIT_H			9
#define ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG_BIT_L		14
#define ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG_BIT_H		14
#define ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX_BIT_L	16
#define ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX_BIT_H	20
/** @} */

/**
 * @name Details of ACM Redundancy Control
 * @anchor acmdrv_redun_ctrl_details
 * @{
 */

/**
 * @def ACMDRV_REDUN_CTRLTAB_SOURCE
 * €brief Source
 *
 * Sets the source of the TxSeqNum for the Gather DMA. The options are the
 * following:
 * * 0 = RxSeqNum
 * * 1 = MAX(RxSeqNum, IntSeqNum)
 * * 2 = IntSeqNum
 * * 3, 4, 5 and 6 - Reserved
 * * 7 = Zeros
 */
#define ACMDRV_REDUN_CTRLTAB_SOURCE	\
	BITMASK(ACMDRV_REDUN_CTLTAB_SRC_BIT)

/**
 * @enum acmdrv_redun_ctrltab_source
 * @brief Redundance Control Table sources of TxSeqNum for the Gather DMA
 *
 * @var acmdrv_redun_ctrltab_source::ACMDRV_REDUN_CTLTAB_SRC_RXSEQNUM
 * @brief RxSeqNum
 *
 * (note: read immediately, not after FCS is checked). This option is ignored
 * (option ACMDRV_REDUN_CTLTAB_SRC_ZEROS is used instead), if the Seamless
 * Redundancy module was triggered by the schedule (and not by the received
 * frame).
 *
 * @var acmdrv_redun_ctrltab_source::ACMDRV_REDUN_CTLTAB_SRC_MAXNUM
 * @brief MAX(RxSeqNum, IntSeqNum)
 *
 * Gather DMA uses the higher sequence number from the two (note: the RxSeqNum
 * is evaluated immediately after it is received, not after FCS check is
 * complete)
 *
 * @var acmdrv_redun_ctrltab_source::ACMDRV_REDUN_CTLTAB_SRC_INTSEQNUM
 * @brief IntSeqNum
 *
 * Gather DMA uses the Sequence number stored in the IntSeqNum table entry
 *
 * @var acmdrv_redun_ctrltab_source::ACMDRV_REDUN_CTLTAB_SRC_ZEROS
 * @brief Zeros
 *
 * Value 0x0000 is used for TxSeqNum.
 */
enum acmdrv_redun_ctrltab_source {
	ACMDRV_REDUN_CTLTAB_SRC_RXSEQNUM = 0,
	ACMDRV_REDUN_CTLTAB_SRC_MAXNUM = 1,
	ACMDRV_REDUN_CTLTAB_SRC_INTSEQNUM = 2,
	ACMDRV_REDUN_CTLTAB_SRC_ZEROS = 7,
};

/**
 * @def ACMDRV_REDUN_CTRLTAB_UPDATE
 * @brief Update
 *
 * Instructs the Seamless redundancy unit to update the corresponding IntSeqNum
 * data. The options are the following:
 * * 0 = NOP
 * * 1 = MAX (RxSeqNum, IntSeqNum)
 * * 2 = Increment on finished generation of frame
 * * 3 = Increment on finished generation of both frames
 */
#define ACMDRV_REDUN_CTRLTAB_UPDATE	\
	BITMASK(ACMDRV_REDUN_CTLTAB_UPD_BIT)

/**
 * @enum acmdrv_redun_ctrltab_update
 * @brief Redundancy Control Table Sequence Number Update Modes
 *
 * @var acmdrv_redun_ctrltab_update::ACMDRV_REDUN_CTLTAB_UPD_NOP
 * @brief No Operation
 *
 * No upodate of IntSeqNum
 *
 * @var acmdrv_redun_ctrltab_update::ACMDRV_REDUN_CTLTAB_UPD_MAXNUM
 * @brief MAX(RxSeqNum, IntSeqNum)
 *
 * Pick the higher value from the two sequence numbers if a frame was received
 * and its FCS check passed.
 *
 * @var acmdrv_redun_ctrltab_update::ACMDRV_REDUN_CTLTAB_UPD_FIN
 * @brief Increment on finished generation of frame
 *
 * Increment when the corresponding Gather DMA finished generation of a frame.
 *
 * @var acmdrv_redun_ctrltab_update::ACMDRV_REDUN_CTLTAB_UPD_FIN_BOTH
 * @brief Increment on finished generation of both frames
 *
 * Increment when both Gather DMAs finished generation of 2 redundant replicas.
 */
enum acmdrv_redun_ctrltab_update {
	ACMDRV_REDUN_CTLTAB_UPD_NOP = 0,
	ACMDRV_REDUN_CTLTAB_UPD_MAXNUM = 1,
	ACMDRV_REDUN_CTLTAB_UPD_FIN = 2,
	ACMDRV_REDUN_CTLTAB_UPD_FIN_BOTH = 3,
};
/**
 * @def ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG
 * @brief Drop No R-tag
 *
 * If this bit is set, it instructs the Gather DMA and Scatter DMA to stop
 * processing a frame and drop it in case that it does not contain an R-Tag. If
 * this bit is cleared, Gather DMA and Scatter DMA will process the frame
 * normally ignoring the fact that the R-Tag is not present in the frame. This
 * bit is ignored if the processing was triggered by the ACM Schedule.
 */
#define ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG	\
	BITMASK(ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG_BIT)

/**
 * @def ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX
 * @brief IntSeqNum/Status Index
 *
 * Index to the IntSeqNum and Status tables
 */
#define ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX	\
	BITMASK(ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX_BIT)

/**
 * @brief Helper to create Redundancy Control Table Entry
 *
 * @param source Source
 * @param update Update mode
 * @param drop_no_rtag Drop No R-Tag flag
 * @param int_seq_idx Index to the IntSeqNum table
 * @return value for acmdrv_redun_ctrl_entry.ctrl
 */
static inline uint32_t acmdrv_redun_ctrltab_entry_create(
	enum acmdrv_redun_ctrltab_source source,
	enum acmdrv_redun_ctrltab_update update,
	bool drop_no_rtag,
	uint8_t int_seq_idx)
{
	return WVAL(ACMDRV_REDUN_CTLTAB_SRC_BIT, source) |
	       WVAL(ACMDRV_REDUN_CTLTAB_UPD_BIT, update) |
	       WVAL(ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG_BIT, drop_no_rtag) |
	       WVAL(ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX_BIT,
		int_seq_idx);
}

/**
 * @brief Helper to read Redundancy Control Table Entry's source
 *
 * @param entry Pointer to acmdrv_redun_ctrl_entry
 * @return Source
 */
static inline enum acmdrv_redun_ctrltab_source
acmdrv_redun_ctrltab_entry_source_read(
	const struct acmdrv_redun_ctrl_entry *entry)
{
	return RVAL(ACMDRV_REDUN_CTLTAB_SRC_BIT, entry->ctrl);
}

/**
 * @brief Helper to read Redundancy Control Table Entry's update mode
 *
 * @param entry Pointer to acmdrv_redun_ctrl_entry
 * @return Update mode
 */
static inline enum acmdrv_redun_ctrltab_update
acmdrv_redun_ctrltab_entry_update_read(
	const struct acmdrv_redun_ctrl_entry *entry)
{
	return RVAL(ACMDRV_REDUN_CTLTAB_UPD_BIT, entry->ctrl);
}

/**
 * @brief Helper to read Redundancy Control Table Entry's drop no r-tag flag
 *
 * @param entry Pointer to acmdrv_redun_ctrl_entry
 * @return Drop no r-tag flag
 */
static inline bool acmdrv_redun_ctrltab_entry_drop_no_rtag_read(
	const struct acmdrv_redun_ctrl_entry *entry)
{
	return RVAL(ACMDRV_REDUN_CTLTAB_DROP_NO_RTAG_BIT, entry->ctrl);
}

/**
 * @brief Helper to read Redundancy Control Table Entry's Index IntSeqNum table
 *
 * @param entry Pointer to acmdrv_redun_ctrl_entry
 * @return Index of IntSeqNum table
 */
static inline uint8_t acmdrv_redun_ctrltab_entry_int_seq_idx_read(
	const struct acmdrv_redun_ctrl_entry *entry)
{
	return RVAL(ACMDRV_REDUN_CTLTAB_SEQ_NUM_STATUS_IDX_BIT,
		entry->ctrl);
}
/** @} acmdrv_redun_ctrl_details */

 /**
  * @struct acmdrv_redun_status
  * @brief redundancy status entry representation
  *
  * @var acmdrv_redun_status::status
  * @brief status data
  *
  * see @ref acmdrv_redun_status_details "Details of ACM Redundancy Status"
  */
struct acmdrv_redun_status {
	uint32_t status;
} __packed;

/**
 * @name bit structure for acmdrv_redun_status.status
 * @{
 */
#define ACMDRV_REDUN_STATUS_INT_SEQ_NUM_BIT_L	0
#define ACMDRV_REDUN_STATUS_INT_SEQ_NUM_BIT_H	15
#define ACMDRV_REDUN_STATUS_RR_BIT_L		16
#define ACMDRV_REDUN_STATUS_RR_BIT_H		16
#define ACMDRV_REDUN_STATUS_RT_BIT_L		17
#define ACMDRV_REDUN_STATUS_RT_BIT_H		17
#define ACMDRV_REDUN_STATUS_PR_BIT_L		18
#define ACMDRV_REDUN_STATUS_PR_BIT_H		18
#define ACMDRV_REDUN_STATUS_OLD_STATUS_0_BIT_L	20
#define ACMDRV_REDUN_STATUS_OLD_STATUS_0_BIT_H	21
#define ACMDRV_REDUN_STATUS_OLD_STATUS_1_BIT_L	24
#define ACMDRV_REDUN_STATUS_OLD_STATUS_1_BIT_H	25
/** @} */

/**
 * @name Details of ACM Redundancy Status
 * @anchor acmdrv_redun_status_details
 * @{
 */

/**
 * @def ACMDRV_REDUN_STATUS_INT_SEQ_NUM
 * @brief Internal Sequence Number
 *
 * This value is mirrored from the corresponding IntSeqNum entry to allow
 * consistent status acquisition together with other flags
 */
#define ACMDRV_REDUN_STATUS_INT_SEQ_NUM	\
	BITMASK(ACMDRV_REDUN_STATUS_INT_SEQ_NUM_BIT)

/**
 * @def ACMDRV_REDUN_STATUS_RR
 * @brief Replica Received
 *
 * Indicates that the recovery of the 1st replica was finished without errors.
 * This bit is set only when Update field of the Control table entry is set
 * to 1, no matter if the MAX(RxSeqNum, IntSeqNum) function resulted in
 * IntSeqNum modification or the original value was kept.
 */
#define ACMDRV_REDUN_STATUS_RR	BITMASK(ACMDRV_REDUN_STATUS_RR_BIT)

/**
 * @def ACMDRV_REDUN_STATUS_RT
 * @brief Replica Transmitted
 *
 * Indicates that the frame replica was transmitted. This bit is user/valid only
 * when Update field of the Control table entry is set to 3 (Increment when both
 * replicas are transmitted). In such case, the field is set to 1 when the first
 * frame replica is transmitted and then cleared back to 0 when the second frame
 * replica is transmitted.
 */
#define ACMDRV_REDUN_STATUS_RT	BITMASK(ACMDRV_REDUN_STATUS_RT_BIT)

/**
 * @def ACMDRV_REDUN_STATUS_PR
 * @brief Parallel Recovery
 *
 * Indicates that both replicas are being received in parallel and no replica
 * could be eliminated until at least one is received completely and FCS check
 * is successfully performed. This bit is valid/used when the Recovery bit of
 * the Control table entry is set to 1.
 */
#define ACMDRV_REDUN_STATUS_PR	BITMASK(ACMDRV_REDUN_STATUS_PR_BIT)

/**
 * @enum acmdrv_redun_status_old_status
 * @brief ACM Redundancy Status old state enumeration
 *
 * @var acmdrv_redun_status_old_status::ACMDRV_REDUN_OSTAT_NO_FRM
 * @brief No frame replica received
 *
 * @var acmdrv_redun_status_old_status::ACMDRV_REDUN_OSTAT_FRM_RCVD
 * @brief Frame received successfully
 *
 * @var acmdrv_redun_status_old_status::ACMDRV_REDUN_OSTAT_FRM_RCVD_SKIP
 * @brief Frame received successfully but discarded
 *
 * Other replica was received first.
 *
 * @var acmdrv_redun_status_old_status::ACMDRV_REDUN_OSTAT_FRM_RCVD_ERR
 * @brief Frame received with errors and discarded
 */
enum acmdrv_redun_status_old_status {
	ACMDRV_REDUN_OSTAT_NO_FRM = 0,
	ACMDRV_REDUN_OSTAT_FRM_RCVD = 1,
	ACMDRV_REDUN_OSTAT_FRM_RCVD_SKIP = 2,
	ACMDRV_REDUN_OSTAT_FRM_RCVD_ERR = 3,
};

/**
 * @def ACMDRV_REDUN_STATUS_OLD_STATUS_0
 * @brief Old Status 0
 *
 * This field contains the status of the previously received frame replica by
 * the ACM Bypass 0.
 */
#define ACMDRV_REDUN_STATUS_OLD_STATUS_0	\
	BITMASK(ACMDRV_REDUN_STATUS_OLD_STATUS_0_BIT)

/**
 * @def ACMDRV_REDUN_STATUS_OLD_STATUS_1
 * @brief Old Status 1
 *
 * This field contains the status of the previously received frame replica by
 * the ACM Bypass 1.
 */
#define ACMDRV_REDUN_STATUS_OLD_STATUS_1	\
	BITMASK(ACMDRV_REDUN_STATUS_OLD_STATUS_1_BIT)

/**
 * @brief Helper to read Redundancy Status Entry's Internal Sequence Number
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Internal Sequence Number
 */
static inline uint16_t acmdrv_redun_status_int_seq_num_read(
	const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_INT_SEQ_NUM_BIT, st->status);
}

/**
 * @brief Helper to read Redundancy Status Entry's Replica Received flag
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Replica Received flag
 */
static inline bool acmdrv_redun_status_rr_read(
	const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_RR_BIT, st->status);
}

/**
 * @brief Helper to read Redundancy Status Entry's Replica Transmitted flag
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Replica Transmitted flag
 */
static inline bool acmdrv_redun_status_rt_read(
	const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_RT_BIT, st->status);
}

/**
 * @brief Helper to read Redundancy Status Entry's Parallel Recovery flag
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Parallel Recovery flag
 */
static inline bool acmdrv_redun_status_pr_read(
	const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_PR_BIT, st->status);
}

/**
 * @brief Helper to read Redundancy Status Entry's Redundancy Status BP0
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Redundancy Status BP0
 */
static inline enum acmdrv_redun_status_old_status
acmdrv_redun_status_old_status_0_read(const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_OLD_STATUS_0_BIT, st->status);
}

/**
 * @brief Helper to read Redundancy Status Entry's Redundancy Status BP1
 *
 * @param st Pointer to acmdrv_redun_status
 * @return Redundancy Status BP1
 */
static inline enum acmdrv_redun_status_old_status
acmdrv_redun_status_old_status_1_read(const struct acmdrv_redun_status *st)
{
	return RVAL(ACMDRV_REDUN_STATUS_OLD_STATUS_1_BIT, st->status);
}
/** @} acmdrv_redun_status_details */

/**
 * @struct acmdrv_redun_intseqnum
 * @brief Redundancy internal sequence number representation
 *
 * @var acmdrv_redun_intseqnum::intseqnum
 * @brief Internal Sequence Number
 *
 * see @ref acmdrv_redun_intseqnum_details "Details of IntSeqNum"
 */
struct acmdrv_redun_intseqnum {
	uint32_t intseqnum;
} __packed;

/**
 * @name bit structure for acmdrv_redun_status.status
 * @{
 */
#define ACMDRV_REDUN_INTSEQNUM_BIT_L	0
#define ACMDRV_REDUN_INTSEQNUM_BIT_H	15
/** @} */

/**
 * @name Details of ACM Redundancy Internal Sequence Number
 * @anchor acmdrv_redun_intseqnum_details
 * @{
 */

/**
 * @def ACMDRV_REDUN_INTSEQNUM
 * @brief Internal Sequence Number
 *
 * This field holds the value of the Internal Sequence Number. On read, the
 * value of this field is provided. On write, the value of this field is
 * overwritten by the provided data
 */
#define ACMDRV_REDUN_INTSEQNUM	BITMASK(ACMDRV_REDUN_INTSEQNUM_BIT)

/**
 * @brief Helper to create Internal Sequence Number
 *
 * @param seqnum Sequence number to transform
 * @return value for acmdrv_redun_intseqnum.intseqnum
 */
static inline uint32_t acmdrv_redun_intseqnum_create(uint16_t seqnum)
{
	return WVAL(ACMDRV_REDUN_INTSEQNUM_BIT, seqnum);
}
/** @} acmdrv_redun_intseqnum_details */

/**
 * @struct acmdrv_redun_individual_recovery_module
 * @brief binary interface structure for controlling individual recovery
 *        timeouts of a single module
 *
 * @var acmdrv_redun_individual_recovery_module::timeout
 * @brief timeout value for individual recovery of a stream in milliseconds
 */
struct acmdrv_redun_individual_recovery_module {
	uint32_t timeout[ACMDRV_BYPASS_NR_RULES];
} __packed;

/**
 * @struct acmdrv_redun_individual_recovery
 * @brief binary interface structure for controlling individual recovery
 *        timeouts
 *
 * @var acmdrv_redun_individual_recovery::module
 * @brief individual recovery of a module
 */
struct acmdrv_redun_individual_recovery {
	struct acmdrv_redun_individual_recovery_module
		module[ACMDRV_BYPASS_MODULES_COUNT];
} __packed;

/**
 * @struct acmdrv_redun_base_recovery
 * @brief binary interface structure for controlling base recovery
 *        timeouts
 *
 * @var acmdrv_redun_base_recovery::timeout
 * @brief timeout value for base recovery in milliseconds
 */
struct acmdrv_redun_base_recovery {
	uint32_t timeout[ACMDRV_REDUN_TABLE_ENTRY_COUNT];
} __packed;

/**
 * @brief number of schedulers
 */
#define ACMDRV_SCHEDULER_COUNT	2

/**
 * @struct acmdrv_scheduler_down_counter
 * @brief data structure for sched_down_counter interface
 *
 * @var acmdrv_scheduler_down_counter::counter
 * @brief counter value
 *
 * see @ref acmdrv_scheduler_down_counter_details "Details of Sched Down
 * Counter"
 */
struct acmdrv_scheduler_down_counter {
	uint16_t counter;
} __packed;

/**
 * @name bit structure for acmdrv_scheduler_down_counter.counter
 * @{
 */
#define ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV_BIT_L	0
#define ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV_BIT_H	1
#define ACMDRV_SCHEDULER_DOWNCNT_START_VAL_BIT_L	4
#define ACMDRV_SCHEDULER_DOWNCNT_START_VAL_BIT_H	14
/** @} */

/**
 * @name Details of ACM Scheduler Down Counter
 * @anchor acmdrv_scheduler_down_counter_details
 * @{
 */

/**
 * @def ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV
 * @brief Downcounter Speed by 10/100 divider
 *
 * Downcounter speed, by 10/100 divider
 * * '0' = one addend (see register DWNCNTR_SPD) in a clock cycle
 * * '1' = one addend in ten clock cycles
 * * '2' = one addend in one hundred clock cycles
 * * '3' = reserved
 *
 * When FSC is used with FES/FRS/FDS this setting corresponds to 1Gb/s,
 * 100Mbit/s and 10Mbit/s line speeds.
 */
#define ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV	\
	BITMASK(ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV_BIT)

/**
 * @enum acmdrv_scheduler_downcnt_speed
 * @brief Scheduler Down Counter Speed Select
 *
 * @var acmdrv_scheduler_downcnt_speed::ACMDRV_SCHEDULER_DOWNCNT_SPEED_1000
 * @brief 1Gbps
 *
 * @var acmdrv_scheduler_downcnt_speed::ACMDRV_SCHEDULER_DOWNCNT_SPEED_100
 * @brief 100Mbps
 *
 * @var acmdrv_scheduler_downcnt_speed::ACMDRV_SCHEDULER_DOWNCNT_SPEED_10
 * @brief 10Mbps
 */
enum acmdrv_scheduler_downcnt_speed {
	ACMDRV_SCHEDULER_DOWNCNT_SPEED_1000 = 0,
	ACMDRV_SCHEDULER_DOWNCNT_SPEED_100 = 1,
	ACMDRV_SCHEDULER_DOWNCNT_SPEED_10 = 2
};

/**
 * @def ACMDRV_SCHEDULER_DOWNCNT_START_VAL
 * @brief Downcounter Start Value
 *
 * Downcounter start value. The value from which the Downcounters count down to
 * zero.
 */
#define ACMDRV_SCHEDULER_DOWNCNT_START_VAL	\
	BITMASK(ACMDRV_SCHEDULER_DOWNCNT_START_VAL_BIT)

/**
 * @brief Create acmdrv_scheduler_down_counter.counter from counter value
 *
 * @param speed Speed select
 * @param start Start value for down counter
 */
static inline uint16_t acmdrv_scheduler_down_counter_create(
		enum acmdrv_scheduler_downcnt_speed speed, uint16_t start)
{
	return WVAL(ACMDRV_SCHEDULER_DOWNCNT_SPEED_DIV_BIT, speed) |
	       WVAL(ACMDRV_SCHEDULER_DOWNCNT_START_VAL_BIT, start);
}
/** @} acmdrv_scheduler_down_counter_details */

/**
 * @brief number of scheduler tables per scheduler
 */
#define ACMDRV_SCHED_TBL_COUNT		2

/**
 * @brief number of table rows per scheduler table
 */
#define ACMDRV_SCHED_TBL_ROW_COUNT	1024

/**
 * @struct acmdrv_sched_tbl_row
 * @brief data representation for sysfs sched_tab_row entry
 *
 * @var acmdrv_sched_tbl_row::cmd
 * @brief row command
 *
 * @var acmdrv_sched_tbl_row::delta_cycle
 * @brief delta cycle
 *
 * @var acmdrv_sched_tbl_row::padding
 * @brief data padding to 64 bit only
 *
 * see @ref acmdrv_sched_tbl_row_details "Details of Sched Table Row
 * Entry"
 */
struct acmdrv_sched_tbl_row {
	uint32_t cmd;		/**< row command */
	uint16_t delta_cycle;	/**< delta cycle */
	uint16_t padding;	/**< data padding only */
} __packed;

/**
 * @name bit structure for acmdrv_sched_tbl_row.cmd
 * @{
 */
#define ACMDRV_SCHED_TBL_CMD_DMA_INDEX_BIT_L		0
#define ACMDRV_SCHED_TBL_CMD_DMA_INDEX_BIT_H		8
#define ACMDRV_SCHED_TBL_CMD_RULE_ID_BIT_L		14
#define ACMDRV_SCHED_TBL_CMD_RULE_ID_BIT_H		18
#define ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID_BIT_L	19
#define ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID_BIT_H	23
#define ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER_BIT_L		25
#define ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER_BIT_H		26
#define ACMDRV_SCHED_TBL_CMD_WIN_CLOSE_BIT_L		27
#define ACMDRV_SCHED_TBL_CMD_WIN_CLOSE_BIT_H		27
#define ACMDRV_SCHED_TBL_CMD_WIN_OPEN_BIT_L		28
#define ACMDRV_SCHED_TBL_CMD_WIN_OPEN_BIT_H		28
#define ACMDRV_SCHED_TBL_CMD_NGN_DISABLE_BIT_L		29
#define ACMDRV_SCHED_TBL_CMD_NGN_DISABLE_BIT_H		29
#define ACMDRV_SCHED_TBL_CMD_NGN_ENABLE_BIT_L		30
#define ACMDRV_SCHED_TBL_CMD_NGN_ENABLE_BIT_H		30
/** @} */

/**
 * @name Details of ACM Scheduler Table Row Entry
 * @anchor acmdrv_sched_tbl_row_details
 * @{
 */

/**
 * @def ACMDRV_SCHED_TBL_CMD_DMA_INDEX
 * @brief DMA List/Command Index
 *
 * Offset in the Gather DMA pointing to the time-triggered DMA chain.
 */
#define ACMDRV_SCHED_TBL_CMD_DMA_INDEX	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_DMA_INDEX_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_RULE_ID
 * @brief Rule ID 1
 *
 * Used as a Rule ID for the following functions:
 * * Window Control: Defines the RuleID for which the policing window is opened
 *                   (if Window Open is set) or closed (if Window Close is set).
 * * Conditional triggering: Specifies the Rule ID of a frame that precedes the
 *                           schedule trigger. This is used for the first-stage
 *                           conditional DMA chain execution. Example: if a
 *                           frame classified with certain Rule ID does not
 *                           arrive in time, Gather DMA must generate a recovery
 *                           frame and send it to the network. This is performed
 *                           by executing a configured DMA chain.
 */
#define ACMDRV_SCHED_TBL_CMD_RULE_ID	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_RULE_ID_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID
 * @brief Redundancy Group ID
 *
 * Used together with the DMA Trigger (if its binary value is 01, 10 or 11) to
 * specify which Redundancy group should be selected by the ACM Redundancy
 * module.
 */
#define ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER
 * @brief DMA Trigger
 *
 * Defines which operation is performed by the Gather DMA as follows:
 * * 0 = No trigger
 * * 1 = stand-alone DMA chain execution
 * * 2 = First-stage conditional trigger
 * * 3 = Reserved
 */
#define ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER_BIT)

/**
 * @enum acmdrv_sched_tbl_trig_mode
 * @brief Scheduler Trigger modes
 *
 * @var acmdrv_sched_tbl_trig_mode::ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG
 * @brief No trigger
 *
 * This schedule event is not for the Gather DMA at all, it might be used e.g.
 * for the ingress policing
 *
 * @var acmdrv_sched_tbl_trig_mode::ACMDRV_SCHED_TBL_TRIG_MODE_STAND_ALONE
 * @brief stand-alone DMA chain execution
 *
 * DMA chain is executed no matter if a frame was received or not.
 *
 * @var acmdrv_sched_tbl_trig_mode::ACMDRV_SCHED_TBL_TRIG_MODE_FIRST_STAGE
 * @brief First-stage conditional trigger
 *
 * DMA chain is executed only if a frame with the Rule ID defined in the
 * Rule ID 1 field was received. If the DMA chain is executed, this will set the
 * corresponding bit (i. e. identified by Rule ID 1) at the end of the schedule
 * cycle.
 */
enum acmdrv_sched_tbl_trig_mode {
	ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG = 0,
	ACMDRV_SCHED_TBL_TRIG_MODE_STAND_ALONE = 1,
	ACMDRV_SCHED_TBL_TRIG_MODE_FIRST_STAGE = 2
};

/**
 * @def ACMDRV_SCHED_TBL_CMD_WIN_CLOSE
 * @brief Window Close
 *
 * Indicates that the reception window for a particular Rule ID (identified by
 * Rule ID 1 field) closes here.
 */
#define ACMDRV_SCHED_TBL_CMD_WIN_CLOSE	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_WIN_CLOSE_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_WIN_OPEN
 * @brief Window Open
 *
 * Indicates that the reception window for a particular Rule ID (identified by
 * Rule ID 1 field) opens here.
 */
#define ACMDRV_SCHED_TBL_CMD_WIN_OPEN	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_WIN_OPEN_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_NGN_DISABLE
 * @brief NGN Disable
 *
 * Disables the ACM Bypass module, i. e., all received frames are dropped and
 * no frames are transmitted. This features together with NGN Enable is used to
 * disable the ACM bypass module when the TSN Switch is transmitting frames to
 * prevent collision on egress, and vice versa.
 */
#define ACMDRV_SCHED_TBL_CMD_NGN_DISABLE	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_NGN_DISABLE_BIT)

/**
 * @def ACMDRV_SCHED_TBL_CMD_NGN_ENABLE
 * @brief NGN Enable
 *
 * Enables the NGN Bypass module, i. e., it can receive and transmit frames.
 * This features together with NGN Disable is used to disable the NGN bypass
 * module when the TSN Switch is transmitting frames to prevent collision on
 * egress, and vice versa.
 */
#define ACMDRV_SCHED_TBL_CMD_NGN_ENABLE	\
	BITMASK(ACMDRV_SCHED_TBL_CMD_NGN_ENABLE_BIT)

/**
 * @brief Helper to create Scheduler Table Command
 *
 * @param dma_index DMA List/Command Index
 * @param rule_id Rule ID 1
 * @param redund_group_id Redundancy Group ID
 * @param dma_trigger DMA Trigger mode
 * @param win_close Window Close flag
 * @param win_open Window Open flag
 * @param disable ACM Disable flag
 * @param enable ACM Enable flag
 * @return value for acmdrv_redun_ctrl_entry.ctrl
 */
static inline uint32_t acmdrv_sched_tbl_cmd_create(
	uint16_t dma_index,
	uint8_t rule_id,
	uint8_t redund_group_id,
	enum acmdrv_sched_tbl_trig_mode dma_trigger,
	bool win_close,
	bool win_open,
	bool disable,
	bool enable)
{
	return WVAL(ACMDRV_SCHED_TBL_CMD_DMA_INDEX_BIT, dma_index) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_RULE_ID_BIT, rule_id) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_REDUND_GROUP_ID_BIT,
		    redund_group_id) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER_BIT, dma_trigger) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_WIN_CLOSE_BIT, win_close) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_WIN_OPEN_BIT, win_open) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_NGN_DISABLE_BIT, disable) |
	       WVAL(ACMDRV_SCHED_TBL_CMD_NGN_ENABLE_BIT, enable);
}
 /** @} acmdrv_sched_tbl_row_details */

/**
 * @struct acmdrv_sched_tbl_status
 * @brief data structure for table_status interface
 *
 * @var acmdrv_sched_tbl_status::status
 * @brief Scheduler table entry status word
 *
 * see @ref acmdrv_sched_tbl_status_details "Details"
 */
struct acmdrv_sched_tbl_status {
	uint16_t status;	/**< scheduler table entry status word */
};

/**
 * @name bit structure for acmdrv_sched_tbl_status.status
 * @{
 */
#define ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED_BIT_L	0
#define ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED_BIT_H	0
#define ACMDRV_SCHED_TBL_STATUS_IN_USE_BIT_L		1
#define ACMDRV_SCHED_TBL_STATUS_IN_USE_BIT_H		1
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN_BIT_L	8
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN_BIT_H	8
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED_BIT_L	9
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED_BIT_H	9
#define ACMDRV_SCHED_TBL_STATUS_UPDATE_BIT_L		15
#define ACMDRV_SCHED_TBL_STATUS_UPDATE_BIT_H		15
/** @} */

/**
 * @name Details of ACM Scheduler Table Status Entry
 * @anchor acmdrv_sched_tbl_status_details
 * @{
 */

/**
 * @def ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED
 * @brief Schedule Table Can Be Taken Into Use
 *
 * * 0 = cannot be taken into use
 * * 1 = can be taken into use
 *
 * When scheduler takes the table into use, it clears this bit
 */
#define ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED	\
	BITMASK(ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED_BIT)

/**
 * @def ACMDRV_SCHED_TBL_STATUS_IN_USE
 * @brief Schedule Table Currently In Use
 *
 * * 0 = This table is not currently in use
 * * 1 = This table is in use
 */
#define ACMDRV_SCHED_TBL_STATUS_IN_USE	\
	BITMASK(ACMDRV_SCHED_TBL_STATUS_IN_USE_BIT)

/**
 * @def ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN
 * @brief Last Cycle Number Register Enable/Disable
 *
 * * 0 = Disabled, Cycler process goes on forever
 * * 1 = Enabled, Cycler process stops when last cycle is reached.
 */
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN	\
	BITMASK(ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN_BIT)

/**
 * @def ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED
 * @brief Last Cycle Reached
 */
#define ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED	\
	BITMASK(ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED_BIT)

/**
 * @def ACMDRV_SCHED_TBL_STATUS_UPDATE
 * @brief Update
 *
 * Update Write 1 to update read-only information in CYCLE_TS_* and CYCLE_CNT
 * registers.
 */
#define ACMDRV_SCHED_TBL_STATUS_UPDATE	\
	BITMASK(ACMDRV_SCHED_TBL_STATUS_UPDATE_BIT)

/**
 * @brief Helper to create Scheduler Table Status Entry
 *
 * @param can_be_used Can Be Taken Into Use flag
 * @param last_cyc_num_en Last Cycle Number Register Enable/Disable flag
 * @param update Update flag
 * @return value for acmdrv_sched_tbl_status.status
 */
static inline struct acmdrv_sched_tbl_status acmdrv_sched_tbl_status_create(
	bool can_be_used,
	bool last_cyc_num_en,
	bool update)
{
	struct acmdrv_sched_tbl_status s;

	s.status = WVAL(ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED_BIT, can_be_used) |
		   WVAL(ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN_BIT,
			last_cyc_num_en) |
		   WVAL(ACMDRV_SCHED_TBL_STATUS_UPDATE_BIT, update);

	return s;
}

/**
 * @brief Helper to read Scheduler Table Entry's Can Be Taken Into Use flag
 *
 * @param st Pointer to acmdrv_sched_tbl_status
 * @return Can Be Taken Into Use flag
 */
static inline bool acmdrv_sched_tbl_status_can_be_used_read(
	const struct acmdrv_sched_tbl_status *st)
{
	return RVAL(ACMDRV_SCHED_TBL_STATUS_CAN_BE_USED_BIT, st->status);
}

/**
 * @brief Helper to read Scheduler Table Entry's In Use flag
 *
 * @param st Pointer to acmdrv_sched_tbl_status
 * @return In Use flag
 */
static inline bool acmdrv_sched_tbl_status_in_use_read(
	const struct acmdrv_sched_tbl_status *st)
{
	return RVAL(ACMDRV_SCHED_TBL_STATUS_IN_USE_BIT, st->status);
}

/**
 * @brief Helper to read Scheduler Table Entry's Last Cycle Number Register
 *        Enable/Disable flag
 *
 * @param st Pointer to acmdrv_sched_tbl_status
 * @return Last Cycle Number Register Enable/Disable flag
 */
static inline bool acmdrv_sched_tbl_status_last_cyc_num_en_read(
	const struct acmdrv_sched_tbl_status *st)
{
	return RVAL(ACMDRV_SCHED_TBL_STATUS_LAST_CYC_NUM_EN_BIT, st->status);
}

/**
 * @brief Helper to read Scheduler Table Entry's Last Cycle Reached flag
 *
 * @param st Pointer to acmdrv_sched_tbl_status
 * @return Last Cycle Number Register Enable/Disable
 */
static inline bool acmdrv_sched_tbl_status_last_cyc_reached_read(
	const struct acmdrv_sched_tbl_status *st)
{
	return RVAL(ACMDRV_SCHED_TBL_STATUS_LAST_CYC_REACHED_BIT,
		    st->status);
}
/** @} acmdrv_sched_tbl_status_details */

/**
 * @brief data representation for sched_cycle_time
 */
struct acmdrv_sched_cycle_time {
	uint32_t subns;	/**< sub-nanoseconds */
	uint32_t ns;	/**< nanoseconds */
} __packed;

/**
 * @brief data representation for sched_start_table
 */
struct acmdrv_timespec64 {
	int64_t tv_sec;		/**< seconds */
	int32_t tv_nsec;	/**< nanoseconds */
} __packed;

/**
 * @struct acmdrv_sched_emerg_disable
 * @brief data structure for emergency_disable interface
 *
 * @var acmdrv_sched_emerg_disable::eme_dis
 * @brief emergency disable flag
 *
 * see acmdrv_sched_emerg_disable_details "Details"
 */
struct acmdrv_sched_emerg_disable {
	uint16_t eme_dis;
} __packed;

/**
 * @name bit structure for acmdrv_sched_emerg_disable.eme_dis
 * @{
 */
#define ACMDRV_SCHED_EMERG_DIS_MUX_BIT_L	0
#define ACMDRV_SCHED_EMERG_DIS_MUX_BIT_H	0
#define ACMDRV_SCHED_EMERG_DIS_MUX_STATE_BIT_L	1
#define ACMDRV_SCHED_EMERG_DIS_MUX_STATE_BIT_H	1
/** @} */

/**
 * @name Details of ACM Scheduler Emergency Disable
 * @anchor acmdrv_sched_emerg_disable_details
 * @{
 */

/**
 * @def ACMDRV_SCHED_EMERG_DIS_MUX
 * @brief Emergency disable MUX control
 *
 * * '0' = set MUX so that signals to Downcounters/gate_state outputs are from
 *         scheduler
 * * '1' = set MUX so that signals to Downcounters/gate state outputs are from
 *         EME_DIS_STAT registers
 *
 * The MUX reacts to this setting with zero delay in 0->1 direction. 1->0 change
 * happens only at start of a cycle.
 */
#define ACMDRV_SCHED_EMERG_DIS_MUX	BITMASK(ACMDRV_SCHED_EMERG_DIS_MUX_BIT)

/**
 * @def ACMDRV_SCHED_EMERG_DIS_MUX_STATE
 * @brief Emergency disable MUX state
 *
 * Current state of mergency disable MUX
 * * '0' = signals to Downcounters/gate_state outputs are from scheduler
 * * '1' = signals to Downcounters/gate state outputs are from EME_DIS_STAT
 *         registers
 *
 * The MUX reacts to this setting with zero delay in 0->1 direction. 1->0 change
 * happens only at start of a cycle.
 */
#define ACMDRV_SCHED_EMERG_DIS_MUX_STATE	\
	BITMASK(ACMDRV_SCHED_EMERG_DIS_MUX_STATE_BIT)

/**
 * @brief Helper to create Scheduler Emergency Disable data
 *
 * @param eme_dis_mux Emergency disable MUX control flag
 * @return value for acmdrv_sched_emerg_disable.eme_dis
 */
static inline uint16_t acmdrv_sched_emerg_disable_create(
	bool eme_dis_mux)
{
	return WVAL(ACMDRV_SCHED_EMERG_DIS_MUX_BIT, eme_dis_mux);
}

/**
 * @brief Helper to read Scheduler Emergency disable MUX control flag
 *
 * @param eme Pointer to acmdrv_sched_emerg_disable
 * @return Scheduler Emergency disable MUX control flag
 */
static inline bool acmdrv_sched_emerg_disable_eme_dis_mux_read(
	const struct acmdrv_sched_emerg_disable *eme)
{
	return RVAL(ACMDRV_SCHED_EMERG_DIS_MUX_BIT, eme->eme_dis);
}

/**
 * @brief Helper to read Scheduler Emergency disable MUX state flag
 *
 * @param eme Pointer to acmdrv_sched_emerg_disable
 * @return Scheduler Emergency disable MUX state flag
 */
static inline bool acmdrv_sched_emerg_disable_eme_dis_mux_state_read(
	const struct acmdrv_sched_emerg_disable *eme)
{
	return RVAL(ACMDRV_SCHED_EMERG_DIS_MUX_STATE_BIT, eme->eme_dis);
}
/** @} acmdrv_sched_emerg_disable_details */

/**
 * @name bit structure for cntl_ngn_enable interface
 * @{
 */
#define ACMDRV_BYPASS_CTRL_ENABLE_BIT_L	0
#define ACMDRV_BYPASS_CTRL_ENABLE_BIT_H	0
/** @} */

/**
 * @name Details of ACM Bypass control enable
 * @anchor acmdrv_bypass_control_enable_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_CTRL_ENABLE
 * @brief Enable flag
 *
 * Writing '1' will enable the Bypass module. When it is disabled no frames are
 * being processed by RX MAC. Register value can be change by Hardware via
 * Schedule Trigger to Enabled or Disabled State.
 */
#define ACMDRV_BYPASS_CTRL_ENABLE	BITMASK(ACMDRV_BYPASS_CTRL_ENABLE_BIT)

/**
 * @brief Helper to create data for cntl_ngn_enable interface
 *
 * @param enable Enable flag
 * @return value for cntl_ngn_enable interface
 */
static inline uint32_t acmdrv_bypass_ctrl_enable_create(bool enable)
{
	return WVAL(ACMDRV_BYPASS_CTRL_ENABLE_BIT, enable);
}

/**
 * @brief Helper to read Enable flag
 *
 * @param ifval Value read from cntl_ngn_enable interface
 * @return Enable flag
 */
static inline bool acmdrv_bypass_ctrl_enable_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_CTRL_ENABLE_BIT, ifval);
}
/** @} acmdrv_bypass_control_enable_details */

/**
 * @name bit structure for cntl_lookup_enable interface
 * @{
 */
#define ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE_BIT_L	0
#define ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE_BIT_H	15
/** @} */

/**
 * @name Details of ACM Bypass rule id lookup enable
 * @anchor acmdrv_bypass_rule_id_lookup_enable_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE
 * @brief Enable mask
 *
 * Per each rule in ACM lookup module an enable bit is present in this register.
 * Rule can be matched only when enabled. Write 1 to respective bit for enabling
 * the rule. Default value is all disabled. Default rule 16 cannot be disabled.
 */
#define ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE	\
	BITMASK(ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE_BIT)

/**
 * @brief Helper to create data for cntl_lookup_enable interface
 *
 * @param mask Enable mask
 * @return value for cntl_lookup_enable interface
 */
static inline uint32_t acmdrv_bypass_lookup_enable_create(uint16_t mask)
{
	return WVAL(ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE_BIT, mask);
}

/**
 * @brief Helper to read Enable mask from cntl_lookup_enable interface
 *
 * @param ifval Value read from cntl_lookup_enable interface
 * @return Enable mask
 */
static inline uint16_t acmdrv_bypass_lookup_enable_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_RULE_ID_LOOKUP_ENABLE_BIT, ifval);
}
/** @} acmdrv_bypass_rule_id_lookup_enable_details */

/**
 * @name bit structure for cntl_layer7_enable interface
 * @{
 */
#define ACMDRV_BYPASS_L7_CHECK_ENABLE_BIT_L	0
#define ACMDRV_BYPASS_L7_CHECK_ENABLE_BIT_H	15
/** @} */

/**
 * @name Details of ACM Bypass l7 check enable
 * @anchor acmdrv_bypass_l7_check_enable_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_L7_CHECK_ENABLE
 * @brief Enable mask
 *
 * Each bit is used for enabling the Layer 7 check for a corresponding Rule ID.
 */
#define ACMDRV_BYPASS_L7_CHECK_ENABLE	\
	BITMASK(ACMDRV_BYPASS_L7_CHECK_ENABLE_BIT)

/**
 * @brief Helper to create data for cntl_layer7_enable interface
 *
 * @param mask Enable mask
 * @return value for cntl_layer7_enable interface
 */
static inline uint32_t acmdrv_bypass_layer7_enable_create(uint16_t mask)
{
	return WVAL(ACMDRV_BYPASS_L7_CHECK_ENABLE_BIT, mask);
}

/**
 * @brief Helper to read Enable mask from cntl_layer7_enable interface
 *
 * @param ifval Value read from cntl_layer7_enable interface
 * @return Enable mask
 */
static inline uint16_t acmdrv_bypass_layer7_enable_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_L7_CHECK_ENABLE_BIT, ifval);
}
/** @} acmdrv_bypass_l7_check_enable_details */

/**
 * @name bit structure for cntl_ingress_policing_enable interface
 * @{
 */
#define ACMDRV_BYPASS_INGRESS_POLICING_ENABLE_BIT_L	0
#define ACMDRV_BYPASS_INGRESS_POLICING_ENABLE_BIT_H	15
/** @} */

/**
 * @name Details of ACM Bypass ingress policing enable
 * @anchor acmdrv_bypass_ingress_policing_enable_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_INGRESS_POLICING_ENABLE
 * @brief Enable mask
 *
 * Enabling ingress policing feature for each Rule ID respectively.
 */
#define ACMDRV_BYPASS_INGRESS_POLICING_ENABLE	\
	BITMASK(ACMDRV_BYPASS_INGRESS_POLICING_ENABLE_BIT)

/**
 * @brief Helper to create data for cntl_ingress_policing_enable interface
 *
 * @param mask Enable mask
 * @return value for cntl_ingress_policing_enable interface
 */
static inline uint32_t acmdrv_bypass_ingress_policing_enable_create(
	uint16_t mask)
{
	return WVAL(ACMDRV_BYPASS_INGRESS_POLICING_ENABLE_BIT, mask);
}

/**
 * @brief Helper to read Enable mask from cntl_ingress_policing_enable interface
 *
 * @param ifval Value read from cntl_ingress_policing_enable interface
 * @return Enable mask
 */
static inline uint16_t acmdrv_bypass_ingress_policing_enable_read(
	uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_INGRESS_POLICING_ENABLE_BIT, ifval);
}
/** @} acmdrv_bypass_ingress_policing_enable_details */

/**
 * @enum acmdrv_conn_mode
 * @brief ACM Connection Modes
 *
 * @var acmdrv_conn_mode::ACMDRV_BYPASS_CONN_MODE_SERIES
 * @brief ACM in series to switch IP
 *
 * @var acmdrv_conn_mode::ACMDRV_BYPASS_CONN_MODE_PARALLEL
 * @brief ACM in parallel to switch IP
 *
 * see @ref acmdrv_bypass_conn_mode_details "Connection Mode Details"
 */
enum acmdrv_conn_mode {
	ACMDRV_BYPASS_CONN_MODE_SERIES = 0,
	ACMDRV_BYPASS_CONN_MODE_PARALLEL = 1
};

/**
 * @name bit structure for cntl_connection_mode interface
 * @{
 */
#define ACMDRV_BYPASS_CONN_MODE_BIT_L	0
#define ACMDRV_BYPASS_CONN_MODE_BIT_H	0
/** @} */

/**
 * @name Details of ACM Bypass connection mode
 * @anchor acmdrv_bypass_conn_mode_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_CONNECTION_MODE
 * @brief Connection Mode
 *
 * ACM Bypass module connection mode selection:
 * * 0 - series
 * * 1 - parallel
 */
#define ACMDRV_BYPASS_CONNECTION_MODE	BITMASK(ACMDRV_BYPASS_CONN_MODE_BIT)

/**
 * @brief Helper to create data for cntl_connection_mode interface
 *
 * @param mode Connection mode
 * @return value for cntl_connection_mode interface
 */
static inline uint32_t acmdrv_bypass_conn_mode_create(
	enum acmdrv_conn_mode mode)
{
	return WVAL(ACMDRV_BYPASS_CONN_MODE_BIT, mode);
}

/**
 * @brief Helper to read connection mode from cntl_connection_mode interface
 *
 * @param ifval Value read from cntl_connection_mode interface
 * @return Enable mask
 */
static inline enum acmdrv_conn_mode acmdrv_bypass_conn_mode_read(
	uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_CONN_MODE_BIT, ifval);
}
/** @} acmdrv_bypass_conn_mode_details */

/**
 * @name bit structure for cntl_output_disable interface
 * @{
 */
#define ACMDRV_BYPASS_OUTPUT_DISABLE_BIT_L	0
#define ACMDRV_BYPASS_OUTPUT_DISABLE_BIT_H	0
/** @} */

/**
 * @name Details of ACM Bypass output disable
 * @anchor acmdrv_bypass_output_disable_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_OUTPUT_DISABLE
 * @brief TX Disable flag
 *
 * Bypass module output port control:
 * * 0 - enabled
 * * 1 - disabled
 */
#define ACMDRV_BYPASS_OUTPUT_DISABLE	\
	BITMASK(ACMDRV_BYPASS_OUTPUT_DISABLE_BIT)

/**
 * @brief Helper to create data for cntl_output_disable interface
 *
 * @param disable TX Disable flag
 * @return value for cntl_output_disable interface
 */
static inline uint32_t acmdrv_bypass_output_disable_create(bool disable)
{
	return WVAL(ACMDRV_BYPASS_OUTPUT_DISABLE_BIT, disable);
}

/**
 * @brief Helper to read TX Disable flag from cntl_output_disable interface
 *
 * @param ifval Value read from cntl_output_disable interface
 * @return TX Disable flag
 */
static inline bool acmdrv_bypass_output_disable_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_OUTPUT_DISABLE_BIT, ifval);
}
/** @} acmdrv_bypass_output_disable_details */

/**
 * @name bit structure for cntl_layer7_length interface
 * @{
 */
#define ACMDRV_BYPASS_L7_CHECK_LENGTH_BIT_L	0
#define ACMDRV_BYPASS_L7_CHECK_LENGTH_BIT_H	6
/** @} */

/**
 * @name Details of ACM Bypass L7 Length
 * @anchor acmdrv_bypass_l7_length_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_L7_CHECK_LENGTH
 * @brief L7 Length
 *
 * Number of frame bytes to be involved in the Layer7 check. Layer7 check starts
 * after the VLAN tag (16th frame byte). If value is greater than 112,
 * E07_RIL_L7_CHECK_TOO_LONG error flag is asserted. If the value is 0 and the
 * L7 check is enabled, it will always pass and no error is raised.
 */
#define ACMDRV_BYPASS_L7_CHECK_LENGTH	\
	BITMASK(ACMDRV_BYPASS_L7_CHECK_LENGTH_BIT)

/**
 * @brief Helper to create data for cntl_layer7_length interface
 *
 * @param len L7 Length
 * @return value for cntl_layer7_length interface
 */
static inline uint32_t acmdrv_bypass_layer7_length_create(uint8_t len)
{
	return WVAL(ACMDRV_BYPASS_L7_CHECK_LENGTH_BIT, len);
}

/**
 * @brief Helper to read L7 Length from cntl_layer7_length interface
 *
 * @param ifval Value read from cntl_layer7_length interface
 * @return L7 Length
 */
static inline uint8_t acmdrv_bypass_layer7_length_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_L7_CHECK_LENGTH_BIT, ifval);
}
/** @} acmdrv_bypass_l7_length_details */

/**
 * @enum acmdrv_bypass_speed_select
 * @brief ACM Bypass GMII speed selection
 *
 * @var acmdrv_bypass_speed_select::ACMDRV_BYPASS_SPEED_SELECT_100
 * @brief 100Mbps
 *
 * @var acmdrv_bypass_speed_select::ACMDRV_BYPASS_SPEED_SELECT_1000
 * @brief 1000Mbps
 *
 * see @ref acmdrv_bypass_speed_selection_details "Details"
 */
enum acmdrv_bypass_speed_select {
	ACMDRV_BYPASS_SPEED_SELECT_100 = 0,
	ACMDRV_BYPASS_SPEED_SELECT_1000 = 1,
};

/**
 * @name bit structure for cntl_speed interface
 * @{
 */
#define ACMDRV_BYPASS_SPEED_SELECT_BIT_L	0
#define ACMDRV_BYPASS_SPEED_SELECT_BIT_H	0
/** @} */

/**
 * @name Details of ACM Bypass Speed selection
 * @anchor acmdrv_bypass_speed_selection_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_SPEED_SELECT
 * @brief Speed
 *
 * ACM Bypass GMII speed selection:
 * * 0 - 100Mbps
 * * 1 - 1Gbps
 * Host has the write access to this field only when the ACM Bypass is disabled
 */
#define ACMDRV_BYPASS_SPEED_SELECT	\
	BITMASK(ACMDRV_BYPASS_SPEED_SELECT_BIT)

/**
 * @brief Helper to create data for cntl_speed interface
 *
 * @param s Speed
 * @return value for cntl_speed interface
 */
static inline uint32_t acmdrv_bypass_speed_create(
	enum acmdrv_bypass_speed_select s)
{
	return WVAL(ACMDRV_BYPASS_SPEED_SELECT_BIT, s);
}

/**
 * @brief Helper to read L7 Length from cntl_speed interface
 *
 * @param ifval Value read from cntl_speed interface
 * @return speed
 */
static inline enum acmdrv_bypass_speed_select acmdrv_bypass_speed_read(
	uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_SPEED_SELECT_BIT, ifval);
}
/** @} acmdrv_bypass_speed_selection_details */

/**
 * @name bit structure for cntl_gather_delay interface
 * @{
 */
#define ACMDRV_BYPASS_GATHER_DELAY_BIT_L	0
#define ACMDRV_BYPASS_GATHER_DELAY_BIT_H	10
/** @} */

/**
 * @name Details of ACM Bypass Gather Delay
 * @anchor acmdrv_bypass_gather_delay_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_GATHER_DELAY
 * @brief Delay
 *
 * Specifies the additional delay in number of frame bytes added by the Gather
 * DMA before it starts execution of the Execution DMA chain. This delay will
 * not affect execution of the Prefetch DMA chain that will start immediately
 * after the stream trigger is received from the DMA Command Lookup.
 */
#define ACMDRV_BYPASS_GATHER_DELAY	BITMASK(ACMDRV_BYPASS_GATHER_DELAY_BIT)

/**
 * @brief Helper to create data for cntl_gather_delay interface
 *
 * @param delay Delay
 * @return value for cntl_gather_delay interface
 */
static inline uint32_t acmdrv_bypass_gather_delay_create(uint16_t delay)
{
	return WVAL(ACMDRV_BYPASS_GATHER_DELAY_BIT, delay);
}

/**
 * @brief Helper to read delay from cntl_gather_delay interface
 *
 * @param ifval Value read from cntl_gather_delay interface
 * @return delay
 */
static inline uint16_t acmdrv_bypass_gather_delay_read(uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_GATHER_DELAY_BIT, ifval);
}
/** @} acmdrv_bypass_gather_delay_details */

/**
 * @name bit structure for cntl_ingress_policing_control interface
 * @{
 */
#define ACMDRV_BYPASS_INGRESS_POLICING_CTRL_BIT_L	0
#define ACMDRV_BYPASS_INGRESS_POLICING_CTRL_BIT_H	15
/** @} */

/**
 * @name Details of ACM Bypass Ingress Policing Control
 * @anchor acmdrv_bypass_ingress_policing_control_details
 * @{
 */

/**
 * @def ACMDRV_BYPASS_INGRESS_POLICING_CTRL
 * @brief Close on first match mask
 *
 * Enabling close on first match feature for each Rule ID respectively.
 * * 0 - The window can only be closed when the schedule trigger is asserted
 * * 1 - The window shall be closed when matching frames arrives while window
 *       is open.
 */
#define ACMDRV_BYPASS_INGRESS_POLICING_CTRL	\
	BITMASK(ACMDRV_BYPASS_INGRESS_POLICING_CTRL_BIT)

/**
 * @brief Helper to create data for cntl_ingress_policing_control interface
 *
 * @param mask Close on first match mask
 * @return value for cntl_ingress_policing_control interface
 */
static inline uint32_t acmdrv_bypass_ingress_policing_control_create(
	uint16_t mask)
{
	return WVAL(ACMDRV_BYPASS_INGRESS_POLICING_CTRL_BIT, mask);
}

/**
 * @brief Helper to read Close on first match mask from
 *        cntl_ingress_policing_control interface
 *
 * @param ifval Value read from cntl_ingress_policing_control interface
 * @return Close on first match mask
 */
static inline uint16_t acmdrv_bypass_ingress_policing_control_read(
	uint32_t ifval)
{
	return RVAL(ACMDRV_BYPASS_INGRESS_POLICING_CTRL_BIT, ifval);
}
/** @} acmdrv_bypass_ingress_policing_control_details */

/**
 * @brief magic to be used for clearing/initializing entire ACM IP in
 *        clear_all_fpga interface
 */
#define ACMDRV_CLEAR_ALL_PATTERN	0x13F72288

/** @} acmsysfsconfig */

/******************************************************************************/
/**
 * @defgroup acmsysfserror ACM SYSFS Error Section
 * @brief The error attributes of the ACM
 *
 * The SYSFS error section provides the following ASCII attributes under
 * subdirectory specified by #ACMDRV_SYSFS_ERROR_GROUP. This attributes always
 * come twice appended by "_M0" and "_M1" for the respective bypass module:
 * - *halt_on_error*: An error occurred in the IP and lead to complete HALT of
 *                    its operation (i. e. ACM is disabled and cannot be
 *                    re-enabled until IP reset). The flag is only cleared on
 *                    reset.
 *                    Default value: 0
 * - *halt_on_other_bypass*: Bypass module is in the HALT state because the
 *                           other bypass module got into the HALT state (there
 *                           was an IP error in the other bypass module that
 *                           lead to HALT). The flag is only cleared on reset.
 *                           Default value: 0
 * - *@ref acmbypasserrorflags "error_flags"*: Each time an error occurs the
 *                  corresponding bit(s) in this vector are asserted. Each bit
 *                  in this vector represents one error. The bits are cleared on
 *                  read or on IP reset.
 *                  Default value: 0
 * - *@ref acmbypasspolicingerrorflags "policing_flags"*: Each time an policing
 *                  error occurs the corresponding bit(s) in this vector are
 *                  asserted. Each bit in this vector represents one error. The
 *                  bits are cleared on read or on IP reset.
 *                  Default value: 0
 * @{
 */

/**
 * @brief ACM Error section SYSFS directory
 */
#define ACMDRV_SYSFS_ERROR_GROUP	error

/**
 * @name acmdrv_bypass_halt_on_error: Halt On Error Occured
 *
 * This flag informs that an error occurred in the IP and lead to complete HALT
 * of its operation (i. e. ACM is disabled and cannot be re-enabled until IP
 * reset). This only occurs for errors that use M00_IDLE_WAIT_FOR_RESET as the
 * mitigation strategy. All other errors cannot trigger HALT but the IP can
 * mitigate them. The flag is only cleared on reset.
 * @{
 */
#define ACMDRV_BYPASS_HALT_ON_ERROR_BIT_L	0
#define ACMDRV_BYPASS_HALT_ON_ERROR_BIT_H	0
#define ACMDRV_BYPASS_HALT_ON_ERROR	\
	BITMASK(ACMDRV_BYPASS_HALT_ON_ERROR_BIT)
/** @} */

/**
 * @name acmdrv_bypass_halt_on_other_bypass: Halt On Error On Other Bypass
 * Occured
 *
 * This flag informs that this ACM Bypass module is in the HALT state because
 * the other ACM Bypass module got into the HALT state (there was an IP error in
 * the other ACM Bypass module that lead to HALT). The flag is only cleared
 * on reset.
 * @{
 */
#define ACMDRV_BYPASS_HALT_ON_OTHER_BYPASS_BIT_L	16
#define ACMDRV_BYPASS_HALT_ON_OTHER_BYPASS_BIT_H	16
#define ACMDRV_BYPASS_HALT_ON_OTHER_BYPASS	\
	BITMASK(ACMDRV_BYPASS_HALT_ON_OTHER_BYPASS_BIT)
/** @} */

/**
 * @defgroup acmbypasserrorflags ACM Bypass Error Flags
 * @brief Flags used at error_flags SYSFS interface
 *
 * Flags indicating "hard" errors like GMII errors, Network configuration errors
 * and IP configuration errors. These errors should not occur during normal
 * operation of the IP.
 *
 * @{
 */
/**
 * @def ACMDRV_BYPASS_E00_RXM_SPEED_ERROR
 *
 * IP detected that the GMII interface speed (dictated by the TSN switch) is
 * different than the speed configured in Register Speed Select (SPEED_SELECT).
 * This error can be only triggered if the ACM Bypass module is enabled.
 */
#define ACMDRV_BYPASS_E00_RXM_SPEED_ERROR	BIT_ULL(0)

/**
 * @def ACMDRV_BYPASS_E01_RXM_SOF_ERROR
 *
 * * GMII RXDV raises, but the byte is neither 0x55 (preamble) nor 0xD5 (SOF),
 *   or
 * * Preamble bytes 0x55 are not followed by the 0xD5 (SOF) byte,
 *   or
 * * RXERR raises while RXDV is High during Preamble (0x55) or SOF (0xD5) bytes.
 */
#define ACMDRV_BYPASS_E01_RXM_SOF_ERROR	BIT_ULL(1)

/**
 * @def ACMDRV_BYPASS_E02_RXM_IFG_ERROR
 *
 * The duration of (Inter-frame gap + Preamble bytes + SOF byte) is less than
 * 15 bytes. Duration of 1 byte is 8ns for 1Gbps port speed, 80ns for 100 Mbps
 */
#define ACMDRV_BYPASS_E02_RXM_IFG_ERROR	BIT_ULL(2)

/**
 * @def ACMDRV_BYPASS_E03_RXM_RX_ERROR
 *
 * RXERR raises while RXDV is High. If this happens during Preamble or SOF
 * bytes, the error is considered as SOF error (E01). However, if error occurs
 * during frame data reception, such frame has to be dropped from the frame
 * buffer
 */
#define ACMDRV_BYPASS_E03_RXM_RX_ERROR	BIT_ULL(3)

/**
 * @def ACMDRV_BYPASS_E04_RXM_SIZE_ERROR
 *
 * Frame size (from first DMAC byte to last FCS byte) exceeds 1528 bytes.
 */
#define ACMDRV_BYPASS_E04_RXM_SIZE_ERROR	BIT_ULL(4)

/**
 * @def ACMDRV_BYPASS_E05_RXM_RUNT_FRAME_ERROR
 *
 * Size of frame (excluding Preamble and SOF bytes) is less than 64 bytes
 */
#define ACMDRV_BYPASS_E05_RXM_RUNT_FRAME_ERROR	BIT_ULL(5)

/**
 * @def ACMDRV_BYPASS_E06_RXM_FCS_ERROR
 *
 * The frame fails the FCS check.
 */
#define ACMDRV_BYPASS_E06_RXM_FCS_ERROR	BIT_ULL(5)

/**
 * @def ACMDRV_BYPASS_E07_RIL_L7_CHECK_TOO_LONG
 *
 * The frame ends before the Layer 7 check could be finished and the Layer 7
 * check is enabled for the given Rule ID. Or the Layer 7 Check Length
 * (L7_CHECK_LENGTH) register contains value greater than 112 which is a
 * configuration error.
 */
#define ACMDRV_BYPASS_E07_RIL_L7_CHECK_TOO_LONG	BIT_ULL(7)

/**
 * @def ACMDRV_BYPASS_E08_FBUF_Q_FULL
 *
 * The frame buffer cannot store a new ingress frames because its frame queue
 * is already full
 */
#define ACMDRV_BYPASS_E08_FBUF_Q_FULL	BIT_ULL(8)

/**
 * @def ACMDRV_BYPASS_E09_DCL_INVALID_CMD
 *
 * Occurs when the stream trigger with the Valid = 0 is executed
 */
#define ACMDRV_BYPASS_E09_DCL_INVALID_CMD	BIT_ULL(9)

/**
 * @def ACMDRV_BYPASS_E10_DCL_Q_OVERFLOW
 *
 * A new event trigger or time trigger is generated, but the trigger queue is
 * already full
 */
#define ACMDRV_BYPASS_E10_DCL_Q_OVERFLOW	BIT_ULL(10)

/**
 * @def ACMDRV_BYPASS_E11_SCD_INVALID_CMD
 *
 * A DMA command with Type = 0 (Invalid) was executed by the Scatter DMA, or
 * Scatter NOP command was executed without “Last” flag set, or Scatter MAX
 * (2047 bytes) move command was executed without “Last” flag set.
 */
#define ACMDRV_BYPASS_E11_SCD_INVALID_CMD	BIT_ULL(11)

/**
 * @def ACMDRV_BYPASS_E12_SCD_CHAIN_OVERFLOW
 *
 * The last command in the DMA memory was executed and did not contain the Last
 * flag set to 1
 */
#define ACMDRV_BYPASS_E12_SCD_CHAIN_OVERFLOW	BIT_ULL(12)

/**
 * @def ACMDRV_BYPASS_E13_SCD_FBUF_UNDERRUN
 *
 * If the received frame (after FCS is received) is shorter than the specified
 * offset/length combination in the DMA command
 */
#define ACMDRV_BYPASS_E13_SCD_FBUF_UNDERRUN	BIT_ULL(13)

/**
 * @def ACMDRV_BYPASS_E14_SCD_MBUF_OVERRUN
 *
 * If the message buffer size is smaller than the length of the message written
 * to it by the Scatter DMA
 */
#define ACMDRV_BYPASS_E14_SCD_MBUF_OVERRUN	BIT_ULL(14)

/**
 * @def ACMDRV_BYPASS_E15_SCD_INVALID_LEN
 *
 * If the value specified in the Length field is 1, 2 or 3, from 1529 to 2046
 */
#define ACMDRV_BYPASS_E15_SCD_INVALID_LEN	BIT_ULL(15)

/**
 * @def ACMDRV_BYPASS_E16_SCD_RESERVED_CMD_EXECUTED
 *
 * If the executed command has the Type = 3 (i. e., reserved actions)
 */
#define ACMDRV_BYPASS_E16_SCD_RESERVED_CMD_EXECUTED	BIT_ULL(16)

/**
 * @def ACMDRV_BYPASS_E17_SCD_MBUF_LOCK_FAILED
 *
 * Occurs if the sub-buffer cannot be locked, because all sub-buffers are
 * already locked
 */
#define ACMDRV_BYPASS_E17_SCD_MBUF_LOCK_FAILED	BIT_ULL(17)

/**
 * @def ACMDRV_BYPASS_E18_PRE_INVALID_CMD
 *
 * A Prefetch command with Type = 0 (Invalid) was executed.
 */
#define ACMDRV_BYPASS_E18_PRE_INVALID_CMD	BIT_ULL(18)

/**
 * @def ACMDRV_BYPASS_E19_PRE_CHAIN_OVERFLOW
 *
 * The last command in the Prefetch DMA memory was executed and did not contain
 * the Last flag set to 1 or was not implicitly the last DMA chain command
 */
#define ACMDRV_BYPASS_E19_PRE_CHAIN_OVERFLOW	BIT_ULL(19)

/**
 * @def ACMDRV_BYPASS_E20_PRE_NOP_NOT_ONLY_CMD
 *
 * NOP command (Type = 1) is not the only command in the DMA chain.
 */
#define ACMDRV_BYPASS_E20_PRE_NOP_NOT_ONLY_CMD	BIT_ULL(20)

/**
 * @def ACMDRV_BYPASS_E21_PRE_LOCK_CMD_FOLLOWS_OTHER_CMDS
 *
 * Lock command (Type = 3) must always be executed at the beginning of the DMA
 * chain. To lock all the message buffers, two Lock commands need to be used.
 * However, if any of these commands is used after a different command, this
 * error is generated. Also if more than two Lock commands are used this error
 * is generated.
 */
#define ACMDRV_BYPASS_E21_PRE_LOCK_CMD_FOLLOWS_OTHER_CMDS	BIT_ULL(21)

/**
 * @def ACMDRV_BYPASS_E23_PRE_LOCK_FAILED
 *
 * Occurs if the sub-buffer cannot be locked. This can happen if message buffer
 * is already locked but by a different lock type, i. e.
 * 1. Message buffer is already dual-locked (e. g. by the other Bypass module),
 *    and Prefetch tries to single-lock it (implicitly or explicitly)
 * 2. Message buffer is already single-locked (implicitly or explicitly) and
 *    Prefetch tries to dual-lock it.
 */
#define ACMDRV_BYPASS_E23_PRE_LOCK_FAILED	BIT_ULL(23)

/**
 * @def ACMDRV_BYPASS_E24_PRE_MBUF_UNDERRUN
 *
 * If the buffer is smaller than the message length specified in the DMA command
 * length field
 */
#define ACMDRV_BYPASS_E24_PRE_MBUF_UNDERRUN	BIT_ULL(24)

/**
 * @def ACMDRV_BYPASS_E25_PRE_MBUF_EMPTY
 *
 * If the message buffer does not contain data (has the Empty flag set)
 */
#define ACMDRV_BYPASS_E25_PRE_MBUF_EMPTY	BIT_ULL(25)

/**
 * @def ACMDRV_BYPASS_E26_PRE_FIFO_OVERFLOW
 *
 * More than 1506 bytes were stored in the Prefetch FIFO
 */
#define ACMDRV_BYPASS_E26_PRE_FIFO_OVERFLOW	BIT_ULL(26)

/**
 * @def ACMDRV_BYPASS_E27_PRE_Q_OVERFLOW
 *
 * Prefetch failed to read a new message from the Message Buffer while it
 * already contains 8 messages
 */
#define ACMDRV_BYPASS_E27_PRE_Q_OVERFLOW	BIT_ULL(27)

/**
 * @def ACMDRV_BYPASS_E28_PRE_NOT_FULLY_READ
 *
 * If less messages were read by the Gather from the Prefetch than were stored
 * in the Prefetch, and the Execution DMA chain has finished execution (i. e.,
 * frame was assembled and transmitted).
 */
#define ACMDRV_BYPASS_E28_PRE_NOT_FULLY_READ	BIT_ULL(28)

/**
 * @def ACMDRV_BYPASS_E29_GTD_INVALID_CMD
 *
 * An Execution command with Type = 0 (Invalid) was executed. Moreover, this
 * error can be raised if a NOP command was executed and it was not the first in
 * the DMA chain, or if the NOP command was executed without the Last flag set
 * to 1.
 */
#define ACMDRV_BYPASS_E29_GTD_INVALID_CMD	BIT_ULL(29)

/**
 * @def ACMDRV_BYPASS_E30_GTD_TT_INVALID_CMD
 *
 * If the trigger was generated by the schedule, but one of the following
 * commands was executed:
 * * Forward (Type=1)
 * * Move from Frame Buf. (Type=5)
 */
#define ACMDRV_BYPASS_E30_GTD_TT_INVALID_CMD	BIT_ULL(30)

/**
 * @def ACMDRV_BYPASS_E31_GTD_CHAIN_OVERFLOW
 *
 * The last command in the Execution DMA memory was executed and did not contain
 * the Last flag set to 1 or was not implicitly the last DMA chain command.
 */
#define ACMDRV_BYPASS_E31_GTD_CHAIN_OVERFLOW	BIT_ULL(31)

/**
 * @def ACMDRV_BYPASS_E32_GTD_INSERT_DELAY_FOLLOWS_OTHER_CMDS
 *
 * If used, Insert Delay command (Type = 3) must always be executed at the
 * beginning of the DMA chain. Otherwise this error is generated
 */
#define ACMDRV_BYPASS_E32_GTD_INSERT_DELAY_FOLLOWS_OTHER_CMDS	BIT_ULL(32)

/**
 * @def ACMDRV_BYPASS_E33_GTD_FWD_NOT_ONLY_CMD
 *
 * Forward command (Type = 1) is not the only command in the DMA chain
 */
#define ACMDRV_BYPASS_E33_GTD_FWD_NOT_ONLY_CMD	BIT_ULL(33)

/**
 * @def ACMDRV_BYPASS_E34_GTD_FBUF_UNDERRUN
 *
 * Command with Type = 5 (i. e., move data from the Frame Buffer to the TX MAC)
 * is executed, but the Frame Buffer does not contain the data specified by the
 * offset/length field combination in the command. This error is triggered
 * either if Gather tries to read frame bytes than have not yet been received by
 * the Frame Buffer, or when the frame is shorter (after the FCS bytes are
 * received) than the specified offset/length field combination in the command
 */
#define ACMDRV_BYPASS_E34_GTD_FBUF_UNDERRUN	BIT_ULL(34)

/**
 * @def ACMDRV_BYPASS_E35_GTD_PREFETCH_UNDERRUN
 *
 * Command with Type = 2 (i. e. move data from the Prefetch to the TX MAC) is
 * executed, but the Prefetch does not contain the message or enough data
 */
#define ACMDRV_BYPASS_E35_GTD_PREFETCH_UNDERRUN	BIT_ULL(35)

/**
 * @def ACMDRV_BYPASS_E36_TXM_DISABLE_OVERRUN
 *
 * If the ACM is disabled (by the user, schedule, speed change, etc.) while a
 * frame is being transmitted
 */
#define ACMDRV_BYPASS_E36_TXM_DISABLE_OVERRUN	BIT_ULL(36)

/**
 * @def ACMDRV_BYPASS_E37_RED_RTAG_MISSING
 *
 * Received frame was expected to contain an R-Tag, but it did not
 */
#define ACMDRV_BYPASS_E37_RED_RTAG_MISSING	BIT_ULL(37)

/**
 * @def ACMDRV_BYPASS_E38_GTD_CBUF_OVERFLOW
 *
 * The Gather DMA Execution command for moving data from the Constant Buffer
 * (i. e., Command Type = 4) has the sum of "Constant Buffer Offset" and
 * "Length" greater than 4096. This causes Constant Buffer overflow when it is
 * read by the Gather DMA.
 */
#define ACMDRV_BYPASS_E38_GTD_CBUF_OVERFLOW	BIT_ULL(38)

/**
 * @def ACMDRV_BYPASS_E39_MB_INVALID_BUFF_ACCESS
 *
 * Scatter or gather access to a message buffer which is not configured.
 */
#define ACMDRV_BYPASS_E39_MB_INVALID_BUFF_ACCESS	BIT_ULL(39)

/**
 * @def ACMDRV_BYPASS_E40_SWITCH_NGN_COLLISION
 *
 * Switch and NGN are transmitting at the same time. Not applied to IFG time,
 * e.g. switch can start transmission as soon as ACM finishes its transmission.
 */
#define ACMDRV_BYPASS_E40_SWITCH_NGN_COLLISION	BIT_ULL(40)

/** @} acmbypasserrorflags */

/**
 * @defgroup acmbypasspolicingerrorflags ACM Bypass Policing Error Flags
 * @brief Flags used at policing_flags SYSFS interface
 *
 * Flags indicating "soft" policing errors that lead to frame drops resulting
 * from an incorrect schedule configuration or when frames arrived too late
 * because of some network problem.
 *
 * @{
 */
/**
 * @def ACMDRV_BYPASS_P00_DCL_L7_MISMATCH
 *
 * Layer 7 check is enabled and it failed for the received frame Layer 7 check
 * is enabled and it failed for the received frame
 */
#define ACMDRV_BYPASS_P00_DCL_L7_MISMATCH	BIT(0)

/**
 * @def ACMDRV_BYPASS_P01_DCL_WINDOW_CLOSED
 *
 * Frame was received but it failed the window policing check, since the
 * policing window was closed
 */
#define ACMDRV_BYPASS_P01_DCL_WINDOW_CLOSED	BIT(1)

/**
 * @def ACMDRV_BYPASS_P02_DCL_1ST_RECOVERY_EXEC
 *
 * First-stage conditional trigger was received from the schedule and it found
 * associated policing window opened
 */
#define ACMDRV_BYPASS_P02_DCL_1ST_RECOVERY_EXEC	BIT(2)

/** @} acmbypasspolicingerrorflags */
/**@} acmsysfserror*/

/******************************************************************************/
/**
 * @defgroup acmsysfsstatus ACM SYSFS Status Section
 * @brief The status attributes of the ACM
 *
 * The SYSFS status section provides the following ASCII attributes under
 * subdirectory specified by #ACMDRV_SYSFS_STATUS_GROUP. This attributes always
 * come twice appended by "_M0" and "_M1" for the respective bypass module:
 * - *runt_frames*: Counts the number of frames that do not have a SOF error,
 *                  nor MII error, but are shorter than 64 bytes. The counter
 *                  will not wrap and is cleared on read.
 * - *mii_errors*: Counts the number of frames that started with a valid start
 *                 sequence (preamble plus SOF delimiter byte) but terminated
 *                 with the MII error input being asserted. It will be reset on
 *                 power-on/reset and whenever the host reads the status word
 *                 containing the counter. The field will not wrap.
 *                 Default value: 0
 * - *layer7_missmatch_cnt*: Counts the number of frames that failed the Layer 7
 *                           check if this check was enabled. This counter is
 *                           cleared on read and will not wrap.
 *                           Default value: 0
 * - *drop_frames_cnt_prev*: Counts dropped frames, due to policing errors,
 *                           during previous (last finished) gate cycle.
 *                           Default value: 0
 * - *scatter_DMA_frames_cnt_prev*: Number of frames processed by Scatter DMA
 *                                  engine (without dropped frames) in the
 *                                  previous cycle. The field will not wrap.
 *                                  Default value: 0
 * - *tx_frames_prev*: Contains the number of MAC-level correct frames (without
 *                     GMII errors) transmitted on the respective port in the
 *                     previous gating cycle. The counter will not wrap and is
 *                     updated at the beginning of the cycle.
 *                     Default value: 0
 * - *gmii_errors_set_prev*: Frames with GMII error set in the previous cycle.
 *                           If a frame transmission had to be stopped (e. g.
 *                           FCS or MTU error was detected) then ACM IP need to
 *                           assert GMII error signal to intentionally corrupt
 *                           the frame. The counter will not wrap.
 *                           Default value: 0
 *                           (available only for up to ttt,acm-2.0. It has been
 *                           substituted by gmii_error_prev_cycle sinc
 *                           ttt-acm-3.0)
 * - *disable_overrun_prev*: Number of all frames whose transmission was aborted
 *                           (frame was broken) by the TX MAC when ACM was
 *                           disabled by the schedule in the previous cycle.
 *                           The field will not wrap.
 *                           Default value: 0
 * - *tx_frame_cycle_change*: This field is incremented when at the end of a
 *                            cycle rTxFramesPrevCycle and rTxFramesCurrCycle
 *                            are not equal. This field will not wrap.
 *                            Default value: 0
 *
 * For ACM IP versions up till 0.9.27:
 * - *ifc_version*: ACM IP interface version
 * - *config_version*: ACM IP configuration version
 *
 * For ACM IP versions from 0.9.28 onwards:
 * - *device_id*: ACM IP Core Identification
 * - *version_id*: ACM IP Core Version String
 * - *revision_id*: ACM IP Core Revision Identification
 * - *extended_status*: ACM IP extended status flag
 * - *testmodule_enable*: ACM IP testmodule enable flag
 * - *time_freq*: scheduler clock frequency in Hz
 * - *msgbuf_memsize*: size of entire message buffer memory
 *
 * Additional data available since ACM IP versions > 1.1.0:
 * - *msgbuf_count*: number of available message buffers
 * - *msgbuf_datawidth*: width of a single message buffer data item
 *
 * For ttt,acm-3.0 the following registers additionally apply:
 *
 * - *rx_frames_prev*: RX Frames Critical Trafic Previous Cycle
 *                     Number of ingress frames processed by Bypass Module in
 *                     the previous cycle. Counter increments only for ingress
 *                     frames without errors (GMII, CRC, ...) which Stream ID
 *                     Lookup classified as rule 0-15 (rule 16 is excluded from
 *                     statistics) and L7 check passed (when enabled). Counter
 *                     also should not count frames dropped due to policing
 *                     errors. The field will not wrap.
 * - *rx_frames_cycle_change*: Frames Received Cycle Change
 *                             This field is incremented when at the end of a
 *                             cycle rx_frames_critical_prev_cycle and
 *                             rx_frames_critical_curr_cycle are not equal. The
 *                             counter will not wrap and is clear on read.
 *
 * - *sof_errors*: SOF Errors
 *                 This field counts the number of frames that:
 *                 * started less than 16 clocks after the most recent frame,
 *                 * have a byte other than 0xD5 being the first byte that is
 *                   different from 0x55 (if the frame starts with a preamble),
 *                 * have the MII error input being asserted prior to or up to
 *                   the SOF delimiter byte
 *                 * terminated before the first byte of the Destination MAC
 *                 address.
 *                 It will be reset on power-on/reset and whenever the host
 *                 reads the status word containing the counter. The field will
 *                 not wrap.
 *
 * - *gmii_error_prev_cycle*: Frames with GMII Error
 *                            This field counts the number of frames which were
 *                            output with GMII Error (e.g. GMII Error detected
 *                            on input, FCS or MTU error was detected, ...).
 *                            It will be reset on power-on/reset or by host read
 *                            (clear on read register). The field will not wrap.
 *
 * When using a extended status enabled ACM IP there are a few additional registers for
 * debugging purposes, which are presented via the following sysfs interfaces:
 *
 * - *rx_bytes*: Contains the number of bytes (all data bytes of an Ethernet
 *               frame from the first byte of the Ethernet destination MAC
 *               address to the last byte of the checksum but not including
 *               preamble bytes nor SOF delimiters) received on the respective
 *               port. The counter will not wrap and is clear on read.
 * - *rx_frames*: Contains the number of MAC-level correct frames received on
 *                the respective port. The counter will not wrap and is cleared
 *                on read.
 * - *fcs_errors*: Number of frames that had a receive-side FCS error
 * - *size_errors*: Counts the number of frames that had an invalid length on
 *                  this port. The counter will not wrap and is cleared on read.
 *                  The length is considered incorrect if frame contains more
 *                  than 1528 bytes (no matter if the frame contains VLAN tag or
 *                  R-Tag).
 * - *lookup_match_vec*: Contains vector (16:0) indicating last match in Rule ID
 *                       Lookup Module. This vector is cleared on read.
 * - *layer7_match_vec*: Contains vector (15:0) indicating last match in the
 *                       Layer 7 for all the enabled Rule IDs. This vector is
 *                       cleared on read.
 * - *last_stream_trigger*: Contains stream trigger data of last stream trigger
 *                          that was looked up .This field is cleared on read.
 * - *ingress_win_stat*: Ingress Window Status
 *                       This register contains information about window for
 *                       particular Rule ID.
 *                       * 0 - window is closed
 *                       * 1 - window is open
 *                       If the ingress policing is disabled, frames will not be
 *                       dropped no matter if the window is open or closed.
 * - *sched_trig_cnt_prev_cyc*: Schedule Trigger Counter Previous Cycle
 *                              Number of processed schedule triggers (both
 *                              normal and conditional triggers are counted) in
 *                              the previous cycle. This counter updates its
 *                              value at the beginning of each new schedule
 *                              cycle. The field will not wrap.
 * - *sched_1st_cond_trig_cnt_prev_cyc*: Schedule First Conditional Trigge
 *                                       Counter Previous Cycle
 *                                       Number of first-stage conditional
 *                                       schedule triggers that resulted in
 *                                       first-stage recovery frame transmission
 *                                       (because the corresponding frame did
 *                                       not arrive on ingress in time) in the
 *                                       previous cycle . The field will not
 *                                       wrap.
 * - *pending_req_max_num*: Pending Request Max Number
 *                          Maximal number of pending requests in DMA Command
 *                          Lookup trigger queue since last read. It cannot be
 *                          larger than 8 and does not wrap. It is cleared on
 *                          read.
 * - *scatter_DMA_frames_cnt_curr*: Frames scattered Current Cycle
 *                                  Number of frames processed by Scatter DMA
 *                                  engine (without dropped frames) in the
 *                                  current cycle. The field will not wrap.
 * - *scatter_DMA_bytes_cnt_prev*: Bytes Scattered Previous Cycle
 *                                 Number of bytes moved by Scatter DMA engine
 *                                 to RX Message Buffer in the previous cycle.
 *                                 The field will not wrap.
 * - *scatter_DMA_bytes_cnt_curr*: Bytes Scattered Current Cycle
 *                                 Number of bytes moved by Scatter DMA engine
 *                                 to RX Message Buffer in the current cycle.
 *                                 This field displays just the least
 *                                 significant byte of a non-wrap 24 bit
 *                                 counter.
 * - *tx_bytes_prev*: Bytes Transmitted Previous Cycle
 *                    This field contains the number of transmitted bytes (all
 *                    data bytes of an Ethernet frame from the first byte of the
 *                    Ethernet destination MAC address to the last byte of the
 *                    FCS checksum but not including preamble bytes nor SOF
 *                    delimiters) on the respective port at MAC-level correct
 *                    frames in the previous gating cycle. Only bytes without
 *                    GMII error are counted. The counter will not wrap and is
 *                    updated at the beginning of the cycle.
 * - *tx_bytes_curr*: Bytes Transmitted Current Cycle
 *                    This field contains the number of transmitted bytes (all
 *                    data bytes of an Ethernet frame from the first byte of the
 *                    Ethernet destination MAC address to the last byte of the
 *                    FCS checksum but not including preamble bytes nor SOF
 *                    delimiters) on the respective port at MAC-level correct
 *                    frames in the current cycle. Only bytes without GMII error
 *                    are counted. This field displays just the least
 *                    significant byte.
 * - *tx_frames_cyc_1st_change*: Transmitted Frames First Change
 *                               PTP Time of first increment of
 *                               tx_frames_cycle_change_cnt. This is a 64-bit
 *                               value (represents nanoseconds wrapping
 *                               after 999,999,999 ns).
 * - *tx_frames_cyc_last_change*: Transmitted Frames First Change
 *                                PTP Time of last increment of
 *                                tx_frames_cycle_change_cnt. This is a 64-bit
 *                                value (represents nanoseconds wrapping
 *                                after 999,999,999 ns).
 *
 * For ttt,acm-3.0 the following debug registers additionally apply:
 *
 * - *rx_frames_curr*: RX Frames Critical Trafic Current Cycle
 *                     Number of ingress frames processed by Bypass Module in
 *                     the current cycle. Counter increments only for ingress
 *                     frames without errors (GMII CRC, ...) which Stream ID
 *                     Lookup classified as rule 0-15 (rule 16 is excluded from
 *                     statistics) and L7 check passed (when enabled). Counter
 *                     also should not count frames dropped due to policing
 *                     errors. The field will not wrap.
 *
 * For ttt,acm-4.0 the following registers additionally apply:
 *
 * - *redund_frames_produced*: Count Redundancy Frames Produced by respective
 *                             Bypass 0. Counter do not wrap.
 *
 * @{
 */

/**
 * @brief ACM Status section SYSFS directory
 */
#define ACMDRV_SYSFS_STATUS_GROUP	status

/**@} acmsysfsstatus */

/**
 * @defgroup acmsysfsdiag ACM SYSFS Diagnostic Section
 * @brief Diagnostic data of the ACM
 *
 * The SYSFS diagnostic section provides the following attributes under
 * subdirectory specified by #ACMDRV_SYSFS_DIAG_GROUP:
 * - *diagnostics*: a set of accumulated diagnostic binary data, e.g. counters
 *                  and flags. A value of 0xFFFFFFFF for a counter indicates
 *                  a counter overflow (either on HW or during accumulation) and
 *                  thus an unusable value.
 * - *diag_poll_time*: comma separated list of diagnostic poll time in
 *                     millisecond. Each value represents the poll time for
 *                     the respective bypass module. A value of 0 turns polling
 *                     off, the default is set to 50m to avoid the schedule
 *                     cycle counter overflows down to 200usec cycle time.
 * @{
 */

/**
 * @brief ACM Control section SYSFS directory
 */
#define ACMDRV_SYSFS_DIAG_GROUP	diag

/**
 * @struct acmdrv_diagnostics
 * @brief ACM diagnostics data structure per module
 *
 * @var acmdrv_diagnostics::timestamp
 * @brief Diagonstic data timestamp
 *
 * @var acmdrv_diagnostics::scheduleCycleCounter
 * @brief Schedule Cycle Counter
 *
 * @var acmdrv_diagnostics::txFramesCounter
 * @brief Number of transmitted frames
 *
 * @var acmdrv_diagnostics::rxFramesCounter
 * @brief Number of received frames
 *
 * @var acmdrv_diagnostics::ingressWindowClosedFlags
 * @brief Ingress Window Closed Bitmask
 *
 * @var acmdrv_diagnostics::ingressWindowClosedCounter
 * @brief Ingress Window Closed Counter array
 *
 * @var acmdrv_diagnostics::noFrameReceivedFlags
 * @brief No Frames Received Bitmask
 *
 * @var acmdrv_diagnostics::noFrameReceivedCounter
 * @brief No Frames Received Counter array
 *
 * @var acmdrv_diagnostics::recoveryFlags
 * @brief Recovery Bitmask
 *
 * @var acmdrv_diagnostics::recoveryCounter
 * @brief Recovery Counter array
 *
 * @var acmdrv_diagnostics::additionalFilterMismatchFlags
 * @brief Additional Filter Mismatch Bitmask
 *
 * @var acmdrv_diagnostics::additionalFilterMismatchCounter
 * @brief Additional Filter Mismatch Counter array
 */
struct acmdrv_diagnostics {
	struct acmdrv_timespec64 timestamp;
	uint32_t scheduleCycleCounter;

	uint32_t txFramesCounter;
	uint32_t rxFramesCounter;

	uint32_t ingressWindowClosedFlags;
	uint32_t ingressWindowClosedCounter[ACMDRV_BYPASS_NR_RULES];

	uint32_t noFrameReceivedFlags;
	uint32_t noFrameReceivedCounter[ACMDRV_BYPASS_NR_RULES];

	uint32_t recoveryFlags;
	uint32_t recoveryCounter[ACMDRV_BYPASS_NR_RULES];

	uint32_t additionalFilterMismatchFlags;
	uint32_t additionalFilterMismatchCounter[ACMDRV_BYPASS_NR_RULES];
} __packed;

/**@} acmsysfsdiag */

/**@} acmsysfs */

/******************************************************************************/
/**
 * @defgroup acmmsgbuf ACM Message Buffer
 * @brief The ACM kernel module's message buffer interface
 *
 * A message buffer device is created dynamically when msg_buff_alias in the
 * config SYSFS section is written under /dev/\<alias\> from
 * acmdrv_buff_alias.alias. It is a character device which can be read or
 * written depending on its configuration via msg_buff_desc SYSFS config
 * interface according to #ACMDRV_BUFF_DESC_BUFF_TYPE in acmdrv_msfbuf_desc.desc
 * @{
 *
 */
/**@} acmmsgbuf */

/******************************************************************************/
/**
 * @defgroup acmdrvdata ACM Driver Data
 * @brief The ACM kernel module's driver data items
 *
 * This section lists all driver specific  data (in opposite to device specific
 * data).
 *
 * @{
 */

/**
 * @def ACMDRV_NAME
 * @brief ACM driver name
 *
 * Name of the kernel module
 */
#define ACMDRV_NAME	"acm"

/**
 * @def ACMDRV_DRIVER_VERSION_FILE
 * @brief Full path to the driver's version number file
 */
#define ACMDRV_DRIVER_VERSION_FILE "/sys/module/" ACMDRV_NAME "/version"

/**@} acmdrvdata */

/******************************************************************************/
/**
 * @defgroup acmdevtree ACM Devicetree
 * @brief The ACM kernel module's devicetree configuration
 *
 * The driver can be adapted to the IP using OF devicetree mechanisms.
 * The following properties must be set:
 *   - compatible: must be set to "ttt,acm-1.0" for ACM IP < 0.9.28
 *     or "ttt,acm-2.0" for ACM IP >= 0.9.28
 *   - reg-names: a list of memory regions that corresponds to to the IP's
 *     structure:
 *     - "CommonRegister": the Common Register Block (<b>ACM IP >= 0.9.28
 *       only</b>)
 *     - "Bypass0": bypass module from port 1 to 0
 *     - "Bypass1": bypass module from port 0 to 1
 *     - "Redundancy": the ACM redundancy module
 *     - "Messagebuffer": the message buffer section for data transfer
 *     - "Scheduler": the network scheduler section
 *   - reg: the memory regions corresponding to reg.names
 *   - reset-gpios: list of reset-out an reset in gpios (<b>ACM IP < 0.9.28
 *     only</b>)
 *   - ports: a list of phandle of the phys associated with the respective
 *            bypass module
 *   - ptp_worker: a phandle to the DEIPCE FRTC used as PTP worker clock
 *
 * Devicetree examples:
 *
 * for ACM IP 0.9.27:
 *
 *     ngn {
 *         compatible = "ttt,acm-1.0";
 *         reg-names = "Bypass0",
 *                     "Bypass1",
 *                     "Redundancy",
 *                     "Messagebuffer",
 *                     "Scheduler";
 *         reg = <0xc0220000 0x10000>,
 *               <0xc0200000 0x10000>,
 *               <0xc0240000 0x10000>,
 *               <0xc0280000 0x40000>,
 *               <0xc0260000 0x20000>;
 *         reset-gpios = <&fpga_gpo 2 GPIO_ACTIVE_HIGH>,
 *                       <&fpga_gpi 2 GPIO_ACTIVE_HIGH>;
 *         ports = <&phy0>,<&phy1>;
 *     };
 *
 * for ACM IP >= 0.9.28:
 *
 *     ngn {
 *         compatible = "ttt,acm-2.0";
 *         reg-names = "CommonRegister"
 *                     "Bypass0",
 *                     "Bypass1",
 *                     "Redundancy",
 *                     "Messagebuffer",
 *                     "Scheduler";
 *         reg = <0xc0200000 0x00300>,
 *               <0xc0230000 0x10000>,
 *               <0xc0210000 0x10000>,
 *               <0xc0250000 0x10000>,
 *               <0xc0280000 0x40000>,
 *               <0xc0260000 0x20000>;
 *         ports = <&phy0>,<&phy1>;
 *     };
 *
 * The compatible string version determines the provided functionality and/or
 * the provided interface which always must be used together with an appropriate
 * ACM IP version.
 *
 * - ttt,acm-1.0: basic set of interfaces, gpio supported reset only. Typically
 *                used for ACM IP <= 0.9.27
 * - ttt,acm-2.0: adds common register block, thus changing the status
 *                interface. Reset is register based. Typically used since
 *                ACM IP >= 0.9.28
 * - ttt,acm-3.0: additional status registers accompanied by changes to already
 *                existing registers. Used with ACM IP >= 0.9.31
 * - ttt,acm-4.0: configuration read back option to disable reading of
 *                configuration and provision of diagnostic data.
 *                Used with ACM IP >= 1.1.0
 *
 * @{
 *
 */
/**@} acmmodparam */
/******************************************************************************/
/**
 * @defgroup acmmodparam ACM Module Parameters
 * @brief The ACM kernel module's module parameters
 *
 * Certain aspects of the ACM kernel module can be configured by module
 * parameters at module load time.
 *
 * @{
 *
 */
/**@} acmmodparam */

/**@} acmapi*/

#endif /* ACMDRV_H_ */
