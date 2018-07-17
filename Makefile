CC=gcc
AR=ar
MKDIR=mkdir
RM=rm
CFLAGS= -std=c99 -Wall

DEBUG=1

ifdef DEBUG
CFLAGS+= -O0 -g
endif

SRCDIR=src
OUTDIR=out

PINDIR=main/platform_specific
SPIDIR=main/spi
PLATFORM=raspberrypi

ifeq ($(PLATFORM), 'raspberrypi')
LIBS= -lwiringPi
endif

INCLUDE:= -I$(SRCDIR)/$(PINDIR)/$(PLATFORM)
INCLUDE+= -I$(SRCDIR)/$(SPIDIR)

all: $(OUTDIR)/$(SPIDIR)/spi.a

# SPI 
$(OUTDIR)/$(SPIDIR)/spi.o: $(SRCDIR)/$(SPIDIR)/spi.c $(SRCDIR)/$(SPIDIR)/spi.h | $(OUTDIR)/$(SPIDIR)
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)
	
$(OUTDIR)/$(SPIDIR)/spi.a: $(OUTDIR)/$(SPIDIR)/spi.o
	$(AR) rcs $@ $^

$(OUTDIR)/$(SPIDIR):
	mkdir -p $(OUTDIR)/$(SPIDIR)

# CLEANUP
clean:
	$(RM) -rf $(OUTDIR)
