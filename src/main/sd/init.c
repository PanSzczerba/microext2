#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"

static bool sd_card_initialized = false;
uint8_t csd_register[16];
static uint8_t sd_state;
mext2_command* command;


bool reset_software()
{
    //set CMD0
    command.index = calc_command_number(COMMAND_SOFTWARE_RESET);
    command.argument[] = {0x00, 0x00, 0x00, 0x00};
    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

    mext2_response response = mext2_send_command(command, MEXT2_R1);
    if(response == NULL || response.r1 != R1_IN_IDLE_STATE)
    {
        return false;
    }

    wait_after_response(response);
    return true;
}

bool check_voltage_range()
{
    //set CMD8
    command.index = calc_command_number(COMMAND_CHECK_VOLTAGE);
    command.argument[] = {0x00, 0x00, 0x01, 0xaa};
    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

    mext2_response response = mext2_send_command(command, MEXT2_R7);
     if(response == NULL)
    {
        return false;
    }

    if(response.r1 & R1_ILLEGAL_COMMAND)
        sd_version = SD_V1X;

    wait_after_response(response);
    return true;
}

bool read_OCR()
{
    //set CMD58
    command.index = calc_command_number(COMMAND_READ_OCR);
    command.argument[] = {0x00, 0x00, 0x00, 0x00};
    command.crc = calc_crc7(command, COMMAND_SIZE - sizeof(uint8_t));

    mext2_response response = mext2_send_command(command, MEXT2_R3);
     if(response == NULL || (response.r1 & R1_ILLEGAL_COMMAND) == true)
    {
        return false;
    }

    wait_after_response(response);
    return true;
}


uint8_t init()
{
    mext2_command* command = (mext2_command*)malloc(sizeof(mext2_command));
    sd_state = SD_IDLE;

    while(1)
    {
        switch(sd_state)
        {
            case SD_ERROR:
            {
                free(command);
                mext2_reset_pins();
                return SD_INIT_NOK;
            }

            case SD_INITIALIZED:
            {
                free(command);
                return SD_INIT_OK;
            }

            case SD_IDLE:
            {
                if(sd_card_initialized == true) 
                    {
                        sd_state = SD_INITIALIZED;
                        break;
                    }
                    else sd_state = SD_CONFIGURE_PINS;
            }

            case SD_CONFIGURE_PINS:
            {
                if(mext2_configure_pins() == 1)
                {
                    debug("Error: pin initialization failed\n");
                    sd_state = SD_ERROR;
                    break;
                }
                else sd_state = SD_RESET_SOFTWARE;
            }

            case SD_RESET_SOFTWARE:
            {
                usleep(1000);  //wait 1ms
                digitalWrite(MEXT2_MOSI, MEXT2_HIGH);

                for(uint8_t i = 0; i < 74; i++) //74 dummy clock cycles
                {
                    digitalWrite(MEXT2_SCLK, MEXT2_HIGH);
                    digitalWrite(MEXT2_SCLK, MEXT2_LOW);
                }

                if(reset_software() == false)
                {
                    debug("Error: not an SD card\n");
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_CHECK_VOLTAGE;
            }

            case SD_CHECK_VOLTAGE:
            {
                if(check_voltage_range() == false)
                {
                    debug("Error: wrong voltage range.\n");
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_READ_OCR;
            }

            case SD_READ_OCR:
            {
                if(read_OCR() == false)
                {
                    debug("Error: cannot read OCR.\n");
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_PREPARE_INIT_PROCESS;
            }

            case SD_PREPARE_INIT_PROCESS:
            {
                if(prepare_init_process() == false)
                {
                    debug("Error: cannot prepare for initialization process.\n");
                    mext2_reset_pins();
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_START_INIT_PROCESS;
            }

            case SD_START_INIT_PROCESS:
            {
                uint8_t response = start_init_process();
                if(response & R1_IDLE_STATE)
                {
                    sd_state = SD_PREPARE_INIT_PROCESS;
                    break;
                }
                else sd_state = SD_READ_OCR_AGAIN;
            }

            case SD_READ_OCR_AGAIN:
            {
                if(sd_version != SD_V1X)
                {
                    uint8_t response = read_OCR_again();
                    if(response & (uint8_t)0x40)
                    {
                        sd_version = SD_V2XHCXC;
                    }
                    else
                    {
                        sd_version = SD_V2X;
                    }
                }
                sd_state = SD_READ_CSD;
            }
            case SD_READ_CSD:
            {
                uint8_t result = read_CSD_register();
                //itd
            }

            default:
            {   sd_state = SD_ERROR;
                break;
            }
        }

    }

}