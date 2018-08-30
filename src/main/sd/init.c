#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"

static bool sd_card_initialized = false;
uint8_t csd_register[16];
static uint8_t sd_state;
mext2_command* command;

#define SD_NOT_DETERMINED   0
#define SD_V1X              1
#define SD_V2X              2
#define SD_V2XHCXC          3

uint8_t sd_version = SD_NOT_DETERMINED;


#define COMMAND_SOFTWARE_RESET      (uint8_t)0x00    //CMD0
#define COMMAND_CHECK_VOLTAGE       (uint8_t)0x08    //CMD8
#define COMMAND_READ_OCR            (uint8_t)0x3A    //CMD58
#define COMMAND_BEFORE_INIT_PROCESS (uint8_t)0x37    //CMD55
#define COMMAND_INIT_PROCESS        (uint8_t)0x29    //ACMD41
#define COMMAND_READ_CSD            (uint8_t)0x09    //CMD09

#define SD_INIT_OK              (uint8_t)0x00
#define SD_INIT_NOK             (uint8_t)0x01
#define SD_IDLE                 (uint8_t)0x02
#define SD_ERROR                (uint8_t)0x03
#define SD_INITIALIZED          (uint8_t)0x04

#define SD_CONFIGURE_PINS       (uint8_t)0x05
#define SD_RESET_SOFTWARE       (uint8_t)0x06
#define SD_CHECK_VOLTAGE        (uint8_t)0x07
#define SD_READ_OCR             (uint8_t)0x08
#define SD_PREPARE_INIT_PROCESS (uint8_t)0x09
#define SD_START_INIT_PROCESS   (uint8_t)0x0a
#define SD_READ_OCR_AGAIN       (uint8_t)0x0b
#define SD_READ_CSD             (uint8_t)0x0c

bool check_sd_version = false;
uint8_t csd_register[16];

bool reset_software()
{
    //set CMD0
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    set_command(command, COMMAND_SOFTWARE_RESET, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != R1_IN_IDLE_STATE)
    {
        return false;
    }

    wait_after_response((uint8_t*) response);
    return true;
}

bool check_voltage_range()
{
    //set CMD8
    uint8_t command_argument[] = {0x00, 0x00, 0x01, 0xaa};
    set_command(command, COMMAND_CHECK_VOLTAGE, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R7);
     if(response == NULL)
    {
        return false;
    }

    if(response -> r1 & R1_ILLEGAL_COMMAND)
        sd_version = SD_V1X;

    wait_after_response((uint8_t*) response);
    return true;
}

bool read_OCR()
{
    //set CMD58
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    set_command(command, COMMAND_READ_OCR, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R3);
    if(response == NULL || (response -> r1 & R1_ILLEGAL_COMMAND) == true)
    {
        return false;
    }

    if(check_sd_version)
    {
        if(response -> ocr[0] & (uint8_t)0x40)
            sd_version = SD_V2XHCXC;
        else
            sd_version = SD_V2X;
    }

    wait_after_response((uint8_t*) response);
    return true;
}

bool prepare_init_process()
{
    //set CMD55
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    set_command(command, COMMAND_BEFORE_INIT_PROCESS, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 & R1_ILLEGAL_COMMAND)
    {
        return false;
    }
    wait_after_response((uint8_t*) response);
    return true;
}

uint8_t start_init_process()
{
    //set ACMD41
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    set_command(command, COMMAND_INIT_PROCESS, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL)
    {
        return 0;
    }
    
    return response -> r1;
}


bool read_CSD_register()
{
    //set CMD9
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    set_command(command, COMMAND_READ_CSD, command_argument); 

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL)
    {
        return false;
    }


    if(response -> r1 != 0)
    {
        debug("Error: reading CSD register failed\n");
    }
    else
    {
        uint8_t buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff};
        if(wait_for_response(buffer) == false) 
            return false;

        memset(csd_register, 0xff, 16);
        spi_read_write(csd_register, 16);
        buffer[0] = 0xff;
        buffer[1] = 0xff;
        spi_read_write(buffer, 2);
    }

    wait_after_response((uint8_t*) response);

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
                reset_pins();
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
                if(configure_pins() == 1)
                {
                    debug("Error: pin initialization failed\n");
                    sd_state = SD_ERROR;
                    break;
                }
                else sd_state = SD_RESET_SOFTWARE;
            }

            case SD_RESET_SOFTWARE:
            {
                //usleep(1000);  //wait 1ms
                sleep(1);
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
                    reset_pins();
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_START_INIT_PROCESS;
            }

            case SD_START_INIT_PROCESS:
            {
                uint8_t response = start_init_process();
                if(response & R1_IN_IDLE_STATE)
                {
                    sd_state = SD_PREPARE_INIT_PROCESS;
                    response = 0xff;
                    spi_read_write(&response, 1); // after response wait for 8 clock cycles (for safety)
                    break;
                }
                else sd_state = SD_READ_OCR_AGAIN;
            }

            case SD_READ_OCR_AGAIN:
            {
                if(sd_version != SD_V1X)
                {
                    check_sd_version = true;
                    if(read_OCR() == false)
                    {
                        debug("Error: cannot read OCR.\n");
                        sd_state = SD_ERROR;
                        break;
                    }
                }
                sd_state = SD_READ_CSD;
            }

            case SD_READ_CSD:
            {
                if(read_CSD_register() == false)
                {
                    debug("Error: cannot read CSD register.\n");
                    reset_pins();
                    sd_state = SD_ERROR;
                    break;
                } else sd_state = SD_INITIALIZED;
            }

            default:
            {   sd_state = SD_ERROR;
                break;
            }
        }

    }

}