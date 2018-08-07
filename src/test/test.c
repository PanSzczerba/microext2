#include <stdio.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/CUCurses.h>

#include "crc/crc_test.h"


int main(void)
{
    if(CU_initialize_registry() != CUE_SUCCESS)
    {
        fprintf(stderr, "Error: Could not initialize CUnit registry");
        return EXIT_FAILURE;
    }

    add_crc_test_suite();

    CU_curses_run_tests();

// CLEANUP
    CU_cleanup_registry();
    return EXIT_SUCCESS;
}
