#ifndef MEXT2_COMMAND_H
#define MEXT2_COMMAND_H

#include "common.h"

#define COMMAND_SOFTWARE_RESET      (uint8_t)0x00    //CMD0
#define COMMAND_CHECK_VOLTAGE       (uint8_t)0x08    //CMD8
#define COMMAND_READ_OCR            (uint8_t)0x3A    //CMD58
#define COMMAND_BEFORE_INIT_PROCESS (uint8_t)0x37    //CMD55
#define COMMAND_INIT_PROCESS        (uint8_t)0x29    //ACMD41
#define COMMAND_READ_CSD            (uint8_t)0x09    //CMD09

#define COMMAND_READ_SINGLE_BLOCK       (uint8_t)0x11   //CMD17
#define COMMAND_READ_MULTIPLE_BLOCK     (uint8_t)0x12   //CMD18
#define COMMAND_WRITE_SINGLE_BLOCK      (uint8_t)0x18   //CMD24
#define COMMAND_WRITE_MULTIPLE_BLOCK    (uint8_t)0x19   //CMD25
#define COMMAND_STOP_READ_DATA          (uint8_t)0x0c   //CMD12

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

struct mext2_response
{
    uint8_t r1;
    uint8_t ocr[OCR_REGISTER_LENGTH];
} __mext2_packed;
typedef struct mext2_response mext2_response;

#define R1_IN_IDLE_STATE (uint8_t)0x01
#define R1_ILLEGAL_COMMAND (uint8_t)0x04

struct mext2_command
{
    uint8_t index;
    uint8_t argument[COMMAND_ARGUMENT_SIZE];
    uint8_t crc;
} __mext2_packed;
typedef struct mext2_command mext2_command;

#define COMMAND_SIZE ((uint8_t)sizeof(mext2_command) / sizeof(uint8_t))

uint8_t calc_command_number(uint8_t number);
mext2_command set_command(uint8_t command_name, uint8_t command_argument[COMMAND_ARGUMENT_SIZE]);
mext2_response* send_command(mext2_command* command, mext2_response_type response_type);
uint8_t wait_for_response(uint8_t* buffer);
void wait_8_clock_cycles(uint8_t* buffer);

#endif