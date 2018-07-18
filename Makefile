CC=gcc
AR=ar
MKDIR=mkdir
RM=rm
CFLAGS= -std=c99 -Wall

DEBUG=1

ifdef DEBUG
CFLAGS+= -O0 -g
endif

PINDIR=main/platform_specific
SPIDIR=main/spi
PLATFORM=raspberrypi

ifeq ($(PLATFORM), 'raspberrypi')
LIBS= -lwiringPi
endif

INCLUDE:= -Isrc/$(PINDIR)/$(PLATFORM)
INCLUDE+= -Isrc/$(SPIDIR)

all: out/$(SPIDIR)/spi.a

# SPI 
out/$(SPIDIR)/spi.o: src/$(SPIDIR)/spi.c src/$(SPIDIR)/spi.h | out/$(SPIDIR)
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)
	
out/$(SPIDIR)/spi.a: out/$(SPIDIR)/spi.o
	$(AR) rcs $@ $^

out/$(SPIDIR):
	$(MKDIR) -p out/$(SPIDIR)

# CLEANUP
clean:
	$(RM) -rf out
