#pragma once
#include "common.h"

// Atomic ops
#define AtomicIncrement(ptr) __atomic_add_fetch((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicDecrement(ptr) __atomic_sub_fetch((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicPostIncrement(ptr) __atomic_fetch_add((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicPostDecrement(ptr) __atomic_fetch_sub((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicSwap(ptr, value) __atomic_exchange_n((u32*)(ptr), (value), __ATOMIC_SEQ_CST)

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

static inline void DC_DrainWriteBuffer()
{
	asm volatile("mcr p15, 0, %[data], c7, c10, 4" :: [data] "r" (0));
}

static inline void DC_FlushAll()
{
	asm volatile("mcr p15, 0, %[data], c7, c14, 0" :: [data] "r" (0));
}

static inline void IC_InvalidateAll()
{
	asm volatile("mcr p15, 0, %[data], c7, c5, 0" :: [data] "r" (0));
}

#ifdef HAS_FAST_CACHE_RANGE_OPS

static inline void DC_FlushRange(void* addr, u32 size)
{
	asm volatile("mcrr p15, 0, %[endAddr], %[startAddr], c14" :: [startAddr] "r" ((u32)addr), [endAddr] "r" ((u32)addr + size));
}

static inline void IC_InvalidateRange(void* addr, u32 size)
{
	asm volatile("mcrr p15, 0, %[endAddr], %[startAddr], c5" :: [startAddr] "r" ((u32)addr), [endAddr] "r" ((u32)addr + size));
}

#else
#error "Not implemented"
#endif

// Other ops

static inline void CpuIdle()
{
	DC_DrainWriteBuffer(); // Apparently necessary
	asm volatile("mcr p15, 0, %[data], c7, c0, 4" :: [data] "r" (0));
}

static inline void CpuIrqEnable()
{
	asm volatile("cpsie i");
}

static inline void CpuIrqDisable()
{
	asm volatile("cpsid i");
}

static inline u32 CpuGetCPSR()
{
	u32 ret;
	asm volatile("mrs %[data], cpsr" : [data] "=r" (ret));
	return ret;
}

static inline void CpuLoopDelay(u32 count)
{
	asm volatile("1: subs %[count], %[count], #1; bne 1b" :: [count] "r" (count) : "cc");
}

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
