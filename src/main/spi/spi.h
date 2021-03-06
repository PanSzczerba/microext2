#ifndef MEXT2_SPI_H
#define MEXT2_SPI_H
#include <stdint.h>
#include <stddef.h>

#define MEXT2_MAX_CLOCK_FREQUENCY 0

void mext2_set_clock_frequency(uint64_t freq);
uint8_t mext2_configure_pins();
void mext2_reset_pins();
uint8_t mext2_spi_read_write(uint8_t* buffer, size_t buffer_size);
uint8_t mext2_spi_write(uint8_t* buffer, size_t buffer_size);

#endif
