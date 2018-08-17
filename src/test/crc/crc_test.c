#include <stdint.h>
#include <CUnit/CUnit.h>

#include "crc.h"
#include "crc_test.h"

void zero_test(void)
{
    uint8_t data = 0;
    CU_ASSERT_EQUAL(crc7(&data, 1), 0);
}

void null_vector_test(void)
{
    CU_ASSERT_EQUAL(crc7(NULL, 0), 0);
}

void cmd0_test(void)
{
    uint8_t data[] = { 0x40, 0, 0, 0, 0 };
    CU_ASSERT_EQUAL(crc7(data, sizeof(data)/sizeof(*data)), 0x4A);
}

void cmd8_test(void)
{
    uint8_t data[] = { 0x48, 0, 0, 1, 0xa5 };
    CU_ASSERT_EQUAL(crc7(data, sizeof(data)/sizeof(*data)), 0x34);
}

void cmd29_test(void)
{
    uint8_t data[] = { 0x69, 0x40, 0, 0, 0 };
    CU_ASSERT_EQUAL(crc7(data, sizeof(data)/sizeof(*data)), 0x3b);
}

void add_crc_test_suite(void)
{

    CU_pSuite crc_suite = CU_add_suite(TEST_SUITE_NAME, NULL, NULL);
    CU_ADD_TEST(crc_suite, zero_test);
    CU_ADD_TEST(crc_suite, null_vector_test);
    CU_ADD_TEST(crc_suite, cmd0_test);
    CU_ADD_TEST(crc_suite, cmd8_test);
    CU_ADD_TEST(crc_suite, cmd29_test);
}
