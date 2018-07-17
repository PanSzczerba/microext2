#ifndef MICROEXT2_PIN_H
#define MICROEXT2_PIN_H
#include <wiringPi.h>
/* possible pin states */ 
#define MEXT2_LOW LOW
#define MEXT2_HIGH HIGH

/* possible pin modes */
#define MEXT2_INPUT INPUT
#define MEXT2_OUTPUT OUTPUT
#define MEXT2_INPUT_PULLUP 2

/* pin numbers */
#define MEXT2_PWR 3
#define MEXT2_SCLK 14
#define MEXT2_CS 10
#define MEXT2_MOSI 12
#define MEXT2_MISO 13

/* functions */
#define mext2_init() wiringPiSetup()
#define mext2_pin_mode(pin_number, pin_mode) if((pin_mode) == MEXT2_INPUT || (pin_mode) == MEXT2_OUTPUT) pinMode((pin_number), (pin_mode)); \
                                             else { pinMode((pin_number), INPUT); pullUpDnControl((pin_number), PUD_UP); } // void mext2_pin_mode(uint8_t pin_number, uint8_t pin_mode)
#define mext2_pin_set(pin_number, value) digitalWrite((pin_number), (value)) // void mext2_pin_set(uint8_t pin_number, uint8_t value);
#define mext2_pin_get(pin_number) digitalRead((pin_number)) // uint8_t mext2_pin_get(uint8_t pin_number);

#endif
