#pragma once
#include "mailbox.h"

#define MKVCTAG(type,idx) (((type)<<16) | (idx))

enum
{
	VCTag_GetFirmwareVer = MKVCTAG(0,1),
	VCTag_GetBoardModel = MKVCTAG(1,1),
	VCTag_GetBoardRev,
	VCTag_GetMACAddr,
	VCTag_GetSerial,
	VCTag_GetARMMem,
	VCTag_GetVCMem,
	VCTag_GetClocks,

	VCTag_GetCmdLine = MKVCTAG(5,1),
	
	VCTag_GetDMAChannels = MKVCTAG(6,1),

	VCTag_GetPowerState = MKVCTAG(2,1),
	VCTag_GetDevTiming,
	VCTag_SetPowerState = VCTag_GetPowerState | BIT(15),

	VCTag_GetClkState = MKVCTAG(3,1),
	VCTag_GetClkRate,
	VCTag_GetVoltage,
	VCTag_GetMaxClkRate,
	VCTag_GetMaxVoltage,
	VCTag_GetTemperature,
	VCTag_GetMinClkRate,
	VCTag_GetMinVoltage,
	VCTag_GetTurbo,
	VCTag_GetMaxTemperature,
	VCTag_AllocVCMem = MKVCTAG(3,0xc),
	VCTag_LockVCMem,
	VCTag_UnlockVCMem,
	VCTag_FreeVCMem,
	VCTag_CallVCFunc,
	VCTag_GetEDIDBlock = MKVCTAG(3,0x20),
	VCTag_SetClkState = VCTag_GetClkState | BIT(15),
	VCTag_SetClkRate  = VCTag_GetClkRate  | BIT(15),
	VCTag_SetVoltage  = VCTag_GetVoltage  | BIT(15),

	VCTag_AllocFB = MKVCTAG(4,1),
	VCTag_BlankScreen,
	VCTag_FB_GetPhysSize,
	VCTag_FB_GetVirtSize,
	VCTag_FB_GetBPP,
	VCTag_FB_GetPixelOrder,
	VCTag_FB_GetAlphaMode,
	VCTag_FB_GetStride,
	VCTag_FB_GetVirtOffset,
	VCTag_FB_GetOverscan,
	VCTag_FB_GetPalette,
	VCTag_FB_TestPhysSize   = VCTag_FB_GetPhysSize   | BIT(14),
	VCTag_FB_TestVirtSize   = VCTag_FB_GetVirtSize   | BIT(14),
	VCTag_FB_TestBPP        = VCTag_FB_GetBPP        | BIT(14),
	VCTag_FB_TestPixelOrder = VCTag_FB_GetPixelOrder | BIT(14),
	VCTag_FB_TestAlphaMode  = VCTag_FB_GetAlphaMode  | BIT(14),
	VCTag_FB_TestVirtOffset = VCTag_FB_GetVirtOffset | BIT(14),
	VCTag_FB_TestOverscan   = VCTag_FB_GetOverscan   | BIT(14),
	VCTag_FB_TestPalette    = VCTag_FB_GetPalette    | BIT(14),
	VCTag_FreeFB           = VCTag_AllocFB          | BIT(15),
	VCTag_FB_SetPhysSize   = VCTag_FB_GetPhysSize   | BIT(15),
	VCTag_FB_SetVirtSize   = VCTag_FB_GetVirtSize   | BIT(15),
	VCTag_FB_SetBPP        = VCTag_FB_GetBPP        | BIT(15),
	VCTag_FB_SetPixelOrder = VCTag_FB_GetPixelOrder | BIT(15),
	VCTag_FB_SetAlphaMode  = VCTag_FB_GetAlphaMode  | BIT(15),
	VCTag_FB_SetVirtOffset = VCTag_FB_GetVirtOffset | BIT(15),
	VCTag_FB_SetOverscan   = VCTag_FB_GetOverscan   | BIT(15),
	VCTag_FB_SetPalette    = VCTag_FB_GetPalette    | BIT(15),
};

typedef struct
{
	vu32* buffer;
	u32 size;
	u32 pos;
} VCPROPS;

bool vcPropsAlloc(VCPROPS* pProps, u32 bufSize);
vu32* vcPropsAddTag(VCPROPS* pProps, u32 tagId, u32 requestSize, u32 responseSize);
bool vcPropsExecute(VCPROPS* pProps);

static inline u32 vcPropsGetTagSize(vu32* tag)
{
	return tag[-1] &~ BIT(31);
}

static inline void vcPropsReset(VCPROPS* pProps)
{
	pProps->pos = 2;
	pProps->buffer[1] = 0;
}

static inline void vcPropsFree(VCPROPS* pProps)
{
	free((void*)pProps->buffer);
}
