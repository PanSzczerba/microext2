#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"

static bool sd_card_initialized = false;

static uint8_t sd_init_state;


#define N_CYCLES_TIMEOUT 		(uint16_t) 512

#define COMMAND_SIZE 			((uint8_t)sizeof(mext2_command) / sizeof(uint8_t))

#define SD_INIT_OK 				(uint8_t)0x00
#define SD_INIT_NOK 			(uint8_t)0x01

#define SD_NOT_INITIALIZED 		(uint8_t)0x02
#define SD_IDLE 				(uint8_t)0x03
#define SD_INITIALIZED 			(uint8_t)0x04
#define SD_START_INIT 			(uint8_t)0x05
#define SD_ERROR				(uint8_t)0x06

#define SD_SEND_COMMAND			(uint8_t)0x07

#define SD_SET_CMDO				(uint8_t)0x08
#define SD_SET_CMD8				(uint8_t)0x18
#define SD_SET_CMD58			(uint8_t)0x28
#define SD_SET_CMD55			(uint8_t)0x38
#define SD_SET_ACMD41			(uint8_t)0x48
#define SD_SET_CMD9				(uint8_t)0x58

#define SD_GET_RESULT_CMD0		(uint8_t)0x09
#define SD_GET_RESULT_CMD8		(uint8_t)0x19
#define SD_GET_RESULT_CMD58		(uint8_t)0x29
#define SD_GET_RESULT_CMD55		(uint8_t)0x39
#define SD_GET_RESULT_ACMD41 	(uint8_t)0x49
#define SD_GET_RESULT_CMD9		(uint8_t)0x59


#define COMMAND_SOFTWARE_RESET 		(uint8_t)0x00 	//CMD0
#define COMMAND_CHECK_VOLTAGE		(uint8_t)0x08	//CMD8
#define COMMAND_READ_OCR			(uint8_t)0x3A	//CMD58
#define COMMAND_BEFORE_INIT_PROCESS	(uint8_t)0x37	//CMD55
#define COMMAND_INIT_PROCESS 		(uint8_t)0x29	//ACMD41
#define COMMAND_READ_CSD			(uint8_t)0x09	//CMD09


#define R1_IDLE_STATE (uint8_t)0x01
#define R1_ILLEGAL_COMMAND (uint8_t)0x04



uint8_t init()
{
	sd_init_state = SD_IDLE;
	mext2_command* command = (mext2_command*)malloc(sizeof(mext2_command));

	uint8_t sd_command_result;

	while(1)
	{
		switch(sd_init_state) 
		{
			case SD_IDLE:
			{
				if(sd_card_initialized) sd_init_state = SD_INITIALIZED;
				else sd_init_state = SD_START_INIT;
				break;
			}

			case SD_START_INIT:
			{
				if(mext2_configure_pins() == 1)
				{
					debug("Error: pin initialization failed\n");
					sd_init_state = SD_ERROR;
				}
				else
				{
					usleep(1000);  //wait 1ms
					digitalWrite(MEXT2_MOSI, MEXT2_HIGH);

					for(uint8_t i = 0; i < 74; i++) //74 dummy clock cycles
					{
						digitalWrite(MEXT2_SCLK, MEXT2_HIGH);
						digitalWrite(MEXT2_SCLK, MEXT2_LOW);

						sd_init_state = SD_SET_CMDO;
					}
				}
				break;
			}

			case SD_SET_CMDO:
			{
				command.index = calc_command_number(COMMAND_SOFTWARE_RESET);
				command.argument[] = {0x00, 0x00, 0x00, 0x00};
				command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

				sd_init_state = SD_SEND_COMMAND;
				sd_command_result = SD_GET_RESULT_CMD0;

				break;
			}

			case SD_SET_CMD8:
			{
				command.index = calc_command_number(COMMAND_CHECK_VOLTAGE);
				command.argument[] = {0x00, 0x00, 0x01, 0xaa};
				command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

				sd_init_state = SD_SEND_COMMAND;
				sd_command_result = SD_GET_RESULT_CMD8;

				break; 
			}

			case SD_SEND_COMMAND:
			{
				mext2_spi_read_write(command, COMMAND_SIZE);

				uint8_t buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

				uint16_t i;
				for(i; i < N_CYCLES_TIMEOUT && buffer[0] == 0xff; i++)
				{
					buffer[0] = 0xff;
					mext2_spi_read_write(buffer, 1)
				}

				if(i == N_CYCLES_TIMEOUT)
				{
					debug("Error: exceeded time limit waiting for response, check your SD card reader device.\n");
					mext2_reset_pins();
					sd_init_state = SD_ERROR;
				}
				else
				{
					sd_init_state = sd_command_result;
				}

				break;
			}

			case SD_GET_RESULT_CMD0:
			{
				if(buffer[0 != R1_IDLE_STATE])
				{
					debug("Error: not an SD card\n");
					mext2_reset_pins();
					sd_init_state = SD_ERROR;
				}
				else
				{
					sd_init_state = SD_SET_CMD8;
				}
			}

			case SD_ERROR:
			{
				//free command
				return SD_INIT_NOK;
			}

			case SD_INITIALIZED:
			{
				// free command
				return SD_INIT_OK;
			}
			default:
			{
				break;
			}
		}

	}
}