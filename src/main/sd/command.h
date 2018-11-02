#ifndef MEXT2_SD_COMMAND_H
#define MEXT2_SD_COMMAND_H

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

#define EXTENDED_RESPONSE_LENGTH 5
#define COMMAND_ARGUMENT_SIZE 4
#define N_CYCLES_TIMEOUT        (uint16_t)(-1)

struct mext2_response
{
    uint8_t r1;
    uint8_t extended_response[EXTENDED_RESPONSE_LENGTH - 1];
} __mext2_packed;
typedef struct mext2_response mext2_response;

#define R1_INVALID_RESPONSE (uint8_t)0x08
#define R1_IN_IDLE_STATE    (uint8_t)0x01
#define R1_ILLEGAL_COMMAND  (uint8_t)0x04

struct mext2_command
{
    uint8_t index;
    uint8_t argument[COMMAND_ARGUMENT_SIZE];
    uint8_t crc;
} __mext2_packed;
typedef struct mext2_command mext2_command;

#define COMMAND_SIZE ((uint8_t)sizeof(mext2_command) / sizeof(uint8_t))

mext2_response mext2_send_command(uint8_t command_number, uint8_t command_argument[COMMAND_ARGUMENT_SIZE]);
uint8_t wait_for_response(uint8_t* buffer);
void wait_8_clock_cycles();
void wait_8_clock_cycles_with_buffer(uint8_t* buffer);

#endif
