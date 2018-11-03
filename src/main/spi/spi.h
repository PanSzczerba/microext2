#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stddef.h>

#define MAX_CLOCK_FREQUENCY 0

void set_clock_frequency(uint64_t freq);
uint8_t configure_pins();
void reset_pins();
uint8_t spi_read_write(uint8_t* buff, size_t buff_size);

#endif
