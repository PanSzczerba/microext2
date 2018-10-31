#include <stdio.h>
#include "debug.h"
#include "sd.h"
#include "command.h"

#define MAX_BLOCKS_TO_READ 128
#define BLOCK_SIZE 512

void display_buffer(size_t block_address, block512_t buff)
{
    for(size_t i = 0; i < 32; i++)
    {
        printf("%03x: ", block_address + i*16);
        for(size_t j = 0; j < 16; j++)
            printf("%02x ", buff.data[i * 16 + j]);
        printf("\n");
    }
}

void display_blocks(mext2_sd* sd, size_t block_address, size_t block_no)
{
    block512_t* buff;
    buff = (block512_t*)malloc(sizeof(block512_t)*block_no);
    mext2_read_blocks(sd, block_address, buff, block_no);
    for(size_t k = 0; k < block_no; k++)
    {
        for(size_t i = 0; i < 32; i++)
        {
            printf("%03x: ", block_address + k*BLOCK_SIZE + i*16);
            for(size_t j = 0; j < 16; j++)
                printf("%02x ", buff[k].data[i * 16 + j]);
            printf("\n");
        }
    }
}

void display_csd(mext2_sd* sd)
{
    for(int i = 0; i < sizeof(sd->csd)/sizeof(sd->csd[0]); i++)
    {
        printf("0x%hhx ", sd->csd[i]);
    }
    printf("\n");
}


mext2_sd sd;

int main(void)
{
    int return_value = mext2_sd_init(&sd);
    mext2_set_log_level(INFO);
    switch(return_value)
    {
    case MEXT2_RETURN_FAILURE:
        printf("SD initialization failure\n");
        return EXIT_FAILURE;
    case MEXT2_RETURN_SUCCESS:
        printf("SD initialization success\n");
        break;
    }
    printf("%d\n", sd.sd_version);
    display_csd(&sd);
    display_blocks(&sd, 0, 1);
}
