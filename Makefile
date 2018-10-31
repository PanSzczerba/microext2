CC=gcc
AR=ar
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
	
.PHONY: all clean lib test build_test build_manual manual_test

define print
	$(ECHO) "  $1 $2"
endef

out/%/: # Rule for dirs
	@$(call print, MKDIR, $(@))
	$E$(MKDIR) -p $@
	
out/$(MAINDIR)%.o: # rule for objects
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
	

########### ALL ##############
all: lib test

########### COMMON ###########
COMMONDIR = $(MAINDIR)/common
INCLUDE = -Isrc/$(COMMONDIR)
OBJS += out/$(COMMONDIR)/endianess.o

out/$(COMMONDIR)/endianess.o: $(addprefix src/$(COMMONDIR)/, endianess.c common.h) | $$(@D)/

###### PLATFORM SPECIFIC #####

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino
PLATFORM = $(strip $(DEFAULT_PLATFORM))

PLATFORMDIR := $(MAINDIR)/platform_specific/$(PLATFORM)

INCLUDE += -Isrc/$(PLATFORMDIR)

ifeq ($(strip $(PLATFORM)), raspberrypi)
	LIBS= -lwiringPi 
endif

ifeq ($(strip $(USE_LOGS)), 1)
	CFLAGS += -DMEXT2_MSG_DEBUG
	OBJS += out/$(PLATFORMDIR)/debug.o

out/$(PLATFORMDIR)/debug.o: $(addprefix src/$(PLATFORMDIR)/, debug.c debug.h)\
 src/$(COMMONDIR)/common.h | $$(@D)/
endif

########### SPI ##############
SPIDIR := $(MAINDIR)/spi
OBJS += out/$(SPIDIR)/spi.o
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h)\
 $(addprefix src/$(PLATFORMDIR)/, debug.h pin.h timing.h) | $$(@D)/

########### CRC ##############
CRCDIR := $(MAINDIR)/crc
OBJS += out/$(CRCDIR)/crc.o
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | $$(@D)/

########### SD ##############
SDDIR := $(MAINDIR)/sd
OBJS += $(addprefix out/$(SDDIR)/, $(patsubst %.c,%.o, $(notdir $(wildcard src/$(SDDIR)/*.c))))
INCLUDE += -Isrc/$(SDDIR)

out/$(SDDIR)/command.o: $(addprefix src/$(SDDIR)/, command.c command.h sd.h)\
 src/$(COMMONDIR)/common.h $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h timing.h)\
 src/$(CRCDIR)/crc.h | $$(@D)/

out/$(SDDIR)/init.o: $(addprefix src/$(SDDIR)/, init.c sd.h) src/$(COMMONDIR)/common.h\
 $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h timing.h)| $$(@D)/

out/$(SDDIR)/rw.o: $(addprefix src/$(SDDIR)/, rw.c sd.h) src/$(COMMONDIR)/common.h\
 $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h) src/$(SPIDIR)/spi.h | $$(@D)/

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: $(OBJS)
	@$(call print, LD, $(@))
	$E$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### TESTS ############
TESTDIR := test
TESTDIR_CRC := $(TESTDIR)/crc
TESTDIR_ENDIANESS := $(TESTDIR)/endianess
TESTLIBS := -lcunit 
TEST_INCLUDE := 


TEST_BINS = out/$(TESTDIR_CRC)/test out/$(TESTDIR_ENDIANESS)/test

build_test: $(TEST_BINS)

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h $(wildcard $(@D)/stubs/*) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -c -o $@ $< $(TEST_INCLUDE) 

out/$(TESTDIR_CRC)/test: $(patsubst src%,out%,$(patsubst %.c,%.o,$(wildcard src/$(TESTDIR_CRC)/*.c)))\
 out/$(CRCDIR)/crc.o 
	@$(call print, LD, $(@))
	$E$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 

out/$(TESTDIR_ENDIANESS)/test: $(patsubst src%,out%,$(patsubst %.c,%.o,\
 $(wildcard src/$(TESTDIR_ENDIANESS)/*.c)))\
 out/$(COMMONDIR)/endianess.o 
	@$(call print, LD, $(@))
	$E$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 

test: build_test
	@$(call print, RUN, out/$(TESTDIR_CRC)/test)
	$Eout/$(TESTDIR_CRC)/test | tail -6; echo 
	@$(call print, RUN, out/$(TESTDIR_ENDIANESS)/test)
	$Eout/$(TESTDIR_ENDIANESS)/test | tail -6; echo


######### MANUAL TESTS ########
MANUAL_TESTDIR := $(TESTDIR)/manual

MANUAL_BINS = out/$(MANUAL_TESTDIR)/test
MANUAL_LIBS = -Lout/$(MAINDIR) $(LIBS) -lmext2

build_manual: $(MANUAL_BINS) 

out/$(MANUAL_TESTDIR)/%.o: src/$(MANUAL_TESTDIR)/%.c src/$(MANUAL_TESTDIR)/%.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(MANUAL_TESTDIR)/test: out/$(MANUAL_TESTDIR)/test.o out/$(MAINDIR)/libmext2.so
	@$(call print, CC, $(@))
	$E$(CC) $(INCLUDE) -o $@ $^ $(MANUAL_LIBS)

manual_test: build_manual
	@$(call print, RUN, out/$(MANUAL_TESTDIR)/test)
	$Eout/$(MANUAL_TESTDIR)/test


########### CLEAN ############
clean:
	@$(call print, RM, out)
	$E$(RM) -rf out
