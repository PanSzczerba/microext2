#include "debug.h"
#include "pin.h"
#include "timing.h"
#include "spi.h"
#include "command.h"
#include "common.h"
#include "sd.h"
#include "fs.h"

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

struct partition_info
{
    uint8_t is_bootable;
    uint8_t fst_sector_chs_address[3];
    uint8_t part_type;
    uint8_t lst_sector_chs_address[3];
    uint32_t lba_address;
    uint32_t sector_count;
} __mext2_packed;

struct mbr
{
    uint8_t bootloader[446];
    struct partition_info partitions[4];
    uint16_t boot_signature;
} __mext2_packed;

#define MBR_BOOT_SIGNATURE 0xaa55

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
    mext2_debug("R7 response payload: 0x%hhx 0x%hhx 0x%hhx 0x%hhx", response.extended_response[0], response.extended_response[1],
            response.extended_response[2], response.extended_response[3]);

    if(response.r1 & R1_INVALID_RESPONSE)
        return MEXT2_RETURN_FAILURE;

    if(response.r1 & R1_ILLEGAL_COMMAND)
        sd -> sd_version = SD_V1X;

    return MEXT2_RETURN_SUCCESS;
}
#define VOLTAGE_3_2_3_3 ((uint8_t)(1 << 4))
#define VOLTAGE_3_3_3_4 ((uint8_t)(1 << 4))

