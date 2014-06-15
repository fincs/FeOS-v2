
include $(FEOSMK)/vars.mk

ifeq ($(FEOSPLAT),3ds)
	CPUTYPE := mpcore
	FXEPLAT := 0x3D5
else ifeq ($(FEOSPLAT),rpi)
	CPUTYPE := arm1176jzf-s
	FXEPLAT := 0x314
else ifeq ($(FEOSPLAT),qemu)
	CPUTYPE := arm1176jzf-s
	FXEPLAT := 0x001
else
	FEOSPLAT :=
	CPUARCH := -march=armv6
	FXEPLAT := 0x000
endif

CPUARCH ?= -mcpu=$(CPUTYPE) -mtune=$(CPUTYPE)

ifneq ($(CONF_TARGET),kernel)

ifeq ($(CONF_TARGET),app)
	MODULEBASE := 0x10000
else
	MODULEBASE := 0
endif

ifeq ($(CONF_TARGET),dynlib)
	CONF_EXPLIB := 1
	ENTRYPOINT  := __FXE_DllEntry
endif

ifeq ($(CONF_TARGET),kstaticlib)
	CONF_KMOD   := 1
	CONF_TARGET := staticlib
endif

ifeq ($(CONF_TARGET),kmod)
	CONF_KMOD   := 1
	CONF_TARGET := dynlib
	CONF_EXPLIB := 1
	CRTSUFFIX   := k
	ENTRYPOINT  := __FXE_KModEntry
endif

ENTRYPOINT ?= __FXE_Entry

else
	CONF_KMOD     := 1
	ifeq ($(FEOSPLAT),)
$(error "Please set FEOSPLAT in your environment. Valid values: 3ds rpi qemu")
	endif

	# Compiler & linker flags for compiling a FeOS kernel
	CONF_CFLAGS   += -iquote $(TOPDIR)/source
	CONF_CXXFLAGS += -fno-exceptions
	CONF_NOSTDLIB := 1
	CONF_LIBS     += -lgcc

	# CPU configuration defines
	CONF_DEFINES += -DFXEPLAT=$(FXEPLAT)
	ifeq ($(CPUTYPE),mpcore)
		CONF_DEFINES += -DHAS_PIPT_DCACHE
	else ifeq ($(CPUTYPE),arm1176jzf-s)
		CONF_DEFINES += -DHAS_FAST_CACHE_RANGE_OPS
	endif
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

ARCH := $(CPUARCH) -mfpu=vfp -mfloat-abi=hard

# FeOS base defines
DEFINES := -DFEOS
ifeq ($(CONF_KMOD),1)
	DEFINES += -DFEOS_KERNEL
	ifneq ($(CONF_TARGET),kernel)
		DEFINES += -DFEOS_KMODULE
	endif
endif
ifneq ($(FEOSPLAT),)
	DEFINES += -DFEOS_$(FEOSPLAT) -DFEOS_PLAT=\"$(FEOSPLAT)\" -DFEOS_PLATINCLUDE=\"$(FEOSPLAT)/platform.h\"
endif

# Horrid hack to get rid of format warnings
DEFINES += -U __INT32_TYPE__ -D__INT32_TYPE__=int -U __UINT32_TYPE__ -D__UINT32_TYPE__="unsigned int"

# Debugging defines
ifneq ($(CONF_DEBUG),1)
	COPTFLAG := -O2
	DEFINES  += -DNDEBUG
else
	COPTFLAG := -Og
	DEFINES  += -DDEBUG
	CRTSUFFIX := $(CRTSUFFIX)d
endif

# User defines
DEFINES += $(CONF_DEFINES)

ifneq ($(CONF_KMOD),1)
INCLUDEC   := -I$(FEOS2SDK)/include
INCLUDECXX := -I$(FEOS2SDK)/include/cxx
else
INCLUDEC   := -I$(FEOS2SDK)/include/feos/kernel
INCLUDECXX :=
endif

ifeq ($(CONF_GCSECTIONS),1)
	GCSECTIONS := --gc-sections,
endif

ARMARCH   := -marm
THUMBARCH := -mthumb
DEFARCH   ?= $(ARMARCH)

CFLAGS := -g -Wall $(COPTFLAG) -funwind-tables -save-temps -fvisibility=hidden \
          -fomit-frame-pointer -ffast-math -fshort-wchar -mthumb-interwork \
          $(ARCH) $(DEFINES) $(INCLUDEC) $(INCLUDE) $(CONF_CFLAGS)

CXXFLAGS := $(CFLAGS) $(INCLUDECXX) -fno-rtti -nostdinc++ -std=gnu++11 -fvisibility-inlines-hidden \
            -Wno-delete-non-virtual-dtor $(CONF_CXXFLAGS)

ASFLAGS  := -g $(ARCH) $(DEFINES) $(INCLUDE)

ifneq ($(CONF_TARGET),kernel)
LDFLAGS  := -nostartfiles -nostdlib -T $(FEOSBIN)/fxe.ld $(ARCH) \
            -Wl,-d,-q,$(GCSECTIONS)--use-blx,-Map,$(TARGETNAME).map,--defsym=__modulebase=$(MODULEBASE),-e,$(ENTRYPOINT) \
            $(CONF_LDFLAGS)
else
LDFLAGS  := -nostartfiles -nostdlib -T $(TOPDIR)/ldscript/$(FEOSPLAT).spec -T $(TOPDIR)/ldscript/kernel.ld $(ARCH) \
            -Wl,-Map,$(TARGETNAME).map \
            $(CONF_LDFLAGS)
