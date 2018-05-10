#ifndef SPI_H
#define SPI_H

#define MEXT2_PWR 7
#define MEXT2_SCLK 5 
#define MEXT2_CS 1
#define MEXT2_MOSI 4
#define MEXT2_MISO 6

extern int configured_pins;
int configure_pins();
void reset_pins();
int spi_read_write(uint8_t* buff, size_t buff_size);

#endif
