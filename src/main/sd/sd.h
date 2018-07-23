#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>

#define __packed __attribute__((packed))
#define MAX_RESPONSE_LENGTH 5

enum mext2_response_type
{
    MEXT2_R1,
    MEXT2_R2,
    MEXT2_R3,
};

enum mext2_command
{
	uint8_t index;
	uint8_t argument[4];
	uint8_t crc;
} __packed;

struct mext2_response
{
    uint8_t response[MAX_RESPONSE_LENGTH];
    uint8_t response_type;
} __packed;

uint8_t mext2_send_command(uint8_t command_no, uint32_t command_arg, uint8_t* response, uint8_t response_length);
uint8_t calc_command_number(uint8_t number);
#endif
