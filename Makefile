CC=gcc
CFLAGS=-std=c99 -Wall

ifdef DEBUG
CFLAGS+= -O0 -g
endif

SRCDIR=src
OUTDIR=out

PINDIR=main/pin
PLATFORM=raspberrypi

ifeq ($(PLATFORM), 'raspberrypi')
PINLIBS=-lwiringPi
endif

INCLUDE+=-I$(SRCDIR)/$(PINDIR)

all: $(OUTDIR)/$(PINDIR)/pin.so

clean:
	rm -rf $(OUTDIR)

$(OUTDIR)/$(PINDIR)/pin.so: $(OUTDIR)/$(PINDIR)/pin.o 
	$(CC) -shared -o $@ $< $(PINLIBS) 
	rm $<

$(OUTDIR)/$(PINDIR)/pin.o: $(SRCDIR)/$(PINDIR)/$(PLATFORM)/pin.c \
	                       $(SRCDIR)/$(PINDIR)/pin.h
	mkdir -p $(OUTDIR)/$(PINDIR)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDE) -fpic $< 
