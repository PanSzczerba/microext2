#ifndef SPI_H
#define SPI_H
#include <stdint.h>
#include <stddef.h>

uint8_t configure_pins();
void reset_pins();
uint8_t spi_read_write(uint8_t* buff, size_t buff_size);

#endif
