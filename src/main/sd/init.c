#include "sd.h"
#include "debug.h"

static bool sd_card_initialized = false;
static uint8_t sd_init_status;

#define SD_INIT_OK 			(uint8_t)0x00
#define SD_INIT_NOK 		(uint8_t)0x01

#define SD_NOT_INITIALIZED 	(uint8_t)0x02
#define SD_IDLE 			(uint8_t)0x03
#define SD_INITIALIZED 		(uint8_t)0x04
#define SD_START_INIT 		(uint8_t)0x05
#define SD_ERROR			(uint8_t)0x06
#define SD_SOFTWARE_RESET	(uint8_t)0x07

#define COMMAND_SOFTWARE_RESET 		(uint8_t)0x00


uint8_t init()
{
	sd_init_status = SD_INIT_STATUS_IDLE;

	while(1)
	{
		switch(sd_init_status) 
		{
			case SD_IDLE:
			{
				if(sd_card_initialized) sd_init_status = SD_INITIALIZED;
				else sd_init_status = SD_START_INIT;
				break;
			}

			case SD_START_INIT:
			{
				if(mext2_configure_pins() == 1)
				{
					debug("Error: pin initialization failed\n");
					sd_init_status = SD_ERROR;
				}
				else
				{
					usleep(1000);  //wait 1ms
					digitalWrite(MEXT2_MOSI, MEXT2_HIGH);

					for(uint8_t i = 0; i < 74; i++) //74 dummy clock cycles
					{
						digitalWrite(MEXT2_SCLK, MEXT2_HIGH);
						digitalWrite(MEXT2_SCLK, MEXT2_LOW);

						sd_init_status = SD_SOFTWARE_RESET;
					}
				}
				break;
			}

			case SD_SOFTWARE_RESET:
			{
				uint8_t numebr = calc_command_number(COMMAND_SOFTWARE_RESET);

				break;
			}

			case SD_ERROR:
			{
				return SD_INIT_NOK;
			}

			case SD_INITIALIZED:
			{
				return SD_INIT_OK;
			}
			default:
			{
				break;
			}
		}

	}
}