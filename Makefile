CC=gcc
AR=ar
MKDIR=mkdir
RM=rm
CFLAGS= -std=c99 -Wall -fPIC
LIBCFLAGS= -fPIC

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -O0 -g
endif

MAINDIR := main
PINDIR := $(MAINDIR)/platform_specific

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino
PLATFORM := $(DEFAULT_PLATFORM)

ifeq ($(strip $(PLATFORM)), raspberrypi)
	LIBS= -lwiringPi
endif

INCLUDE := -Isrc/$(PINDIR)/$(PLATFORM)

all: lib test

# SPI 
SPIDIR := $(MAINDIR)/spi
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h) | out/$(SPIDIR)
	$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
	
out/$(SPIDIR):
	$(MKDIR) -p out/$(SPIDIR)

# CRC
CRCDIR := $(MAINDIR)/crc
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | out/$(CRCDIR)
	$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(CRCDIR):
	$(MKDIR) -p out/$(CRCDIR)
	
	
# LIBRARY
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: out/$(SPIDIR)/spi.o out/$(CRCDIR)/crc.o
	$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

# TESTS
TESTDIR := test
TESTLIBS := -lcunit -lmext2 
test: out/$(TESTDIR)/test

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h
	$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(TESTDIR)/test: src/$(TESTDIR)/test.o out/$(TESTDIR)/crc/crc_test.o out/$(MAINDIR)/libmext2.so
	$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 

out/$(TESTDIR):
	$(MKDIR) -p out/$(TESTDIR)

# CLEANUP
clean:
	$(RM) -rf out
