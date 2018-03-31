#ifndef MICROEXT2_PIN_H
#define MICROEXT2_PIN_H

//possible pin states
#define LOW 0
#define HIGH 1

//possible pin modes
#define INPUT 0
#define OUTPUT 1

void pin_mode(int pin_number, int pin_mode);
void pin_set(int pin_number, int value);
int pin_get(int pin_number);

#endif
