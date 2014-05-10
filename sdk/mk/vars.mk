#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

FEOSBIN     = $(FEOS2SDK)/bin
FEOSUSERLIB = $(FEOS2SDK)/userlib

#---------------------------------------------------------------------------------
# devkitARM configuration
#---------------------------------------------------------------------------------
include $(DEVKITARM)/base_tools
