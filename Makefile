CC := gcc
AR := ar
MKDIR := mkdir
RM := rm
CFLAGS := -std=c99 -Wall

DEBUG := 1

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g
endif

PINDIR := main/platform_specific
SPIDIR := main/spi
CRCDIR := main/crc

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino

PLATFORM := $(DEFAULT_PLATFORM)

ifeq ($(PLATFORM), 'raspberrypi')
LIBS= -lwiringPi
endif

INCLUDE := -Isrc/$(PINDIR)/$(PLATFORM)
INCLUDE += -Isrc/$(SPIDIR)

all: out/$(SPIDIR)/spi.o out/$(CRCDIR)/crc.o

# SPI 
out/$(SPIDIR)/spi.o: src/$(SPIDIR)/spi.c src/$(SPIDIR)/spi.h | out/$(SPIDIR)
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)
	
out/$(SPIDIR):
	$(MKDIR) -p out/$(SPIDIR)

# CRC
out/$(CRCDIR)/crc.o: src/$(CRCDIR)/crc.c src/$(CRCDIR)/crc.h | out/$(CRCDIR)
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

out/$(CRCDIR):
	$(MKDIR) -p out/$(CRCDIR)
	


# CLEANUP
clean:
	$(RM) -rf out
