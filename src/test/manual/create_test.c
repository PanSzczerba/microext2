#include "helpers.h"

#define BUFFER_SIZE 90000
mext2_sd sd;

int main(void)
{
    mext2_set_log_level(WARNING);
    mext2_sd sd;
    if(mext2_sd_init(&sd) != MEXT2_RETURN_SUCCESS)
        return 1;

    mext2_file* fd;
    if((fd = mext2_open(&sd, "/a/b/c/d", MEXT2_WRITE)) != NULL)
    {
        mext2_set_log_level(INFO);
        mext2_log("File was successfully opened");
        return 0;
    }
    else
    {
        mext2_set_log_level(INFO);
        mext2_log("File couldn't  be successfully opened");
        return 1;
    }

}
