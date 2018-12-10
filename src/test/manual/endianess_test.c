#include "helpers.h"

int main(void)
{
    mext2_set_log_level(INFO);
    if(mext2_is_big_endian())
    {
        mext2_log("Big endian");
    }
    else
    {
        mext2_log("Little endian");
    }
}
