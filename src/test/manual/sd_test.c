#include "helpers.h"

#define MBR_BLOCK 0
#define MBR_BLOCK_SIZE 1

int main(void)
{
    mext2_set_log_level(WARNING);
    mext2_sd sd;
    if(mext2_sd_init(&sd) != MEXT2_RETURN_SUCCESS)
        return 1;

    mext2_set_log_level(INFO);
    display_sd_version(&sd);
    display_csd(&sd);
}
