#include <stdint.h>
#include <CUnit/CUnit.h>

#include "stubs/crc.h"
#include "crc_test.h"

void zero_test(void)
{
    // test variables
    uint8_t result;
    uint8_t expected_result;
    uint8_t data;
    size_t data_length;

    // test preconditions
    data = 0;
    data_length = 1;
    expected_result = 0;

    // action
    result = crc7(&data, data_length);

     // result
    CU_ASSERT_EQUAL(result, expected_result);
}

void null_vector_test(void)
{
    // test variables
    uint8_t result;
    uint8_t expected_result;
    uint8_t* data;
    size_t data_length;

    // test preconditions
    data = NULL;
    data_length = 0;
    expected_result = 0;

    // action
    result = crc7(data, data_length);

     // result
    CU_ASSERT_EQUAL(result, expected_result);
}

void cmd0_test(void)
{
    // test variables
    uint8_t result;
    uint8_t expected_result;
    size_t data_length;

    // test preconditions
    uint8_t data[] = { 0x40, 0, 0, 0, 0 };
    data_length = 5;
    expected_result = 0x4A;

    // action
    result = crc7(data, data_length);

     // result
    CU_ASSERT_EQUAL(result, expected_result);
}

void cmd8_test(void)
{
    // test variables
    uint8_t result;
    uint8_t expected_result;
    size_t data_length;

    // test preconditions
    uint8_t data[] = { 0x48, 0, 0, 1, 0xa5 };
    data_length = 5;
    expected_result = 0x34;

    // action
    result = crc7(data, data_length);

     // result
    CU_ASSERT_EQUAL(result, expected_result);
}

void cmd29_test(void)
{
    // test variables
    uint8_t result;
    uint8_t expected_result;
    size_t data_length;

    // test preconditions
    uint8_t data[] = { 0x69, 0x40, 0, 0, 0 };
    data_length = 5;
    expected_result = 0x3b;

    // action
    result = crc7(data, data_length);

     // result
    CU_ASSERT_EQUAL(result, expected_result);
}

void add_endianess_test_suite(void)
{

    CU_pSuite crc_suite = CU_add_suite(TEST_SUITE_NAME, NULL, NULL);
    CU_ADD_TEST(crc_suite, zero_test);
    CU_ADD_TEST(crc_suite, null_vector_test);
    CU_ADD_TEST(crc_suite, cmd0_test);
    CU_ADD_TEST(crc_suite, cmd8_test);
    CU_ADD_TEST(crc_suite, cmd29_test);
}
