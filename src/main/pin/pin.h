#ifndef MICROEXT2_PIN_H
#define MICROEXT2_PIN_H

#include <stdint.h>

/* possible pin states */ 
#define MEXT2_LOW 0
#define MEXT2_HIGH 1

/* possible pin modes */
#define MEXT2_INPUT 0
#define MEXT2_OUTPUT 1
#define MEXT2_INPUT_PULLUP 2

/* pin numbers */
#define MEXT2_PWR 0
#define MEXT2_SCLK 1 
#define MEXT2_CS 2
#define MEXT2_MOSI 3
#define MEXT2_MISO 4

void mext2_pin_mode(uint8_t pin_number, uint8_t pin_mode);
void mext2_pin_set(uint8_t pin_number, uint8_t value);
uint8_t mext2_pin_get(uint8_t pin_number);

#endif
