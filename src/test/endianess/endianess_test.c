#include "endianess_test.h"
#include "stubs/endianess.h"

#include <stdint.h>
#include <CUnit/CUnit.h>

void u16_test(void)
{
    // test variables
    uint16_t result;
    uint16_t expected_result;
    uint16_t number;

    // test preconditions
    number = 0x0102;
    expected_result = 0x0201;

    // action
    result = mext2_flip_endianess16(number);


     // result
    CU_ASSERT_EQUAL(result, expected_result);

}

void u32_test(void)
{
    // test variables
    uint32_t result;
    uint32_t expected_result;
    uint32_t number;

    // test preconditions
    number = 0x01020304;
    expected_result = 0x04030201;

    // action
    result = mext2_flip_endianess32(number);

     // result
    CU_ASSERT_EQUAL(result, expected_result);

}

void u64_test(void)
{
    // test variables
    uint64_t result;
    uint64_t expected_result;
    uint64_t number;

    // test preconditions
    number = 0x0102030405060708;
    expected_result = 0x0807060504030201;

    // action
    result = mext2_flip_endianess64(number);

     // result
    CU_ASSERT_EQUAL(result, expected_result);

}

void add_endianess_test_suite(void)
{

    CU_pSuite endianess_suite = CU_add_suite(TEST_SUITE_NAME, NULL, NULL);
    CU_ADD_TEST(endianess_suite, u16_test);
    CU_ADD_TEST(endianess_suite, u32_test);
    CU_ADD_TEST(endianess_suite, u64_test);
}
