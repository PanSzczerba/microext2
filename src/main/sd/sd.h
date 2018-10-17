#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define __packed __attribute__((packed))

#define OCR_REGISTER_LENGTH 4
#define COMMAND_ARGUMENT_SIZE 4
#define N_CYCLES_TIMEOUT        (uint16_t) 512

typedef enum mext2_response_type
{
    MEXT2_R1,
    MEXT2_R1b,
    MEXT2_R2,
    MEXT2_R3,
    MEXT2_R7
} mext2_response_type;

struct mext2_command
{
	uint8_t index;
	uint8_t argument[COMMAND_ARGUMENT_SIZE];
	uint8_t crc;
} __packed;
typedef struct mext2_command mext2_command;

#define COMMAND_SIZE ((uint8_t)sizeof(mext2_command) / sizeof(uint8_t))

struct mext2_response
{
    uint8_t r1;
    uint8_t ocr[OCR_REGISTER_LENGTH];
} __packed;
typedef struct mext2_response mext2_response;

#define R1_IN_IDLE_STATE (uint8_t)0x01
#define R1_ILLEGAL_COMMAND (uint8_t)0x04

typedef struct block512_t
{
    uint8_t data[512];
} block512_t;

uint8_t calc_command_number(uint8_t number);
mext2_response* send_command(mext2_command* command, mext2_response_type response_type);
bool wait_for_response(uint8_t* buffer);
void wait_8_clock_cycles(uint8_t* buffer);
void set_command(mext2_command* command, uint8_t command_name, uint8_t command_argument[COMMAND_ARGUMENT_SIZE]);

uint8_t single_block_wite(uint32_t index, block512_t* block);
uint8_t multiple_block_write(uint32_t index, block512_t* block, uint8_t blocks_number);
#endif
