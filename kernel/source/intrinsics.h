#pragma once
#include "common.h"

static inline void CpuSetASID(int asid)
{
	asm volatile("mcr p15, 0, %[data], c13, c0, 1" :: [data] "r" (asid));
}

static inline int CpuGetASID()
{
	int ret;
	asm volatile("mrc p15, 0, %[data], c13, c0, 1" : [data] "=r" (ret));
	return ret;
}

static inline void CpuSetLowerPT(u32 setting)
{
	asm volatile("mcr p15, 0, %[data], c2, c0, 0" :: [data] "r" (setting));
}

static inline u32 CpuGetLowerPT()
{
	u32 ret;
	asm volatile("mrc p15, 0, %[data], c2, c0, 0" : [data] "=r" (ret));
	return ret;
}

static inline void CpuFlushTlb()
{
	asm volatile("mcr p15, 0, %[data], c8, c7, 0" :: [data] "r" (0));
}

static inline void CpuFlushTlbByASID(int asid)
{
	asm volatile("mcr p15, 0, %[data], c8, c7, 2" :: [data] "r" (asid&0xFF));
}

static inline void CpuFlushTlbByAddrAndASID(void* addr, int asid)
{
	asm volatile("mcr p15, 0, %[data], c8, c7, 1" :: [data] "r" (((u32)addr &~ 0xFFF) | (asid & 0xFF)));
}

static inline void CpuFlushTlbByAddr(void* addr)
{
	// TODO: use proper cp15 instruction for CPUs which support it
	CpuFlushTlbByAddrAndASID(addr, CpuGetASID());
}

static inline void CpuFlushBtac()
{
	asm volatile("mcr p15, 0, %[data], c7, c5, 6" :: [data] "r" (0));
}

// Cache ops

static inline void CpuEnableCaches()
{
	u32 ret;
	asm volatile("mrc p15, 0, %[data], c1, c0, 0" : [data] "=r" (ret));
	ret |= BIT(2) | BIT(12); // enable data & instruction cache respectively
	asm volatile("mcr p15, 0, %[data], c1, c0, 0" :: [data] "r" (ret));
}

// Other ops


static inline void CpuRestoreModeRegs(int mode, u32 sp, u32 lr)
{
	u32 cpsr, spsr;
	if (mode == 0)
		mode = 0xF; // avoid disaster

	asm volatile
	(
		"mrs %[spsr], cpsr\n"
		"bic %[cpsr], %[spsr], #0xF\n"
		"orr %[cpsr], %[mode]\n"
		"msr cpsr, %[cpsr]\n"
		"mov sp, %[sp]\n"
		"mov lr, %[lr]\n"
		"msr cpsr, %[spsr]\n"
		:
		[spsr] "=r" (spsr),
		[cpsr] "=r" (cpsr),
		[mode] "+r" (mode),
		[sp] "+r" (sp),
		[lr] "+r" (lr)
	);
}

static inline void CpuSaveModeRegs(int mode, u32* pSp, u32* pLr)
{
	u32 cpsr, spsr, sp, lr;
	if (mode == 0)
		mode = 0xF; // avoid disaster

	asm volatile
	(
		"mrs %[spsr], cpsr\n"
		"bic %[cpsr], %[spsr], #0xF\n"
		"orr %[cpsr], %[mode]\n"
		"msr cpsr, %[cpsr]\n"
		"mov %[sp], sp\n"
		"mov %[lr], lr\n"
		"msr cpsr, %[spsr]\n"
		:
		[spsr] "=r" (spsr),
		[cpsr] "=r" (cpsr),
		[sp] "=r" (sp),
		[lr] "=r" (lr),
		[mode] "+r" (mode)
	);
	*pSp = sp;
	*pLr = lr;
}
