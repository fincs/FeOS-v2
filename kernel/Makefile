#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifeq ($(strip $(FEOSPLAT)),)
$(error "Please set FEOSPLAT in your environment. Valid values: 3ds rpi qemu")
endif

include $(DEVKITARM)/base_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files embedded using bin2o
# GRAPHICS is a list of directories containing image files to be converted with grit
#---------------------------------------------------------------------------------
ifeq ($(DEBUGVER),)
TARGET   := FeOS.$(FEOSPLAT)
BUILD    := build.$(FEOSPLAT)
else
TARGET   := FeOSd.$(FEOSPLAT)
BUILD    := build_dbg.$(FEOSPLAT)
endif
SOURCES  := source source/$(FEOSPLAT) drivers/$(FEOSPLAT)
INCLUDES := ../sdk/include/feos/kernel
DATA     := data data/$(FEOSPLAT)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

ifeq ($(FEOSPLAT),3ds)
CPU := mpcore
else ifeq ($(FEOSPLAT),rpi)
CPU := arm1176jzf-s
else ifeq ($(FEOSPLAT),qemu)
CPU := arm1176jzf-s
else
$(error "Unsupported platform")
endif

ARCH := -mthumb-interwork -mcpu=$(CPU) -mtune=$(CPU) -mfpu=vfp -mfloat-abi=hard

DEFINES	 := -DFEOS -DFEOS_KERNEL -DFEOS_$(FEOSPLAT) -DFEOS_PLATINCLUDE=\"$(FEOSPLAT)/platform.h\"
ifeq ($(DEBUGVER),)
COPTFLAG :=	-O2
DEFINES  += -DNDEBUG
else
COPTFLAG := -O0
DEFINES  += -DDEBUG
endif

ifeq ($(CPU),mpcore)
DEFINES += -DHAS_PIPT_DCACHE
else ifeq ($(CPU),arm1176jzf-s)
DEFINES += -DHAS_FAST_CACHE_RANGE_OPS
endif

CFLAGS := -g -Wall $(COPTFLAG) -funwind-tables -save-temps \
          -fomit-frame-pointer \
          -ffast-math \
		  -fshort-wchar \
          $(ARCH)

CFLAGS   += $(INCLUDE) $(DEFINES)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -nostdinc++ -std=gnu++11

ASFLAGS := -g $(ARCH) $(DEFINES)
LDFLAGS  = -nostartfiles -nodefaultlibs -T $(TOPDIR)/ldscript/$(FEOSPLAT).spec -T $(TOPDIR)/ldscript/kernel.ld -g $(ARCH) $(foreach func,$(HOOKS),-Wl,-wrap,$(func)) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order is important)
#---------------------------------------------------------------------------------
LIBS := -lgcc

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS :=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export TOPDIR := $(CURDIR)
export OUTPUT := $(CURDIR)/$(TARGET)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES :=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
 
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif

export OFILES := $(addsuffix .o,$(BINFILES)) \
                 $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD) \
                  -iquote $(CURDIR)/source

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo Cleaning...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).img

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).img : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)
 
#---------------------------------------------------------------------------------
%.bin.o : %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.img: %.elf
	@$(OBJCOPY) -O binary $< $@
	@echo Built: $(notdir $@)

#---------------------------------------------------------------------------------
%.elf:
	@echo Linking $(notdir $@)...
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
