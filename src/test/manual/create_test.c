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
    if((fd = mext2_open(&sd, "/a", MEXT2_WRITE | MEXT2_TRUNCATE)) != NULL)
    {
        printf("File was successfully opened\n");
        mext2_close(fd);
        return 0;
    }
    else
    {
        mext2_log("File couldn't be successfully opened\n");
        return 1;
    }

}
