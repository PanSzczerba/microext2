#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>

#define __packed __attribute__((packed))

#define OCR_REGISTER_LENGTH 4
#define N_CYCLES_TIMEOUT        (uint16_t) 512

typedef enum mext2_response_type
{
    MEXT2_R1,
    MEXT2_R2,
    MEXT2_R3,
    MEXT2_R7
} mext2_response_type;

struct mext2_command
{
	uint8_t index;
	uint8_t argument[4];
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

//uint8_t mext2_send_command(uint8_t command_no, uint32_t command_arg, uint8_t* response, uint8_t response_length);
uint8_t calc_command_number(uint8_t number);
uint8_t calc_crc7(uint8_t* buffer, uint8_t count) ;
#endif
