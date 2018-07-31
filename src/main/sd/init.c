#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"

static bool sd_card_initialized = false;

uint8_t csd_register[16];

static uint8_t sd_state;

#define SD_NOT_DETERMINED   0
#define SD_V1X              1
#define SD_V2X              2
#define SD_V2XHCXC          3

uint8_t sd_version = SD_NOT_DETERMINED;

#define N_CYCLES_TIMEOUT        (uint16_t) 512

#define COMMAND_SIZE            ((uint8_t)sizeof(mext2_command) / sizeof(uint8_t))

#define SD_INIT_OK              (uint8_t)0x00
#define SD_INIT_NOK             (uint8_t)0x01

#define SD_NOT_INITIALIZED      (uint8_t)0x02
#define SD_IDLE                 (uint8_t)0x03
#define SD_INITIALIZED          (uint8_t)0x04
#define SD_START_INIT           (uint8_t)0x05
#define SD_ERROR                (uint8_t)0x06

#define SD_SEND_COMMAND         (uint8_t)0x07

#define SD_PROCESS_CMDO             (uint8_t)0x08
#define SD_PROCESS_CMD8             (uint8_t)0x18
#define SD_PROCESS_CMD58            (uint8_t)0x28
#define SD_PROCESS_CMD55            (uint8_t)0x38
#define SD_PROCESS_ACMD41           (uint8_t)0x48
#define SD_PROCESS_CMD9             (uint8_t)0x58
#define SD_PROCESS_CMD58_OCR        (uint8_t)0x68

#define COMMAND_SOFTWARE_RESET      (uint8_t)0x00    //CMD0
#define COMMAND_CHECK_VOLTAGE       (uint8_t)0x08    //CMD8
#define COMMAND_READ_OCR            (uint8_t)0x3A    //CMD58
#define COMMAND_BEFORE_INIT_PROCESS (uint8_t)0x37    //CMD55
#define COMMAND_INIT_PROCESS        (uint8_t)0x29    //ACMD41
#define COMMAND_READ_CSD            (uint8_t)0x09    //CMD09


#define R1_IDLE_STATE (uint8_t)0x01
#define R1_ILLEGAL_COMMAND (uint8_t)0x04