endif

ifeq ($(CONF_NOSTDLIB),)
LIBS     := $(CXXLIB) -lfeoscrt$(CRTSUFFIX) -lgcc
endif
LIBS     := $(CONF_LIBS) $(LIBS)

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
ifneq ($(CONF_NOSTDLIB),1)
STDLIBDIR := -L$(FEOS2SDK)/lib
endif
LIBDIRS := $(foreach lib,$(CONF_USERLIBS),$(FEOSUSERLIB)/$(lib))
LIBDIRS += $(CONF_LIBDIRS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export PATH := $(FEOSBIN):$(PATH)
export TARGETNAME := $(TARGET)
export TOPDIR := $(CURDIR)

ifneq ($(CONF_TARGET),staticlib)
	export OUTPUT := $(CURDIR)/$(TARGET)
	export ELFFILE := $(CURDIR)/$(BUILD)/$(TARGET).elf
	ifeq ($(CONF_EXPLIB),1)
		export EXPFILE := $(OUTPUT).exp
		export LIBFILE := $(CURDIR)/lib/lib$(TARGET).a
	else
		export EXPFILE := /dev/null
	endif
else
	export OUTPUT := $(CURDIR)/lib/lib$(TARGET)
endif

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
DEFFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.def)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------

ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
	ifneq ($(CONF_KMOD),1)
	ifneq ($(CONF_NOCXXLIB),1)
		export CXXLIB := -lfeoscxx
	endif
	endif
endif

export OFILES   := $(addsuffix .o,$(BINFILES)) \
                   $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) $(STDLIBDIR)

export IMPFILES := $(DEFFILES:.def=.imp)

THIS_MAKEFILE ?= Makefile

.PHONY: all clean

#---------------------------------------------------------------------------------

all: $(CONF_PREREQUISITES)
	@mkdir -p $(BUILD)/imps
ifeq ($(CONF_TARGET),staticlib)
	@mkdir -p lib
endif
ifeq ($(CONF_EXPLIB),1)
	@mkdir -p lib
endif
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/$(THIS_MAKEFILE) __RECURSIVE__=1

#---------------------------------------------------------------------------------
clean:
	@echo Cleaning...
ifneq ($(CONF_TARGET),kernel)
ifneq ($(CONF_TARGET),staticlib)
	@rm -fr $(BUILD) $(TARGET).fxe $(TARGET).dbg $(TARGET).exp $(LIBFILE) $(CONF_EXTRACLEAN)
else
	@rm -fr $(BUILD) lib/lib$(TARGET).a $(CONF_EXTRACLEAN)
endif
	@rmdir --ignore-fail-on-non-empty lib
else
	@rm -fr $(BUILD) $(TARGET).img $(TARGET).dbg $(CONF_EXTRACLEAN)
endif

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main target(s)
#---------------------------------------------------------------------------------

ifneq ($(CONF_TARGET),staticlib)

ifneq ($(CONF_TARGET),kernel)

#---------------------------------------------------------------------------------
$(OUTPUT).fxe: $(ELFFILE)
	@echo $(TARGETNAME) MODULE > $(EXPFILE)
	@fxetool $< $@ $(FXEPLAT) >> $(EXPFILE)
ifeq ($(CONF_EXPLIB),1)
	@exp2lib $(EXPFILE) $(LIBFILE) .
endif
	@echo Built: $(notdir $@)

else

#---------------------------------------------------------------------------------
$(OUTPUT).img: $(ELFFILE)
	@$(OBJCOPY) -O binary $< $@
	@echo Built: $(notdir $@)

endif

#---------------------------------------------------------------------------------
$(ELFFILE): $(OFILES)
	@echo
	@echo Linking...
	@$(LD) $(LDFLAGS) $(OFILES) $(wildcard imps/*.imp.o) $(LIBPATHS) $(LIBS) -o $@
	@$(OBJCOPY) --only-keep-debug $@ $(OUTPUT).dbg 2> /dev/null
	@$(NM) -CSn $@ > $(TARGETNAME).lst
	@$(STRIP) -g $@

else

$(OUTPUT).a: $(OFILES)

#---------------------------------------------------------------------------------
%.a:
	@echo
	@echo Archiving...
	@rm -f $@
	@$(AR) -rc $@ $^ $(wildcard imps/*.imp.o)
	@echo Built: $(notdir $@)

endif

$(OFILES): $(IMPFILES)

#---------------------------------------------------------------------------------
%.arm.o: %.arm.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(ARMARCH) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.arm.o: %.arm.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(ARMARCH) $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.thumb.o: %.thumb.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(THUMBARCH) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.thumb.o: %.thumb.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(THUMBARCH) $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(DEFARCH) $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(DEFARCH) $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.o: %.s
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
%.imp: %.def
	@echo $(notdir $<)
	@exp2lib $< :no: imps
	@touch $@

#---------------------------------------------------------------------------------
# canned command sequence for binary data
#---------------------------------------------------------------------------------
define bin2o
	feosbin2s ../$(DATA)/$(shell basename $<) `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)` | $(AS) -o $(@)
	echo "extern const unsigned char" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" > `(echo $(<F) | tr . _)`.h
	echo "extern const unsigned char" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" >> `(echo $(<F) | tr . _)`.h
	echo "#define" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_size" `wc -c $< | awk '{print $$1}'` >> `(echo $(<F) | tr . _)`.h
endef

#---------------------------------------------------------------------------------
%.bin.o: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
