CC=gcc
AR=ar
MKDIR=mkdir
RM=rm
ECHO=echo
CFLAGS= -std=c99 -Wall -Wno-packed-bitfield-compat 
LIBCFLAGS= -fPIC 


ifeq ($(strip $(DEBUG)), 1)
	CFLAGS += -O0 -g 
endif

ifneq ($(VERBOSE), 1)
	E=@
endif


.SECONDEXPANSION:

.PRECIOUS: out/%/
	
.PHONY: all clean lib test build_test build_manual manual_test

define print
	$(ECHO) "  $1 $2"
endef

######## DIRS ###########
MAINDIR := main
COMMONDIR = $(MAINDIR)/common

DEFAULT_PLATFORM := raspberrypi # raspberrypi, arduino
PLATFORM = $(strip $(DEFAULT_PLATFORM))
PLATFORMDIR := $(MAINDIR)/platform_specific/$(PLATFORM)
SPIDIR := $(MAINDIR)/spi
CRCDIR := $(MAINDIR)/crc
SDDIR := $(MAINDIR)/sd
FSDIR := $(MAINDIR)/fs
FILEDIR := $(MAINDIR)/file

TESTDIR := test
UTTESTDIR := $(TESTDIR)/UT
TESTDIR_CRC := $(UTTESTDIR)/crc
TESTDIR_ENDIANESS := $(UTTESTDIR)/endianess
MANUAL_TESTDIR := $(TESTDIR)/manual

######## RULES ##########
out/%/: # Rule for dirs
	@$(call print, MKDIR, $(@))
	$E$(MKDIR) -p $@
	
out/$(MAINDIR)%.o: # rule for objects
	@$(call print, CC, $(@))
	$E$(CC) $(CFLAGS) $(LIBCFLAGS) -c -o $@ $< $(INCLUDE) 

########### ALL ##############
all: lib test

########### COMMON ###########
INCLUDE = -Isrc/$(COMMONDIR)
OBJS += out/$(COMMONDIR)/endianess.o out/$(COMMONDIR)/blocks.o

out/$(COMMONDIR)/endianess.o: $(addprefix src/$(COMMONDIR)/, endianess.c common.h) | $$(@D)/

out/$(COMMONDIR)/blocks.o: $(addprefix src/$(COMMONDIR)/, blocks.c limit.h common.h) | $$(@D)/

###### PLATFORM SPECIFIC #####

INCLUDE += -Isrc/$(PLATFORMDIR)

ifeq ($(strip $(PLATFORM)), raspberrypi)
	LIBS= -lwiringPi 
endif

ifneq ($(strip $(NO_LOGS)), 1)
	CFLAGS += -DMEXT2_MSG_DEBUG
	OBJS += out/$(PLATFORMDIR)/debug.o

out/$(PLATFORMDIR)/debug.o: $(addprefix src/$(PLATFORMDIR)/, debug.c debug.h)\
 src/$(COMMONDIR)/common.h | $$(@D)/
endif

########### SPI ##############
OBJS += out/$(SPIDIR)/spi.o
INCLUDE += -Isrc/$(SPIDIR)

out/$(SPIDIR)/spi.o: $(addprefix src/$(SPIDIR)/, spi.c spi.h)\
 $(addprefix src/$(PLATFORMDIR)/, debug.h pin.h timing.h) | $$(@D)/

########### CRC ##############
OBJS += out/$(CRCDIR)/crc.o
INCLUDE += -Isrc/$(CRCDIR)

out/$(CRCDIR)/crc.o: $(addprefix src/$(CRCDIR)/, crc.c crc.h) | $$(@D)/

########### SD ##############
OBJS += $(addprefix out/$(SDDIR)/, $(patsubst %.c,%.o, $(notdir $(wildcard src/$(SDDIR)/*.c))))
INCLUDE += -Isrc/$(SDDIR)

out/$(SDDIR)/command.o: $(addprefix src/$(SDDIR)/, command.c command.h sd.h)\
 src/$(COMMONDIR)/common.h $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h timing.h)\
 src/$(CRCDIR)/crc.h | $$(@D)/

out/$(SDDIR)/init.o: $(addprefix src/$(SDDIR)/, init.c sd.h) $(addprefix src/$(COMMONDIR)/, common.h)\
 $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h timing.h) src/$(FSDIR)/fs.h | $$(@D)/

out/$(SDDIR)/rw.o: $(addprefix src/$(SDDIR)/, rw.c sd.h) src/$(COMMONDIR)/common.h\
 $(addprefix src/$(PLATFORMDIR)/, pin.h debug.h) src/$(SPIDIR)/spi.h | $$(@D)/

########### FS ###############
INCLUDE += -Isrc/$(FSDIR)
OBJS += $(addprefix out/$(FSDIR)/, fs.o ext2/ext2.o)

out/$(FSDIR)/fs.o : $(addprefix src/$(FSDIR)/, fs.c fs.h ext2/ext2.h ext2/superblock.h)\
 $(addprefix src/$(SDDIR)/, sd.h) $(addprefix src/$(COMMONDIR)/, common.h) $(addprefix src/$(PLATFORMDIR)/, debug.h) | $$(@D)/

out/$(FSDIR)/ext2/ext2.o : $(addprefix src/$(FSDIR)/, ext2/ext2.c ext2/ext2.h ext2/ext2_descriptor.h ext2/superblock.h)\
 $(addprefix src/$(SDDIR)/,sd.h) $(addprefix src/$(COMMONDIR)/, common.h) $(addprefix src/$(FILEDIR)/, ext2/file.h) | $$(@D)/

########## FILE ##############
INCLUDE += -Isrc/$(FILEDIR)
OBJS += $(addprefix out/$(FILEDIR)/, file.o ext2/file.o)

out/$(FILEDIR)/file.o : $(addprefix src/$(FILEDIR)/, file.c file.h) $(addprefix src/$(SDDIR)/, sd.h)\
 $(addprefix src/$(COMMONDIR)/, limit.h common.h) $(addprefix src/$(PLATFORMDIR)/, debug.h) | $$(@D)/

out/$(FILEDIR)/ext2/file.o : $(addprefix src/$(FILEDIR)/, ext2/file.c ext2/file.h) \
 $(addprefix src/$(FSDIR)/, ext2/ext2.h ext2/inode_descriptor.h) | $$(@D)/

########## LIBRARY ###########
lib: out/$(MAINDIR)/libmext2.so

out/$(MAINDIR)/libmext2.so: $(OBJS)
	@$(call print, LD, $(@))
	$E$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $^ $(LIBS)

########### UT  ##############
TESTLIBS := -lcunit 
TEST_INCLUDE := 

TEST_BINS = out/$(TESTDIR_CRC)/test out/$(TESTDIR_ENDIANESS)/test

build_test: $(TEST_BINS)

out/$(UTTESTDIR)/%.o: src/$(UTTESTDIR)/%.c src/$(UTTESTDIR)/%.h $(wildcard $(@D)/stubs/*) | $$(@D)/
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