uint8_t init()
{
    sd_state = SD_IDLE;
    mext2_command* command = (mext2_command*)malloc(sizeof(mext2_command));

    uint8_t sd_next_command;
    uint8_t buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    while(1)
    {
        switch(sd_state) 
        {
            case SD_IDLE:
            {
                if(sd_card_initialized) sd_state = SD_INITIALIZED;
                    else sd_state = SD_START_INIT;
                break;
            }

            case SD_START_INIT:
            {
                if(mext2_configure_pins() == 1)
                {
                    debug("Error: pin initialization failed\n");
                    sd_state = SD_ERROR;
                }
                else
                {
                    usleep(1000);  //wait 1ms
                    digitalWrite(MEXT2_MOSI, MEXT2_HIGH);

                    for(uint8_t i = 0; i < 74; i++) //74 dummy clock cycles
                    {
                        digitalWrite(MEXT2_SCLK, MEXT2_HIGH);
                        digitalWrite(MEXT2_SCLK, MEXT2_LOW);
                    }

                    command.index = calc_command_number(COMMAND_SOFTWARE_RESET);
                    command.argument[] = {0x00, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD0;
                }
                break;
            }

            case SD_SEND_COMMAND:
            {
                mext2_spi_read_write(command, COMMAND_SIZE);

                buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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
                    sd_state = SD_ERROR;
                }
                else
                {
                    sd_state = sd_next_command;
                }

                break;
            }

            case SD_PROCESS_CMD0:
            {
                if(buffer[0] != R1_IDLE_STATE)
                {
                    debug("Error: not an SD card\n");
                    mext2_reset_pins();
                    sd_state = SD_ERROR;
                }
                else
                {
                    buffer[0] = 0xff;
                    mext2_spi_read_write(buffer, 1);
                    
                    //set CMD8
                    command.index = calc_command_number(COMMAND_CHECK_VOLTAGE);
                    command.argument[] = {0x00, 0x00, 0x01, 0xaa};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD8;
                }
            }

            case SD_PROCESS_CMD8:
            {
                if(buffer[0] & R1_ILLEGAL_COMMAND)
                {
                    sd_version = SD_V1X;
                }
                else
                {
                    mext2_spi_read_write(&buffer[1], 4);
                }

                buffer[0] = 0xff;
                mext2_spi_read_write(buffer,1)

                //set CMD58
                command.index = calc_command_number(COMMAND_READ_OCR);
                command.argument[] = {0x00, 0x00, 0x00, 0x00};
                command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                sd_state = SD_SEND_COMMAND;
                sd_next_command = SD_PROCESS_CMD58;

                break;
            }

            case SD_PROCESS_CMD58:
            {
                if(buffer[0] & R1_ILLEGAL_COMMAND)
                {
                    debug("Error: not an SD card\n")
                    mext2_reset_pins();
                    sd_state = SD_ERROR;
                }
                else
                {
                    for(uint8_t i = 0; i < 5; i++) buffer[i] = 0xff;
                    mext2_spi_read_write(buffer, 5);

                    //set CMD55
                    command.index = calc_command_number(COMMAND_BEFORE_INIT_PROCESS);
                    command.argument[] = {0x00, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD55;
                }
            }

            case SD_PROCESS_CMD55:
            {
                if(buffer[0] & R1_ILLEGAL_COMMAND)
                {
                    debug("Error: not an SD card\n")
                    mext2_reset_pins();
                    sd_state = SD_ERROR;
                }
                else
                {
                    buffer[0] = 0xff;
                    mext2_spi_read_write(buffer, 1);

                    //set ACMD41
                    command.index = calc_command_number(COMMAND_INIT_PROCESS);
                    command.argument[] = {0x40, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_ACMD41;
                }
            }

            case SD_PROCESS_ACMD41:
            {
                if(buffer[0] & R1_IDLE_STATE)
                {
                    buffer[0] = 0xff;
                    mext2_spi_read_write(buffer, 1);

                    //set CMD55
                    command.index = calc_command_number(COMMAND_BEFORE_INIT_PROCESS);
                    command.argument[] = {0x00, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD55;
                }
                else if(sd_version != SD_V1X)
                {
                    //set CMD58
                    command.index = calc_command_number(COMMAND_READ_OCR);
                    command.argument[] = {0x00, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD58_OCR;
                } else
                {
                    buffer[0] = 0xff;
                    mext2_spi_read_write(buffer, 1);

                    //set CMD9
                    command.index = calc_command_number(COMMAND_READ_CSD);
                    command.argument[] = {0x00, 0x00, 0x00, 0x00};
                    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                    sd_state = SD_SEND_COMMAND;
                    sd_next_command = SD_PROCESS_CMD9;
                }
                break;
            }

            case SD_PROCESS_CMD58_OCR:
            {
                for(uint8_t i = 1; i < 5; i++) buffer[i] = 0xff;
                mext2_spi_read_write(&buffer[1], 4);

                if(buffer[1] & (uint8_t)0x40)
                {
                    sd_version = SD_V2XHCXC;
                }
                else
                {
                    sd_version = SD_V2X;
                }

                buffer[0] = 0xff;
                mext2_spi_read_write(buffer, 1);

                //set CMD9
                command.index = calc_command_number(COMMAND_READ_CSD);
                command.argument[] = {0x00, 0x00, 0x00, 0x00};
                command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

                sd_state = SD_SEND_COMMAND;
                sd_next_command = SD_PROCESS_CMD9;

                break;
            }

            case SD_PROCESS_CMD9:
            {
                if(buffer[0] != 0)
                {
                    debug("Error: reading CSD register failed\n")
                }
                else
                {
                    buffer[0] = 0xff;
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
                        sd_state = SD_ERROR;
                    }

                    memset(csd_register, 0xff, 16);
                    mext2_spi_read_write(csd_register, 16);
                    buffer[0] = 0xff;
                    buffer[1] = 0xff;
                    mext2_spi_read_write(buffer, 2);
                }

                buffer[0] = 0xff;
                mext2_spi_read_write(buffer,1)

                sd_state = SD_INITIALIZED;
                break;
            }

            case SD_ERROR:
            {
                free(command);
                return SD_INIT_NOK;
            }

            case SD_INITIALIZED:
            {
                free(command);
                return SD_INIT_OK;
            }
            default:
            {   sd_state = SD_ERROR;
                break;
            }
        }

    }
}