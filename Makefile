CC=gcc
MKDIR=mkdir
RM=rm
ECHO=echo
CFLAGS= -std=c99 -Wall
LIBCFLAGS= -fPIC

ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -O0 -g
endif

ifneq ($(VERBOSE), 1)
	E=@
endif

MAINDIR := main

.SECONDEXPANSION:

.PRECIOUS: out/%/
	
.PHONY: all clean lib test run_tests

define print
	$(ECHO) "  $1 $2"
endef

out/%/: # Rule for dirs
	@$(call print, MKDIR, $(@))
	$E$(MKDIR) -p $@

########### ALL ##############
all: lib test

###### PLATFORM SPECIFIC #####

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino
PLATFORM = $(strip $(DEFAULT_PLATFORM))

PLATFORMDIR := $(MAINDIR)/platform_specific/$(PLATFORM)

INCLUDE = -Isrc/$(PLATFORMDIR)

ifeq ($(strip $(PLATFORM)), raspberrypi)
	LIBS= -lwiringPi
endif

ifeq ($(strip $(DEBUG)), 1)
	OBJS += out/$(PLATFORMDIR)/debug.o

out/$(PLATFORMDIR)/debug.o: $(addprefix src/$(PLATFORMDIR)/, debug.c debug.h) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

endif

########### SPI ##############
SPIDIR := $(MAINDIR)/spi
OBJS += out/$(SPIDIR)/spi.o
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h) $(addprefix src/$(PLATFORMDIR)/, debug.h pin.h timing.h) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
	
########### CRC ##############
CRCDIR := $(MAINDIR)/crc
OBJS += out/$(CRCDIR)/crc.o
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: $(OBJS)
	@$(call print, CC, $(@))
	$E$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### TESTS ############
TESTDIR := test
TESTDIR_CRC := $(TESTDIR)/crc
TESTLIBS := -lcunit 

#TESTOBJS := out/$(TESTDIR_CRC)/crc_test.o

test: out/$(TESTDIR_CRC)/test 

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(TESTDIR_CRC)/test: $(patsubst src%,out%,$(patsubst %.c,%.o,$(wildcard src/$(TESTDIR_CRC)/*.c))) out/$(CRCDIR)/crc.o
	@$(call print, CC, $(@))
	$E$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 
	
run_tests: test
	@$(call print, RUN, out/$(TESTDIR_CRC)/test)
	$Eout/$(TESTDIR_CRC)/test

########### CLEAN ############
clean:
	@$(call print, RM, out)
	$E$(RM) -rf out
