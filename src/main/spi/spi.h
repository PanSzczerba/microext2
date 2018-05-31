#ifndef SPI_H
#define SPI_H

int configure_pins();
void reset_pins();
int spi_read_write(uint8_t* buff, size_t buff_size);

#endif
