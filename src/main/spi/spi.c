#include "spi.h"
#include "debud.h"
#include "pin.h"
#include "timing.h"

static int configured_pins = 0;

/*********** PIN INICIALIZATION ***********/
int configure_pins()
{
	if(configured_pins)
	{
		debug("Error: pins have been already configured\n");
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
		debug("Error: couldn't initialize witingPi setup\n");
		return 1;
	}
	return 0;
}

/*********** READ/WRITE FUNCTION ***********/

uint8_t signal_to_word(size_t mask, uint8_t word)
{
	if(mext2_pin_get(MEXT2_MISO))
		word = word | mask;
	else
		word = word & ~mask;

	return word;
}


void word_to_signal(size_t mask, uint8_t word)
{
	if(mask & word)
		mext2_pin_set(MEXT2_MOSI, MEXT2_HIGH);
	else
		mext2_pin_set(MEXT2_MOSI, MEXT2_LOW);
}


int spi_read_write(uint8_t* buffer, size_t buffer_size)
{
	if(!configured_pins)
	{
		debug("Error: can't read - pins have not been configured\n");
		return 1;
	}

	mext2_pin_set(MEXT2_CS, MEXT2_LOW);

	for(size_t i = 0; i < buffer_size; i++)
	{
		for(uint8_t mask = 0x80; mask > 0; mask >>= 1) 
		{
			word_to_signal(mask, buffer[i]); //send command
			buffer[i] = signal_to_word(mask, buffer[i]); //get response
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
