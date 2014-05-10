
include $(FEOSMK)/vars.mk

ifeq ($(FEOSPLAT),3ds)
	CPUARCH := -mcpu=mpcore -mtune=mpcore
	FXEPLAT := 0x3D5
else ifeq ($(FEOSPLAT),rpi)
	CPUARCH := -mcpu=arm1176jzf-s -mtune=arm1176jzf-s
	FXEPLAT := 0x314
else ifeq ($(FEOSPLAT),qemu)
	CPUARCH := -mcpu=arm1176jzf-s -mtune=arm1176jzf-s
	FXEPLAT := 0x001
else
	FEOSPLAT :=
	CPUARCH := -march=armv6
	FXEPLAT := 0x000
endif

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
	CRTSUFFIX   := k
	ENTRYPOINT  := __FXE_KModEntry
endif

ifeq ($(ENTRYPOINT),)
	ENTRYPOINT := __FXE_Entry
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

ARCH := $(CPUARCH) -mfpu=vfp -mfloat-abi=hard

# FeOS base defines
DEFINES := -DFEOS
ifeq ($(CONF_KMOD),1)
	DEFINES += -DFEOS_KERNEL
endif
ifneq ($(FEOSPLAT),)
	DEFINES += -DFEOS_$(FEOSPLAT) -DFEOS_PLAT=\"$(FEOSPLAT)\" -DFEOS_PLATINCLUDE=\"$(FEOSPLAT)/platform.h\"
endif

# Horrid hack to get rid of format warnings
DEFINES += -U __INT32_TYPE__ -D__INT32_TYPE__=int -U __UINT32_TYPE__ -D__UINT32_TYPE__="unsigned int"

# Debugging defines
ifneq ($(CONF_DEBUG),1)
	COPTFLAG :=	-O2
	DEFINES  += -DNDEBUG
else
	COPTFLAG := -O0
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
ifeq ($(strip $(DEFARCH)),)
	DEFARCH   := $(ARMARCH)
endif

CFLAGS := -g -Wall $(COPTFLAG) -funwind-tables -save-temps -nostdinc -fvisibility=hidden\
          -fomit-frame-pointer \
          -ffast-math \
		  -fshort-wchar \
          -mthumb-interwork \
          $(ARCH) $(DEFINES) $(INCLUDEC) $(CONF_CFLAGS)

CXXFLAGS := $(CFLAGS) $(INCLUDECXX) -fno-rtti -nostdinc++ -std=gnu++11 -fvisibility-inlines-hidden \
            -Wno-delete-non-virtual-dtor $(CONF_CXXFLAGS)

ASFLAGS  := -g $(ARCH) $(DEFINES) $(INCLUDE)
LDFLAGS  := -nostartfiles -nostdlib -T $(FEOSBIN)/fxe.ld $(ARCH) \
            -Wl,-d,-q,$(GCSECTIONS)--use-blx,-Map,$(TARGETNAME).map,--defsym=__modulebase=$(MODULEBASE),-e,$(ENTRYPOINT) \
            $(CONF_LDFLAGS)

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
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export PATH := $(FEOSBIN):$(PATH)
export TARGETNAME := $(TARGET)

ifneq ($(strip $(CONF_TARGET)),staticlib)
	export OUTPUT := $(CURDIR)/$(TARGET)
	export ELFFILE := $(CURDIR)/$(BUILD)/$(TARGET).elf
	ifeq ($(CONF_EXPLIB),1)
		export EXPFILE := $(OUTPUT).exp
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
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifneq ($(CONF_FSDIR),)
	export FSDIR := $(CURDIR)/$(CONF_FSDIR)
endif

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

ifeq ($(strip $(THIS_MAKEFILE)),)
	THIS_MAKEFILE := Makefile
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD): $(CONF_PREREQUISITES)
	@[ -d $@ ] || mkdir -p $@
ifeq ($(strip $(CONF_TARGET)),staticlib)
	@mkdir -p lib
endif
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/$(THIS_MAKEFILE)

#---------------------------------------------------------------------------------
clean:
	@echo Cleaning...
ifneq ($(strip $(CONF_TARGET)),staticlib)
	@rm -fr $(BUILD) $(TARGET).fxe $(TARGET).dbg $(TARGET).exp lib/lib$(TARGET).a $(CONF_EXTRACLEAN)
else
	@rm -fr $(BUILD) lib/lib$(TARGET).a $(CONF_EXTRACLEAN)
endif

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main target(s)
#---------------------------------------------------------------------------------

ifneq ($(strip $(CONF_TARGET)),staticlib)

#---------------------------------------------------------------------------------
$(OUTPUT).fxe: $(ELFFILE)
	@echo $(TARGETNAME) MODULE > $(EXPFILE)
	@fxetool $< $@ $(FXEPLAT) >> $(EXPFILE)
	@echo Built: $(notdir $@)

#---------------------------------------------------------------------------------
$(ELFFILE): $(OFILES)
	@echo
	@echo Linking...
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
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
	@$(AR) -rc $@ $^
	@echo Built: $(notdir $@)

endif

#---------------------------------------------------------------------------------
%.arm.o: %.arm.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(ARMARCH) $(CFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.arm.o: %.arm.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(ARMARCH) $(CXXFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.thumb.o: %.thumb.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(THUMBARCH) $(CFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.thumb.o: %.thumb.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(THUMBARCH) $(CXXFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(DEFARCH) $(CFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(DEFARCH) $(CXXFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.s
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

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
