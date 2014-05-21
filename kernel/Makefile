
ifeq ($(strip $(FEOS2SDK)),)
$(error "Please set FEOS2SDK in your environment. export FEOSSDK=<path to>FeOS/sdk")
endif

FEOSMK = $(FEOS2SDK)/mk

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
ifeq ($(CONF_DEBUG),)
TARGET   := FeOS.$(FEOSPLAT)
BUILD    := build/$(FEOSPLAT).Release
else
TARGET   := FeOSd.$(FEOSPLAT)
BUILD    := build/$(FEOSPLAT).Debug
endif
SOURCES  := source source/$(FEOSPLAT) drivers drivers/$(FEOSPLAT) source/stdio
INCLUDES :=
DATA     := data data/$(FEOSPLAT)

include $(FEOSMK)/kernel.mk
