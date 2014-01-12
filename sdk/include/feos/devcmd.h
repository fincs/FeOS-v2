#pragma once
#include "types.h"
#include "errorcodes.h"

#define DEVCMD_MAKE(_iface,_id) (((_iface)<<16)|(_id))
#define BLKDEV_SECTSIZE 512

// For use by non-standard device types
#define DEVCMD_MAKEUSR(_iface,_id) DEVCMD_MAKE(1024+(_iface),(_id))

// Generic device commands
enum
{
	// Avoid using the first 256 command IDs to make it harder to mess up with garbage

	// GetMem: used by devices that require the user to make direct access to a buffer
	// Userland drivers typically marshal this call by mapping the buffer to a portion
	//   of user address space and returning that address instead
	DevCmd_GetMem = DEVCMD_MAKE(0,256),
};

// Stream device commands
enum
{
	StrmCmd_GetStat = DEVCMD_MAKE(1,0),
	StrmCmd_Read,
	StrmCmd_Write,
	StrmCmd_Seek,
	StrmCmd_Truncate,
	StrmCmd_Sync,
};

typedef struct
{
	size_t structSize;
	u32 flags;
	u64 size; // meaningful only if the stream is Seekable
} StrmDevStat;

enum
{
	StrmDev_Seekable = BIT(0),
	StrmDev_Writable = BIT(1),
	StrmDev_Readable = BIT(2),
};

enum
{
	StrmDevSeek_Set = 0,
	StrmDevSeek_Cur,
	StrmDevSeek_End,
};

// Block device commands
enum
{
	BlkCmd_GetInfo = DEVCMD_MAKE(2,0),
	BlkCmd_IsPresent,
	BlkCmd_ReadSectors,
	BlkCmd_WriteSectors,
};

typedef struct
{
	size_t structSize;
	word_t flags;
	u64 sectCount;
} BlkDevInfo;

// Framebuffer device commands
enum
{
	FbCmd_GetInfo = DEVCMD_MAKE(3,0),
};

typedef struct
{
	size_t structSize;
	word_t width, height, stride, bitDepth;
} FbDevInfo;
