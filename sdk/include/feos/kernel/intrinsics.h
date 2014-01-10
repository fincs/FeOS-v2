#pragma once
#include "../types.h"

// Atomic operations
#define AtomicIncrement(ptr) __atomic_add_fetch((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicDecrement(ptr) __atomic_sub_fetch((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicPostIncrement(ptr) __atomic_fetch_add((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicPostDecrement(ptr) __atomic_fetch_sub((u32*)(ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicSwap(ptr, value) __atomic_exchange_n((u32*)(ptr), (value), __ATOMIC_SEQ_CST)

// Data Syncronization Barrier, AKA DC_DrainWriteBuffer on FeOS/DS
static inline void CpuSyncBarrier()
{
	asm volatile("mcr p15, 0, %[data], c7, c10, 4" :: [data] "r" (0));
}

// Data Memory Barrier
static inline void CpuMemBarrier()
{
	asm volatile("mcr p15, 0, %[data], c7, c10, 5" :: [data] "r" (0));
}

// Data Cache Flush
static inline void DC_FlushAll()
{
	asm volatile("mcr p15, 0, %[data], c7, c14, 0" :: [data] "r" (0));
}

// Instruction Cache Invalidate
static inline void IC_InvalidateAll()
{
	asm volatile("mcr p15, 0, %[data], c7, c5, 0" :: [data] "r" (0));
}

#ifdef HAS_FAST_CACHE_RANGE_OPS

// Data Cache Flush Range
static inline void DC_FlushRange(void* addr, u32 size)
{
	asm volatile("mcrr p15, 0, %[endAddr], %[startAddr], c14" :: [startAddr] "r" ((u32)addr), [endAddr] "r" ((u32)addr + size - 1));
}

// Instruction Cache Invalidate Range
static inline void IC_InvalidateRange(void* addr, u32 size)
{
	asm volatile("mcrr p15, 0, %[endAddr], %[startAddr], c5" :: [startAddr] "r" ((u32)addr), [endAddr] "r" ((u32)addr + size - 1));
}

#else
#error "Not implemented"
#endif

// CPU Idle (Wait for Next IRQ)
static inline void CpuIdle()
{
	CpuSyncBarrier(); // Apparently necessary
	asm volatile("mcr p15, 0, %[data], c7, c0, 4" :: [data] "r" (0));
}

// Interrupt Enable
static inline void CpuIrqEnable()
{
	asm volatile("cpsie i");
}

// Interrupt Disable
static inline void CpuIrqDisable()
{
	asm volatile("cpsid i");
}

// Get CPSR
static inline u32 CpuGetCPSR()
{
	u32 ret;
	asm volatile("mrs %[data], cpsr" : [data] "=r" (ret));
	return ret;
}

// Delay by Decrement Loop
static inline void CpuLoopDelay(u32 count)
{
	asm volatile("1: subs %[count], %[count], #1; bne 1b" :: [count] "r" (count) : "cc");
}

// Atomic version of: if(!*ptr) return true; else { --*ptr; return false; }
static inline bool AtomicDecrementCompareZero(vu32* ptr)
{
	u32 ret = 0, temp, temp2;
	asm volatile
	(
		"mcr p15, 0, r0, c7, c10, 5\n\t"
		"1: ldrex %[temp], [%[ptr]]\n\t"
		"cmp %[temp], #0\n\t"
		"moveq %[ret], #1\n\t"
		"beq 2f\n\t"
		"sub %[temp], #1\n\t"
		"strex %[temp2], %[temp], [%[ptr]]\n\t"
		"cmp %[temp2], #0\n\t"
		"bne 1b\n\t"
		"2: mcr p15, 0, r0, c7, c10, 5"
		:
		[ret] "+r" (ret),
		[ptr] "+r" (ptr),
		[temp] "=r" (temp),
		[temp2] "=r" (temp2)
		:: "cc"
	);
	return (bool)ret;
}
