#include <stdio.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "test.h"

#include "endianess_test.h"

int main(void)
{
    if(CU_initialize_registry() != CUE_SUCCESS)
    {
        fprintf(stderr, "Error: Could not initialize CUnit registry");
        return EXIT_FAILURE;
    }

    add_endianess_test_suite();

    CU_basic_set_mode(CU_BRM_NORMAL);
    CU_basic_run_tests();

// CLEANUP
    CU_cleanup_registry();
    return EXIT_SUCCESS;
}