STATIC uint8_t read_voltage_range(mext2_sd* sd)
{
    //set CMD58
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_READ_OCR, command_argument);
    mext2_debug("OCR register content: 0x%hhx 0x%hhx 0x%hhx 0x%hhx", response.extended_response[0], response.extended_response[1],
            response.extended_response[2], response.extended_response[3]);

    if((response.r1 & R1_INVALID_RESPONSE) || (response.r1 & R1_ILLEGAL_COMMAND) == true)
    {
        mext2_error("Cannot read OCR.");
        return MEXT2_RETURN_FAILURE;
    }

    if(!((response.extended_response[1] & VOLTAGE_3_2_3_3) && (response.extended_response[1] & VOLTAGE_3_3_3_4)))
    {
        mext2_error("Unsupported voltage range");
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t read_CCS(mext2_sd* sd)
{
    //set CMD58
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_READ_OCR, command_argument);
    mext2_debug("OCR register content: 0x%hhx 0x%hhx 0x%hhx 0x%hhx", response.extended_response[0], response.extended_response[1],
            response.extended_response[2], response.extended_response[3]);

    if((response.r1 & R1_INVALID_RESPONSE) || (response.r1 & R1_ILLEGAL_COMMAND) == true)
    {
        mext2_error("Cannot read OCR.");
        return MEXT2_RETURN_FAILURE;
    }

    if(sd -> sd_version == SD_NOT_DETERMINED)
    {
        if(response.extended_response[0] & (uint8_t)0x40)
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
    uint8_t command_argument[] = { 0x40, 0x00, 0x00, 0x00 };
    mext2_response response = mext2_send_command(COMMAND_INIT_PROCESS, command_argument);

    if(response.r1 & R1_INVALID_RESPONSE)
        return MEXT2_RETURN_FAILURE;

    if(response.r1 & R1_IN_IDLE_STATE)
        return MEXT2_RETURN_FAILURE;
    else
        return MEXT2_RETURN_SUCCESS;

}

STATIC uint8_t read_CSD_register(mext2_sd* sd)
{
    uint8_t* csd_register = (uint8_t*)&sd->csd;

    //set CMD9
    uint8_t command_argument[] = {0x00, 0x00, 0x00, 0x00};
    mext2_response response = mext2_send_command(COMMAND_READ_CSD, command_argument);

    if(response.r1 != 0)
    {
        mext2_warning("Reading CSD register failed\n");
        return MEXT2_RETURN_FAILURE;

    }

    memset(csd_register, 0xff, 16);
    uint8_t buffer;
    if(!wait_for_response(&buffer))
        return MEXT2_RETURN_FAILURE;

    if(buffer == 0xfe)
    {
        spi_read_write(csd_register, 16);
    }
    else if(buffer == 0x0 || buffer == 0x40)
    {
        csd_register[0] = buffer;
        spi_read_write(csd_register + 1, 15);
    }
    else
    {
        mext2_error("Invalid CSD register token: 0x%hhx", buffer);
        return MEXT2_RETURN_FAILURE;
    }

    wait_8_clock_cycles();
    wait_8_clock_cycles();
    return MEXT2_RETURN_SUCCESS;

}

STATIC void correct_MBR_endianess(struct mbr* mbr_ptr)
{
    for(int i = 0; i < 4; i++)
    {
        mbr_ptr->partitions[i].lba_address = mext2_flip_endianess32(mbr_ptr->partitions[i].lba_address);
        mbr_ptr->partitions[i].sector_count = mext2_flip_endianess32(mbr_ptr->partitions[i].sector_count);
    }
}

STATIC uint8_t parse_MBR(mext2_sd* sd)
{
    struct block512_t mbr[(sizeof(struct mbr) + sizeof(struct block512_t) - 1)/sizeof(struct block512_t)];

    if(mext2_read_blocks(sd, 0, (struct block512_t*)&mbr, sizeof(mbr)/sizeof(struct block512_t)) == MEXT2_RETURN_FAILURE)
        return MEXT2_RETURN_FAILURE;

    struct mbr* mbr_ptr = (struct mbr*)&mbr;

    if(mext2_is_big_endian())
        correct_MBR_endianess(mbr_ptr);

    if(mbr_ptr->boot_signature != MBR_BOOT_SIGNATURE)
    {
        mext2_error("Invalid MBR boot signature: 0x%hx", mbr_ptr->boot_signature);
        return MEXT2_RETURN_FAILURE;
    }

    if(mbr_ptr->partitions[0].is_bootable != 0x0 && mbr_ptr->partitions[0].is_bootable != 0x80)
    {
        mext2_error("Invalid partition info - wrong bootable flag: 0x%hhx", mbr_ptr->partitions[0].is_bootable);
        return MEXT2_RETURN_FAILURE;
    }

    if(mbr_ptr->partitions[0].part_type == 0)
    {
        mext2_error("Invalid partition type: 0");
        return MEXT2_RETURN_FAILURE;
    }

    mext2_debug("Partiton type: 0x%hhx", mbr_ptr->partitions[0].part_type);

    sd->partition_block_addr = mbr_ptr->partitions[0].lba_address;
    mext2_debug("Partition address 0x%x", sd->partition_block_addr);

    sd->partition_block_size = mbr_ptr->partitions[0].sector_count;
    mext2_debug("Partition block size 0x%x", sd->partition_block_size);

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t probe_fs_type(mext2_sd* sd)
{
    sd->fs.open_strategy = NULL;
    for(uint8_t i; i < MEXT2_FS_PROBE_CHAINS_SIZE; i++)
    {
        if(mext2_fs_probe_chains[i](sd) == MEXT2_RETURN_SUCCESS)
            return MEXT2_RETURN_SUCCESS;
    }

    return MEXT2_RETURN_FAILURE;
}

uint8_t mext2_sd_init(mext2_sd* sd)
{
    uint8_t sd_state = SD_IDLE;
    sd ->  sd_version = SD_NOT_DETERMINED;
    sd ->  fs.read_strategy  = NULL;
    sd ->  fs.write_strategy = NULL;
    sd ->  fs.open_strategy  = NULL;
    sd ->  fs.close_strategy = NULL;
    sd ->  fs.eof_strategy   = NULL;
    sd ->  fs.seek_strategy  = NULL;

    while(1)
    {
        switch(sd_state)
        {
            case SD_ERROR:
            {
                reset_pins();
                sd -> sd_initialized = FALSE;
                set_clock_frequency(MAX_CLOCK_FREQUENCY);
                return MEXT2_RETURN_FAILURE;
            }
            break;

            case SD_INITIALIZED:
            {
                sd -> sd_initialized = TRUE;
                set_clock_frequency(MAX_CLOCK_FREQUENCY);
                goto done;
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
                if(configure_pins() == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("Pin initialization failed.");
                    sd_state = SD_ERROR;
                }
                else sd_state = SD_RESET_SOFTWARE;
            }
            break;

            case SD_RESET_SOFTWARE:
            {
                set_clock_frequency(100000);

                mext2_delay(1);
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
                if(read_voltage_range(sd) == MEXT2_RETURN_FAILURE)
                {
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
                    sd_state = SD_READ_OCR_AGAIN;
                else
                    sd_state = SD_PREPARE_INIT_PROCESS;
            }
            break;

            case SD_READ_OCR_AGAIN:
            {
                if(sd -> sd_version == SD_NOT_DETERMINED)
                {
                    if(read_CCS(sd) == MEXT2_RETURN_FAILURE)
                    {
                        sd_state = SD_ERROR;
                    }
                }
                else sd_state = SD_READ_CSD;
            }
            break;

            case SD_READ_CSD:
            {
                if(read_CSD_register(sd) == MEXT2_RETURN_FAILURE)
                {
                    mext2_error("CSD register read_strategy failed.");
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

done:
    if(parse_MBR(sd) == MEXT2_RETURN_FAILURE)
    {
        mext2_error("MBR parse failed");
        return MEXT2_RETURN_FAILURE;
    }

    if(probe_fs_type(sd) == MEXT2_RETURN_FAILURE)
    {
        mext2_error("No known filesystem found");
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}
