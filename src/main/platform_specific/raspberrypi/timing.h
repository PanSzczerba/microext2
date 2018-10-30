#ifndef MICROEXT2_TIMING_H
#define MICROEXT2_TIMING_H
#include <wiringPi.h>

#define mext2_millis() millis() // unsigned int mext2_milis()
#define mext2_micros() micros() // unsigned int mext2_micros()
#define mext2_delay(miliseconds) delay((miliseconds)) // void mext2_delay(unsigned int miliseconds);
#define mext2_delay_microseconds(microseconds) delayMicroseconds((microseconds)) // void mext2_delay_microseconds(unsigned int microseconds);

#endif
