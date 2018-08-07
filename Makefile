CC=gcc
AR=ar
MKDIR=mkdir
RM=rm
CFLAGS= -std=c99 -Wall
LIBCFLAGS= -fPIC

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -O0 -g
endif

MAINDIR := main

.SECONDEXPANSION:
	
.PHONY: all clean lib test

out/%/: # Rule for dirs
	$(MKDIR) -p $@

########### ALL ##############
all: lib test

########### PIN ##############
PINDIR := $(MAINDIR)/platform_specific

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino
PLATFORM = $(DEFAULT_PLATFORM)

INCLUDE = -Isrc/$(PINDIR)/$(PLATFORM)

ifeq ($(strip $(PLATFORM)), raspberrypi)
	LIBS= -lwiringPi
endif

########### SPI ##############
SPIDIR := $(MAINDIR)/spi
OBJS += out/$(SPIDIR)/spi.o
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h) | $$(@D)/
	$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
	
########### CRC ##############
CRCDIR := $(MAINDIR)/crc
OBJS += out/$(CRCDIR)/crc.o
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | $$(@D)/
	$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: $(OBJS)
	$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### TESTS ############
TESTDIR := test
TESTDIR_CRC := $(TESTDIR)/crc
TESTLIBS := -lcunit -lmext2 

TESTOBJS := out/$(TESTDIR_CRC)/crc_test.o

test: out/$(TESTDIR)/test

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h | $$(@D)/
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(TESTDIR)/test: src/$(TESTDIR)/test.o $(TESTOBJS) out/$(MAINDIR)/libmext2.so
	$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 

########### CLEAN ############
clean:
	$(RM) -rf out
