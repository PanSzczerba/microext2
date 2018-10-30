#include "debug.h"
#include "pin.h"
#include "timing.h"
#include "spi.h"
#include "command.h"
#include "common.h"
#include "sd.h"

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

STATIC uint8_t reset_software()
{
    //set CMD0
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_SOFTWARE_RESET, command_argument);

    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != R1_IN_IDLE_STATE)
        return MEXT2_RETURN_FAILURE;

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t check_voltage_range(mext2_sd* sd)
{
    //set CMD8
    uint8_t command_argument[] = {0x00, 0x00, 0x01, 0xaa};
    mext2_response response = mext2_send_command(COMMAND_CHECK_VOLTAGE, command_argument);

    if(response.r1 & R1_INVALID_RESPONSE)
        return MEXT2_RETURN_FAILURE;

    if(response.r1 & R1_ILLEGAL_COMMAND)
        sd -> sd_version = SD_V1X;

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t read_OCR(mext2_sd* sd)
{
    //set CMD58
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_READ_OCR, command_argument);

    if((response.r1 & R1_INVALID_RESPONSE) || (response.r1 & R1_ILLEGAL_COMMAND) == true)
        return MEXT2_RETURN_FAILURE;

    if(sd -> sd_version == SD_NOT_DETERMINED)
    {
        if(response.ocr[0] & (uint8_t)0x40)
            sd -> sd_version = SD_V2XHCXC;
        else
            sd -> sd_version = SD_V2X;
    }

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t prepare_init_process()
{
    //set CMD55
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_BEFORE_INIT_PROCESS, command_argument);

    if((response.r1 & R1_INVALID_RESPONSE) || (response.r1 & R1_ILLEGAL_COMMAND))
        return MEXT2_RETURN_FAILURE;

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t start_init_process()
{
    //set ACMD41
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_INIT_PROCESS, command_argument);

    if(response.r1 & R1_IN_IDLE_STATE)
        return MEXT2_RETURN_SUCCESS;
    else
        return MEXT2_RETURN_FAILURE;

}

STATIC uint8_t read_CSD_register()
{
    uint8_t csd_register[16];

    //set CMD9
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_READ_CSD, command_argument);

    if(response.r1 & R1_INVALID_RESPONSE)
        return MEXT2_RETURN_FAILURE;

    if(response.r1 != 0)
        mext2_warning("Reading CSD register failed\n");
    else
    {
        uint8_t buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff};
        if(!wait_for_response(buffer))
            return MEXT2_RETURN_FAILURE;

        memset(csd_register, 0xff, 16);
        spi_read_write(csd_register, 16);
        buffer[0] = 0xff;
        buffer[1] = 0xff;
        spi_read_write(buffer, 2);
    }

    return MEXT2_RETURN_SUCCESS;
}

mext2_return_value mext2_sd_init(mext2_sd* sd)
{
    uint8_t sd_state = SD_IDLE;
    sd ->  sd_version = SD_NOT_DETERMINED;

    while(1)
    {
        switch(sd_state)
        {
            case SD_ERROR:
            {
                reset_pins();
                sd -> sd_initialized = FALSE;
                return MEXT2_RETURN_FAILURE;
            }
            break;

            case SD_INITIALIZED:
            {
                sd -> sd_initialized = TRUE;
                return MEXT2_RETURN_SUCCESS;
            }
            break;

            case SD_IDLE:
            {
                if(sd -> sd_initialized == TRUE)
                    {
                        mext2_log("Card already initialized.");
                        sd_state = SD_INITIALIZED;
                    }
                    else sd_state = SD_CONFIGURE_PINS;
            }
            break;

            case SD_CONFIGURE_PINS:
            {
                if(configure_pins() == 1)
                {
                    mext2_error("Pin initialization failed.");
                    sd_state = SD_ERROR;
                }
                else sd_state = SD_RESET_SOFTWARE;
            }
            break;

            case SD_RESET_SOFTWARE:
            {
                //usleep(1000);  //wait 1ms
                mext2_delay(1000);
                mext2_pin_set(MEXT2_MOSI, MEXT2_HIGH);

                for(uint8_t i = 0; i < 74; i++) //74 dummy clock cycles
                {
                    mext2_pin_set(MEXT2_SCLK, MEXT2_HIGH);
                    mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
                }

                if(reset_software() == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Not an SD card.");
                    sd_state = SD_ERROR;
                } else sd_state = SD_CHECK_VOLTAGE;
            }
            break;

            case SD_CHECK_VOLTAGE:
            {
                if(check_voltage_range(sd) == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Wrong voltage range.");
                    sd_state = SD_ERROR;
                } else 
                    sd_state = SD_READ_OCR;
            }
            break;

            case SD_READ_OCR:
            {
                if(read_OCR(sd) == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Cannot read OCR.");
                    sd_state = SD_ERROR;
                } else sd_state = SD_PREPARE_INIT_PROCESS;
            }
            break;

            case SD_PREPARE_INIT_PROCESS:
            {
                if(prepare_init_process() == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Cannot prepare for initialization process.");
                    reset_pins();
                    sd_state = SD_ERROR;
                } else sd_state = SD_START_INIT_PROCESS;
            }
            break;

            case SD_START_INIT_PROCESS:
            {
                if(start_init_process())
                    sd_state = SD_PREPARE_INIT_PROCESS;
                else
                    sd_state = SD_READ_OCR_AGAIN;
            }
            break;

            case SD_READ_OCR_AGAIN:
            {
                if(sd -> sd_version == SD_NOT_DETERMINED)
                {
                    if(read_OCR(sd) == MEXT2_RETURN_FAILURE)
                    {
                        mext2_error("Cannot read OCR.");
                        sd_state = SD_ERROR;
                    }
                }
                else sd_state = SD_READ_CSD;
            }
            break;

            case SD_READ_CSD:
            {
                if(read_CSD_register() == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Cannot read CSD register.");
                    reset_pins();
                    sd_state = SD_ERROR;
                } else sd_state = SD_INITIALIZED;
            }
            break;

            default:
            {   sd_state = SD_ERROR;
                break;
            }
        }

    }

}
