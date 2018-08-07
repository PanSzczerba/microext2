#include <stdint.h>
#include <CUnit/CUnit.h>

#include "crc.h"
#include "crc_test.h"

void add_crc_test_suite(void)
{
    char crc_suite_name[] = "crc";
    CU_pSuite crc_suite = CU_add_suite(crc_suite_name, NULL, NULL);
    CU_ADD_TEST(crc_suite, cmd0_test);
}

void cmd0_test(void)
{
    uint8_t data[] = { 0x40, 0, 0, 0, 0 };
    CU_ASSERT_EQUAL(crc7(data, sizeof(data)/sizeof(*data)), 0x4A);
}

