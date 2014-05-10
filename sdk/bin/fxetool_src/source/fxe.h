#pragma once
#include "types.h"

typedef word_t u32;
typedef hword_t u16;

#define FXE_MAGIC 0x30455846

typedef struct
{
	u32 magic; // 'FXE0'
	u32 flags; // see FXEFlags below
	u16 osMajVer, osMinVer;
	u32 entrypoint;
} FXEHeader;

enum
{
	FXEFlags_Platform_Mask = 0xFFF,
	FXEFlags_Platform_Generic  = 0x000, // Generic ARMv6 machine (i.e. no specific platform)
	FXEFlags_Platform_Emulator = 0x001, // Generic emulator (i.e. QEMU)
	FXEFlags_Platform_RPi = 0x314,      // Raspberry RPi (ARM1176JZF-S)
	FXEFlags_Platform_3DS = 0x3D5,      // Nintendo 3DS (ARM11MPCore)

	FXEFlags_SectCountMask = 0xFF000,
	FXEFlags_SectCountShift = 12,
        
	FXEFlags_HasRelocs  = BIT(20), // if set = shared lib, if not set = executable loaded at 0x10000
	FXEFlags_HasExidx   = BIT(21), // exception index table used for unwinding and exception recovery
	FXEFlags_HasImports = BIT(22), // import table
	FXEFlags_HasFixups  = BIT(23), // data import address fixups
	FXEFlags_HasExports = BIT(24), // export table
};

typedef struct
{
	char name[6]; // null-padded
	u16 flags;    // see FXESectFlags
	u32 sectSize; // Bits 0 and 1 must be zero
	u32 loadSize; // As above. loadSize is always <= sectSize, uninitialized part filled with zeroes (BSS)
} FXESection;

enum
{
	FXESectFlag_Executable = BIT(0), // sets XN (eXecute-Never) bit to zero
	FXESectFlag_Writable   = BIT(1), // allows write operations
	FXESectFlag_Shared     = BIT(2), // shared amongst processes
};

typedef struct
{
	u16 skip;
	u16 patch;
} FXEReloc;

typedef struct
{
	u32 tableOffset;
	u32 entryCount;
} FXEExidx;

typedef struct
{
	u32 count;
	u32 size;
} FXESymHdr;

typedef struct
{
	u32 nameOffset;
	u32 symOffset;
} FXESymEntry;

typedef struct
{
	u32 sourceAddr;
	u32 targetAddr;
} FXEFixup;
