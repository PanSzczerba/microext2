#include "common.h"
#include "spi.h"
#include "debug.h"
#include "pin.h"
#include "timing.h"

STATIC mext2_bool configured_pins = MEXT2_FALSE;
STATIC uint64_t clock_delay = 0;

/*********** PIN INICIALIZATION ***********/
uint8_t configure_pins()
{
    if(configured_pins)
    {
        mext2_warning("Pins have been already configured");
        return MEXT2_RETURN_SUCCESS;
    }

    if(!mext2_init())
    {
        unsigned int time_to_wait = 1000;
        mext2_delay_microseconds(time_to_wait);

//      mext2_pin_mode(MEXT2_PWR, MEXT2_OUTPUT);
        mext2_pin_mode(MEXT2_SCLK, MEXT2_OUTPUT);
        mext2_pin_mode(MEXT2_CS, MEXT2_OUTPUT);
        mext2_pin_mode(MEXT2_MOSI, MEXT2_OUTPUT);
        mext2_pin_mode(MEXT2_MISO, MEXT2_INPUT_PULLUP);

//      mext2_pin_set(MEXT2_PWR, MEXT2_HIGH);
        mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
        mext2_pin_set(MEXT2_CS, MEXT2_HIGH);
        mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);

        configured_pins = MEXT2_TRUE;
    }
    else
    {
        mext2_error("Couldn't initialize wiringPi setup");
        return MEXT2_RETURN_FAILURE;
    }
    return MEXT2_RETURN_SUCCESS;
}

/*********** READ/WRITE FUNCTION ***********/

uint8_t spi_read_write(uint8_t* buffer, size_t buffer_size)
{
    if(!configured_pins)
    {
        mext2_error("Can't read - pins have not been configured");
        return MEXT2_RETURN_FAILURE;
    }

    mext2_pin_set(MEXT2_CS, MEXT2_LOW);

    for(size_t i = 0; i < buffer_size; i++)
    {
        for(uint8_t mask = 0x80; mask > 0; mask >>= 1)
        {
            //send command
            if(mask & buffer[i])
                mext2_pin_set(MEXT2_MOSI, MEXT2_HIGH);
            else
                mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);

            mext2_pin_set(MEXT2_SCLK, MEXT2_HIGH);
            //get response
            mext2_delay_microseconds(clock_delay);
            if(mext2_pin_get(MEXT2_MISO))
                buffer[i] = buffer[i] | mask;
            else
                buffer[i] = buffer[i] & ~mask;
            mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
            mext2_delay_microseconds(clock_delay);
        }
    }

    mext2_pin_set(MEXT2_CS, MEXT2_HIGH);
    return MEXT2_RETURN_SUCCESS;
}

/*********** RESET PINS ***********/

void reset_pins()
{
//  mext2_pin_set(MEXT2_PWR, MEXT2_LOW);
    mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
    mext2_pin_set(MEXT2_CS, MEXT2_LOW);
    mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);

//  mext2_pin_mode(MEXT2_PWR, MEXT2_INPUT);
    mext2_pin_mode(MEXT2_SCLK, MEXT2_INPUT);
    mext2_pin_mode(MEXT2_CS, MEXT2_INPUT);
    mext2_pin_mode(MEXT2_MOSI, MEXT2_INPUT);
}

/******* CLOCK FREQUENCY CHANGE ******/
#define FREQUENCY_TO_DELAY(freq) (unsigned int)(((double)1/(2*freq)) * 1000000)

void set_clock_frequency(uint64_t freq)
{
    if(freq == MAX_CLOCK_FREQUENCY)
        clock_delay = 0;
    else
        clock_delay = FREQUENCY_TO_DELAY(freq);
}
