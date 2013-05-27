
PRJ			= playa

### Machine flags
#
CC_CMACH	= -mmcu=atmega328p 
CC_DMACH	= -D__AVR_ATmega328P__
CC_CFREQ	= -DF_CPU=8000000L
### Build flags
#
# -fdata-sections, -ffunction-sections and -Wl,--gc-sections -Wl,-s
# are used for dead code elimination, see:
# http://gcc.gnu.org/ml/gcc-help/2003-08/msg00128.html
#

# Optimiser flags.
#     optimise for size, unsigned by default, pack data.
#     separate sections, drop unused ones, shorten branches, jumps.
#     don't inline, vectorise loops. no exceptions.
#     no os preamble, use function calls in prologues.
# http://gcc.gnu.org/onlinedocs/gcc-4.3.5/gcc/
# http://www.tty1.net/blog/2008-04-29-avr-gcc-optimisations_en.html
OPT_FLAGS = -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
	-ffunction-sections -fdata-sections -Wl,--gc-sections,--relax \
	-fno-inline-small-functions -fno-tree-scev-cprop -fno-exceptions \
	-ffreestanding -mcall-prologues -Xlinker --allow-multiple-definition

CFLAGS		+= $(CC_CMACH) $(CC_DMACH) $(CC_CFREQ) $(OPT_FLAGS)
CFLAGS		+= -Wall -Wstrict-prototypes -Wextra -Wundef -Wno-unused-parameter -Wno-ignored-qualifiers
CFLAGS		+= -Wl,-Map=output.map
CFLAGS		+= -B/usr/avr/lib/avr5/
#CFLAGS		+= -fno-force-addr -finline-limit=1 -fno-schedule-insns
LDFLAGS		=

CFLAGS_REL	+= $(OPT_FLAGS) -fomit-frame-pointer
LDFLAGS_REL	+= 

CFLAGS_DBG	+= -O1 -ggdb
LDFLAGS_DBG	+= -Wl,--gc-sections

# linker flags and include directories
INCLUDES	+= -I./ -I./drivers

### Build tools
# 
AVR_TOOLS_PATH	= /usr/bin

CC		= $(AVR_TOOLS_PATH)/avr-gcc
LD		= $(AVR_TOOLS_PATH)/avr-ld
AS		= $(AVR_TOOLS_PATH)/avr-as
AR		= $(AVR_TOOLS_PATH)/avr-ar
OBJCOPY = $(AVR_TOOLS_PATH)/avr-objcopy
OBJDUMP = $(AVR_TOOLS_PATH)/avr-objdump
CPPFILT = $(AVR_TOOLS_PATH)/avr-c++filt
SIZE    = $(AVR_TOOLS_PATH)/avr-size


SUBDIRS = drivers fatfs

BASH := $(shell which bash || which bash)

.PHONY: all
.PHONY: clean
.PHONY: install
.PHONY: depend
.PHONY: doc
.PHONY: tags
.PHONY: force

all: depend config.h tags $(PRJ).hex

#
# Build list of sources and objects to build
SRCS := $(wildcard *.c)
$(foreach subdir,$(SUBDIRS), \
	$(eval SRCS := $(SRCS) $(wildcard $(subdir)/*.c)) \
)
OBJS := $(patsubst %.c,%.o,$(SRCS))

#
# Dependencies rules
depend: $(PRJ).dep

$(PRJ).dep: $(SRCS)
	@echo "Generating dependencies.."
	@touch $@
	@makedepend $(INCLUDES) -Y -f $@ $^ &> /dev/null
	@rm -f $@.bak

#
# Append specific CFLAGS/LDFLAGS
DEBUG := $(shell grep "^\#define DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
CFLAGS	+= $(CFLAGS_REL)
LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
CFLAGS	+= $(CFLAGS_DBG)
LDFLAGS	+= $(LDFLAGS_DBG)
endif

# rebuild if CFLAGS changed, as suggested in:
# http://stackoverflow.com/questions/3236145/force-gnu-make-to-rebuild-objects-affected-by-compiler-definition/3237349#3237349
$(PRJ).cflags: force
	@echo "$(CFLAGS)" | cmp -s - $@ || echo "$(CFLAGS)" > $@

$(OBJS): $(PRJ).cflags
#
# Top rules

$(PRJ).elf: $(OBJS)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $@ $+	

$(PRJ).hex: $(PRJ).elf
	@$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load \
		--no-change-warnings --change-section-lma .eeprom=0 $< $(PRJ).eep
	@$(OBJCOPY) -O ihex -R .eeprom $< $@
	@$(OBJDUMP) -h -S $< | $(CPPFILT) -t > $(PRJ).lst
	@$(SIZE) $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

config.h:
	@echo "Please do a 'make config' first!" && false

new_timestamps:
	@$(BASH) ./tools/update_rtca_now.sh

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

#install: $(PRJ).txt
#	mspdebug -j olimex-iso-mk2 "prog $<"

clean: $(SUBDIRS)
	@for subdir in $(SUBDIRS); do \
		echo "Cleaning $$subdir .."; rm -f $$subdir/*.o; \
	done
	@rm -f *.o $(PRJ).{elf,txt,hex,lst,eep,cflags,dep} output.map

doc:
	rm -rf doc/*
	doxygen Doxyfile
	
-include $(PRJ).dep
