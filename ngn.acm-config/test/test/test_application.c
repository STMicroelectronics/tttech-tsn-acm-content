#include "unity.h"

#include "application.h"
#include "tracing.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "mock_memory.h"
#include "mock_sysfs.h"
#include "mock_module.h"
#include "mock_logging.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_apply_configuration_neg_clear_fpga(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    uint32_t config_id = 7711;

    write_clear_all_fpga_ExpectAndReturn(-ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_init_delays(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof(configuration));
    uint32_t config_id = 7711;

    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0,
            NULL);
    logging_Expect(0, "Application: problem writing fallback delays for parallel mode");
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_apply_configuration_neg_config_start(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    uint32_t config_id = 7711;

    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &module);
    module_destroy_Expect(&module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_buffer_desc(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    uint32_t config_id = 7711;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &module);
    module_destroy_Expect(&module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_buffer_alias(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    uint32_t config_id = 7711;

    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &module);
    module_destroy_Expect(&module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_module(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[0] = &module;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}
void test_apply_configuration_neg_base_recovery(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof(configuration));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof(dummy_module));
    struct acm_module module1, module2;
    memset(&module1, 0, sizeof(module1));
    memset(&module2, 0, sizeof(module2));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[0] = &module1;
    configuration.bypass[1] = &module2;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0,
            &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs,
                BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs,
            BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module1, 0);
    write_module_data_to_HW_ExpectAndReturn(&module2, 0);
    sysfs_write_base_recovery_ExpectAndReturn(&configuration, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_config_id(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    struct acm_module module1, module2;
    memset(&module1, 0, sizeof (module1));
    memset(&module2, 0, sizeof (module2));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[0] = &module1;
    configuration.bypass[1] = &module2;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module1, 0);
    write_module_data_to_HW_ExpectAndReturn(&module2, 0);
    sysfs_write_base_recovery_ExpectAndReturn(&configuration, 0);
    sysfs_write_configuration_id_ExpectAndReturn(config_id, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_config_end(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    struct acm_module module1;
    memset(&module1, 0, sizeof (module1));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[1] = &module1;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module1, 0);
    //write_module_data_to_HW_ExpectAndReturn(&module2, 0);
    sysfs_write_base_recovery_ExpectAndReturn(&configuration, 0);
    sysfs_write_configuration_id_ExpectAndReturn(config_id, 0);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_END_STATE, -ENOMEM);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_apply_configuration_neg_apply_schedule(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    struct acm_module module1;
    memset(&module1, 0, sizeof (module1));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[1] = &module1;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module1, 0);
    //write_module_data_to_HW_ExpectAndReturn(&module2, 0);
    sysfs_write_base_recovery_ExpectAndReturn(&configuration, 0);
    sysfs_write_configuration_id_ExpectAndReturn(config_id, 0);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_END_STATE, 0);
    write_module_schedule_to_HW_ExpectAndReturn(&module1, -EACMNOFREESCHEDTAB);
    logging_Expect(0, "Config: applying schedule to HW failed");
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(-EACMNOFREESCHEDTAB, result);
}

void test_apply_configuration(void) {
    int result;
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    struct acm_module module1;
    memset(&module1, 0, sizeof (module1));
    uint32_t config_id = 7711;

    /* prepare test */
    configuration.bypass[1] = &module1;

    /* execute test */
    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_START_STATE, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_DESC, 0);
    sysfs_write_msg_buff_to_HW_ExpectAndReturn(&configuration.msg_buffs, BUFF_ALIAS, 0);
    write_module_data_to_HW_ExpectAndReturn(&module1, 0);
    //write_module_data_to_HW_ExpectAndReturn(&module2, 0);
    sysfs_write_base_recovery_ExpectAndReturn(&configuration, 0);
    sysfs_write_configuration_id_ExpectAndReturn(config_id, 0);
    sysfs_write_config_status_to_HW_ExpectAndReturn(ACMDRV_CONFIG_END_STATE, 0);
    write_module_schedule_to_HW_ExpectAndReturn(&module1, 0);
    result = apply_configuration(&configuration, config_id);
    TEST_ASSERT_EQUAL(0, result);
}

void test_remove_configuration(void) {
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof (dummy_module));
    int result;

    write_clear_all_fpga_ExpectAndReturn(0);
    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_100MBps, MODULE_0, &dummy_module);
    module_destroy_Expect(&dummy_module);
    result = remove_configuration();
    TEST_ASSERT_EQUAL(0, result);
}

void test_remove_configuration_neg_clear_fpga(void) {
    struct acm_module dummy_module;
    memset(&dummy_module, 0, sizeof(dummy_module));
    int result;

    write_clear_all_fpga_ExpectAndReturn(-ENOMEM);
    result = remove_configuration();
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}
