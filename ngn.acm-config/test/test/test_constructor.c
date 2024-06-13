#include "unity.h"

#include "constructor.h"
#include "memory.h"
#include "sysfs.h"
#include "logging.h"
#include "tracing.h"

#include <string.h>
#include <stdlib.h>
#include "setup_helper.h"
#include "mock_stream.h"
#include "mock_status.h"

void __attribute__((weak)) suite_setup(void)
{
}

/* !important!
 * It is not possible to stub anything that con() calls, because con() is the
 * constructor and linked to the very beginning and thus executed once before
 * the test is started at all.
 * */

void setUp(void)
{
    setup_sysfs();
}

void tearDown(void)
{
    teardown_sysfs();
}

void test_con(void) {
    con();
    system("rm -f " CONFIG_FILE);
    con();
}
