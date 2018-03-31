#include "../pin.h"
#include <wiringPi.h>
#include <stdint.h>

#define WPI_PWR 7
#define WPI_SCLK 5 
#define WPI_CS 1
#define WPI_MOSI 4
#define WPI_MISO 6

uint8_t mext2_pin_map[] = {WPI_PWR, WPI_SCLK, WPI_CS, WPI_MOSI, WPI_MISO};
uint8_t mext2_mode_map[] = {INPUT, OUTPUT};

void mext2_pin_mode(uint8_t pin_number, uint8_t pin_mode)
{
    pinMode(mext2_pin_map[pin_number], mext2_mode_map[pin_mode]);
}

void mext2_pin_set(uint8_t pin_number, uint8_t value)
{
    digitalWrite(mext2_pin_map[pin_number], value);
}

uint8_t mext2_pin_get(uint8_t pin_number)
{
    return (uint8_t)digitalRead(mext2_pin_map[pin_number]);
}
