#==============================================================================
#
# Makefile for building MSP430 DriverLib Code examples from command line
# environment using the GCC Open Source Compiler for MSP430
#
#==============================================================================

DEVICE:=armv7a
PROJ:=main
SUBDIRS:= 

################## GCC Root Variable ###################
GCC_DIR     ?= /usr
GCC_MSP_INC_DIR ?= $(GCC_DIR)/msp430-elf/lib
DRIVERLIB   := ./tools
GLUELIB     := ./drivers
RM          := rm -f

######################################
GCC_BIN_DIR      ?= $(GCC_DIR)/bin
GCC_INC_DIR      ?= $(GCC_DIR)/msp430-elf/include
######################################
CC               := $(GCC_BIN_DIR)/armv7a-hardfloat-linux-gnueabihf-gcc
GDB	             := $(GCC_BIN_DIR)/gdb
######################################

# -fdata-sections, -ffunction-sections and -Wl,--gc-sections -Wl,-s
# are used for dead code elimination, see:
# http://gcc.gnu.org/ml/gcc-help/2003-08/msg00128.html

CFLAGS           := -D__$(DEVICE)__
CFLAGS           += -Wall -Wno-switch
CFLAGS		     += -fno-force-addr -finline-limit=1 -fno-schedule-insns
CFLAGS		     += -Wl,-Map=output.map
LDFLAGS          := -L $(GCC_MSP_INC_DIR) -g -Wl,--gc-sections
CFLAGS_REL	     += -Os -fdata-sections -ffunction-sections -fomit-frame-pointer
LDFLAGS_REL	     += -Wl,-s
CFLAGS_DBG	     += -O1 -ggdb
LDFLAGS_DBG	     += 
INCLUDES         := -I $(GCC_MSP_INC_DIR) -I $(GCC_INC_DIR) -I $(DRIVERLIB) -I $(GLUELIB) -I ./
######################################
DRIVERLIB_SRC    := $(wildcard $(DRIVERLIB)/*.c)
GLUELIB_SRC      := $(wildcard $(GLUELIB)/*.c)
LOCAL_SRC        := $(wildcard *.c)
PROJ_SRC         := $(PROJ).c
SRC              := $(PROJ_SRC) $(DRIVERLIB_SRC) $(GLUELIB_SRC) $(LOCAL_SRC)
DRIVERLIB_OBJECTS:= $(patsubst %.c,%.o,$(DRIVERLIB_SRC))
GLUELIB_OBJECTS  := $(patsubst %.c,%.o,$(GLUELIB_SRC))
LOCAL_OBJECTS    := $(patsubst %.c,%.o,$(LOCAL_SRC))
EXOBJECT         := $(PROJ).o
EXOUTPUT         := $(PROJ).elf
######################################

# Append specific CFLAGS/LDFLAGS
DEBUG := $(shell grep "^\#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
CFLAGS	+= $(CFLAGS_REL)
LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
CFLAGS	+= $(CFLAGS_DBG)
LDFLAGS	+= $(LDFLAGS_DBG)
endif

.PHONY: all
.PHONY: clean
.PHONY: install
.PHONY: config
.PHONY: depend
.PHONY: driverlib
.PHONY: doc
.PHONY: cflags_update

all: depend config.h tags driverlib $(EXOUTPUT)

depend:	$(PROJ).dep

$(PROJ).dep: $(SRC)
	@echo "Generating dependencies.."
	@touch $@
	@makedepend $(INCLUDES) -Y -f $@ $^ &> /dev/null
	@grep -v ^\# $(PROJ).dep | cut -d" " -f2 | sort -u | xargs > $(PROJ).hdep
	@echo "tags: `cat $(PROJ).hdep`" >> $(PROJ).dep
	@rm -f $@.bak

config.h:
	@echo "config.h file is missing" && false

tags: $(PROJ_SRC) $(DRIVERLIB_SRC) $(GLUELIB_SRC) $(LOCAL_SRC) $(PROJ).dep
	@echo "Generating tags .."
	echo $(deps)
	@exuberant-ctags --recurse=yes ./ $(shell cat $(PROJ).hdep)

%.o: %.c
	@echo "CC $<"
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(EXOBJECT): $(PROJ_SRC) $(PROJ).cflags
	@echo "CC $<"
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(EXOUTPUT): $(DRIVERLIB_OBJECTS) $(GLUELIB_OBJECTS) $(LOCAL_OBJECTS) $(EXOBJECT)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(LDFLAGS) $(LOCAL_OBJECTS) -o $@ && size $@
#	$(CC) $(LDFLAGS) $(EXOBJECT) $(DRIVERLIB_OBJECTS) $(GLUELIB_OBJECTS) $(LOCAL_OBJECTS) -o $@ && size $@

debug: all
	$(GDB) $(PROJ).elf

clean:
	$(RM) *.o $(PROJ).{elf,cflags,dep,hdep} output.map
	$(RM) tags

cleanlib:
	$(RM) $(DRIVERLIB_OBJECTS) $(GLUELIB_OBJECTS)

install: $(PROJ).elf
	@bash ./tools/burn.sh $<

# rebuild if CFLAGS changed, as suggested in:
# http://stackoverflow.com/questions/3236145/force-gnu-make-to-rebuild-objects-affected-by-compiler-definition/3237349#3237349
$(PROJ).cflags: cflags_update
	@echo "$(CFLAGS)" | cmp -s - $@ || echo "$(CFLAGS)" > $@

-include $(PROJ).dep
