#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>     /* PATH_MAX */
#include <sys/stat.h>   /* mkdir(2) */
#include <libgen.h>     /* dirname(3) */
#include <linux/acm/acmdrv.h>

#include "hwconfig_def.h"

/* recursive mkdir */
static int mkdir_p(const char *path) {
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof (_path) - 1) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1;
            }

            // cppcheck-suppress redundantAssignment
            *p = '/';
        }
    }

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}

static void create_txtfile(const char *name, const char *text) {
    int fd;

    unlink(name);
    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    write(fd, text, strlen(text));
    close(fd);
}

static void create_file(const char *name, size_t size) {
    int fd;
    int i;
    const uint8_t initval = 0x00;

    unlink(name);
    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    for (i = 0; i < size; ++i) {
        write(fd, &initval, 1);
    }

    close(fd);
}

void setup_sysfs(void)
{
    static char *configfile;

    /* setup basic test directory structure */
    mkdir_p(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP));
    mkdir_p(ACMDEV_BASE __stringify(ACMDRV_SYSFS_STATUS_GROUP));
    mkdir_p(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONTROL_GROUP));
    mkdir_p(ACMDEV_BASE __stringify(ACMDRV_SYSFS_ERROR_GROUP));
    mkdir_p(ACMDEV_BASE __stringify(ACMDRV_SYSFS_DIAG_GROUP));
    mkdir_p(DELAY_BASE "sw0p2/phy");
    mkdir_p(DELAY_BASE "sw0p3/phy");
    mkdir_p(DELAY_BASE "sw0p1/phy");
    mkdir_p(DELAY_BASE "sw0ep/phy");
    mkdir_p(DELAY_BASE "br0");

    mkdir_p(MSGBUF_BASE);
    configfile = strdup(CONFIG_FILE);
    mkdir_p(dirname(configfile));
    free(configfile);

    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/clear_all_fpga",
            sizeof(uint32_t));
    create_txtfile(ACMDEV_BASE __stringify(ACMDRV_SYSFS_STATUS_GROUP) "/msgbuf_count", "32");
    create_file(ACMDEV_BASE "/empty_file", 0);
    create_txtfile(DELAY_BASE "sw0p2/phy" "/delay100tx", "440");
    create_txtfile(DELAY_BASE "sw0p3/phy" "/delay100rx", "440");
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/configuration_id",
                sizeof(uint32_t));
    create_file(MSGBUF_BASE "/acmbuf_ngn", 0);
    system("cp -f test/config_acm " CONFIG_FILE);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/msg_buff_desc",
                sizeof(struct acmdrv_buff_desc) * 32);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/msg_buff_alias",
                sizeof(struct acmdrv_buff_alias) * 32);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONTROL_GROUP) "/lock_msg_bufs",
                sizeof(struct acmdrv_msgbuf_lock_ctrl));
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONTROL_GROUP) "/unlock_msg_bufs",
                sizeof(struct acmdrv_msgbuf_lock_ctrl));
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_speed",
                sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/config_state",
                sizeof(uint32_t));
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_connection_mode",
                sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/redund_cnt_tab",
            sizeof(struct acmdrv_redun_ctrl_entry) * ACMDRV_REDUN_CTRLTAB_COUNT
                    * ACMDRV_REDUN_TABLE_ENTRY_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/scatter_dma",
            sizeof(struct acmdrv_bypass_dma_command) * ACMDRV_BYPASS_SCATTER_DMA_CMD_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/gather_dma",
            sizeof(struct acmdrv_bypass_dma_command) * ACMDRV_BYPASS_GATHER_DMA_CMD_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/prefetch_dma",
            sizeof(struct acmdrv_bypass_dma_command) * ACMDRV_BYPASS_PREFETCH_DMA_CMD_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/sched_cycle_time",
            sizeof(struct acmdrv_sched_cycle_time) * ACMDRV_SCHED_TBL_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/sched_start_table",
            sizeof(struct acmdrv_timespec64) * ACMDRV_SCHED_TBL_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/base_recovery",
            sizeof(struct acmdrv_redun_base_recovery) );
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/individual_recovery",
            sizeof(struct acmdrv_redun_individual_recovery) );
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/sched_tab_row",
            sizeof(struct acmdrv_sched_tbl_row ) * ACMDRV_SCHED_TBL_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT );
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/table_status",
            sizeof(struct acmdrv_sched_tbl_status) * ACMDRV_SCHED_TBL_COUNT
                    * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/emergency_disable",
            sizeof(struct acmdrv_sched_emerg_disable) );
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_ngn_enable",
            sizeof(uint32_t) );
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/const_buffer",
            sizeof(struct acmdrv_bypass_const_buffer) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/layer7_mask",
            sizeof(struct acmdrv_bypass_layer7_check) * ACMDRV_BYPASS_MODULES_COUNT
                    * ACM_MAX_LOOKUP_ITEMS);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/layer7_pattern",
            sizeof(struct acmdrv_bypass_layer7_check) * ACMDRV_BYPASS_MODULES_COUNT
            * ACM_MAX_LOOKUP_ITEMS);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/lookup_mask",
            sizeof(struct acmdrv_bypass_lookup) * ACMDRV_BYPASS_MODULES_COUNT
            * ACM_MAX_LOOKUP_ITEMS);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/lookup_pattern",
            sizeof(struct acmdrv_bypass_lookup) * ACMDRV_BYPASS_MODULES_COUNT
            * ACM_MAX_LOOKUP_ITEMS);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/stream_trigger",
            sizeof(struct acmdrv_bypass_stream_trigger) * ACMDRV_BYPASS_MODULES_COUNT
            * (ACM_MAX_LOOKUP_ITEMS + 1));
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_ingress_policing_control",
            sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_ingress_policing_enable",
            sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_layer7_enable",
            sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_layer7_length",
            sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_file(ACMDEV_BASE __stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/cntl_lookup_enable",
            sizeof(uint32_t) * ACMDRV_BYPASS_MODULES_COUNT);
    create_txtfile(ACMDEV_BASE "read_uint64_sysfs_item_test", "0");
}

void teardown_sysfs(void) {
    system("rm -rf build/test/sysroot");
}
