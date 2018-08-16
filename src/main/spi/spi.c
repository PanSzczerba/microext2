#include "spi.h"
#include "debug.h"
#include "pin.h"
#include "timing.h"

static int configured_pins = 0;

/*********** PIN INICIALIZATION ***********/
int configure_pins()
{
	if(configured_pins)
	{
		mext2_warning("Pins have been already configured");
		return 0;
	}

	if(!mext2_init())
	{
		unsigned int time_to_wait = 1000;
		mext2_delay_microseconds(time_to_wait);
		
		mext2_pin_mode(MEXT2_PWR, MEXT2_OUTPUT);
		mext2_pin_mode(MEXT2_SCLK, MEXT2_OUTPUT);
		mext2_pin_mode(MEXT2_CS, MEXT2_OUTPUT);
		mext2_pin_mode(MEXT2_MOSI, MEXT2_OUTPUT);
		mext2_pin_mode(MEXT2_MISO, MEXT2_INPUT);

		mext2_pin_mode(MEXT2_MISO, PUD_UP);

		mext2_pin_set(MEXT2_PWR, MEXT2_HIGH);		
		mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
		mext2_pin_set(MEXT2_CS, MEXT2_HIGH);
		mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);

		configured_pins = 1;
	}
	else
	{
		mext2_error("Couldn't initialize witingPi setup");
		return 1;
	}
	return 0;
}

/*********** READ/WRITE FUNCTION ***********/

int spi_read_write(uint8_t* buffer, size_t buffer_size)
{
	if(!configured_pins)
	{
		mext2_error("Can't read - pins have not been configured");
		return 1;
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
			
			//get response
			if(mext2_pin_get(MEXT2_MISO))
				buffer[i] = buffer[i] | mask;
			else
				buffer[i] = buffer[i] & ~mask;

			mext2_pin_set(MEXT2_SCLK, MEXT2_HIGH);
			mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
		}
	}

	mext2_pin_set(MEXT2_CS, MEXT2_HIGH);
	return 0;
}

/*********** RESET PINS ***********/

void reset_pins()
{
	mext2_pin_set(MEXT2_PWR, MEXT2_LOW);		
	mext2_pin_set(MEXT2_SCLK, MEXT2_LOW);
	mext2_pin_set(MEXT2_CS, MEXT2_LOW);
	mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);

	mext2_pin_mode(MEXT2_PWR, MEXT2_INPUT);
	mext2_pin_mode(MEXT2_SCLK, MEXT2_INPUT);
	mext2_pin_mode(MEXT2_CS, MEXT2_INPUT);
	mext2_pin_mode(MEXT2_MOSI, MEXT2_INPUT);
}
