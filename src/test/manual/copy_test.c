#include "helpers.h"

#define BUFFER_SIZE 90000
mext2_sd sd;

int main(void)
{
    mext2_set_log_level(WARNING);
    mext2_sd sd;
    if(mext2_sd_init(&sd) != MEXT2_RETURN_SUCCESS)
        return 1;

    mext2_file* fd1;
    mext2_file* fd2;
    if((fd1 = mext2_open(&sd, "/a/b", MEXT2_READ)) != NULL &&
       (fd2 = mext2_open(&sd, "/a/c", MEXT2_WRITE | MEXT2_TRUNCATE)) != NULL)
    {
        printf("Successfully opened /a/b and created /a/c files\n");
        char buffer[BUFFER_SIZE];
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        long long unsigned all_bytes_read = 0;
        long long unsigned all_bytes_written = 0;
        while(!mext2_eof(fd1))
        {
            bytes_read = mext2_read(fd1, buffer, BUFFER_SIZE);
            all_bytes_read += bytes_read;
            if(mext2_errno != MEXT2_NO_ERRORS && mext2_errno != MEXT2_EOF)
            {
                return 1;
            }
            bytes_written = mext2_write(fd2, buffer, bytes_read);
            all_bytes_written += bytes_written;
            if(mext2_errno != MEXT2_NO_ERRORS)
            {
                return 1;
            }
        }
        printf("Read %llu bytes, written %llu bytes\n", all_bytes_read, all_bytes_written);
        printf("Reached end of the file, closing\n");

        if(mext2_close(fd1) != MEXT2_RETURN_SUCCESS)
            printf("Cannot close fd1\n");
        if(mext2_close(fd2) != MEXT2_RETURN_SUCCESS)
            printf("Cannot close fd2\n");
    }
    else
        return 1;

}
