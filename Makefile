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

########### ALL ##############
all: lib test

########### COMMON ###########
COMMONDIR = $(MAINDIR)/common
INCLUDE = -Isrc/$(COMMONDIR)

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
	DEBUG_OBJ = out/$(PLATFORMDIR)/debug.o
	OBJS += $(DEBUG_OBJ)

out/$(PLATFORMDIR)/debug.o: $(addprefix src/$(PLATFORMDIR)/, debug.c debug.h) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -DMEXT2_MSG_DEBUG $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
endif

########### SPI ##############
SPIDIR := $(MAINDIR)/spi
OBJS += out/$(SPIDIR)/spi.o
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h) $(addprefix src/$(PLATFORMDIR)/, debug.h pin.h timing.h) $(DEBUG_OBJ) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 
	
########### CRC ##############
CRCDIR := $(MAINDIR)/crc
OBJS += out/$(CRCDIR)/crc.o
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 


########### SD ##############
SDDIR := $(MAINDIR)/sd
OBJS += $(addprefix out/$(SDDIR)/, $(patsubst %.c,%.o, $(notdir $(wildcard src/$(SDDIR)/*.c))))
INCLUDE += -Isrc/$(SDDIR)

#out/$(SDDIR)/sd.a: out/$(SDDIR)/common.o out/$(SDDIR)/init.o out/$(SDDIR)/rw.o
#	@$(call print, AR, $(@))
#	$E$(AR) rcs $@ $^

out/$(SDDIR)/command.o: src/$(SDDIR)/command.c src/$(SDDIR)/command.h src/$(SDDIR)/sd.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

out/$(SDDIR)/init.o: src/$(SDDIR)/init.c src/$(SDDIR)/sd.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE)

out/$(SDDIR)/rw.o: src/$(SDDIR)/rw.c src/$(SDDIR)/sd.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE)

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: $(OBJS)
	@$(call print, LD, $(@))
	$E$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### TESTS ############
TESTDIR := test
TESTDIR_CRC := $(TESTDIR)/crc
TESTLIBS := -lcunit 
TEST_INCLUDE := 

#TESTOBJS := out/$(TESTDIR_CRC)/crc_test.o

TEST_BINS = out/$(TESTDIR_CRC)/test

build_test: $(TEST_BINS)

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -c -o $@ $< $(TEST_INCLUDE) 

out/$(TESTDIR_CRC)/test: $(patsubst src%,out%,$(patsubst %.c,%.o,$(wildcard src/$(TESTDIR_CRC)/*.c))) out/$(CRCDIR)/crc.o $(USE_DEBUG_OBJ)
	@$(call print, LD, $(@))
	$E$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 

test: build_test
	@$(call print, RUN, out/$(TESTDIR_CRC)/test)
	$Eout/$(TESTDIR_CRC)/test


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
