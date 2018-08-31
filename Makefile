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
	
.PHONY: all clean lib test build_test run_test

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

ifeq ($(strip $(DEBUG)), 1)
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

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

# SD 
out/main/sd/sd.a:out/main/sd/common.o out/main/sd/init.o out/main/sd/rw.o
	$(AR) rcs $@ $^

out/main/sd/common.o: src/main/sd/common.c src/main/sd/sd.h | out/main/sd
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

out/main/sd/init.o: src/main/sd/init.c src/main/sd/sd.h | out/main/sd
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

out/main/sd/rw.o: src/main/sd/rw.c src/main/sd/sd.h | out/main/sd
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

out/main/sd:
	$(MKDIR) -p out/main/sd

out/$(MAINDIR)/libmext2.so: $(OBJS)
	@$(call print, LD, $(@))
	$E$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### TESTS ############
TESTDIR := test
TESTDIR_CRC := $(TESTDIR)/crc
TESTLIBS := -lcunit 
TEST_INCLUDE := 

#TESTOBJS := out/$(TESTDIR_CRC)/crc_test.o

test: build_test run_test

TEST_BINS = out/$(TESTDIR_CRC)/test

build_test: $(TEST_BINS)

out/$(TESTDIR)/%.o: src/$(TESTDIR)/%.c src/$(TESTDIR)/%.h | $$(@D)/
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) -c -o $@ $< $(TEST_INCLUDE) 

out/$(TESTDIR_CRC)/test: $(patsubst src%,out%,$(patsubst %.c,%.o,$(wildcard src/$(TESTDIR_CRC)/*.c))) out/$(CRCDIR)/crc.o $(USE_DEBUG_OBJ)
	@$(call print, LD, $(@))
	$E$(CC) -o $@ $(filter %.o,$^) -Lout/$(MAINDIR) $(TESTLIBS) 
	
run_test: build_test
	@$(call print, RUN, out/$(TESTDIR_CRC)/test)
	$Eout/$(TESTDIR_CRC)/test

########### CLEAN ############
clean:
	@$(call print, RM, out)
	$E$(RM) -rf out
