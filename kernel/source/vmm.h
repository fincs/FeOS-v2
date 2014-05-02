#pragma once
#include "common.h"

typedef struct
{
	semaphore_t* mutex;  // VMM mutex
	vu32* l1_table;      // L1 table
	vu32* l2_table;      // Current L2 table
	int curIndex;        // Current L1 table index (-1 if not initialized)
	pageinfo_t* curPage; // Page that holds the current L2 table
} vmminfo_t;

// Internal functions for opening a process VM address space for potential modification
bool VmmOpen(vmminfo_t* vmInfo, processInfo* proc, void* addr);
vu32* VmmGetEntry(vmminfo_t* vmInfo, void* addr, bool bCreate);
void VmmClose(vmminfo_t* vmInfo);

static inline void VmmPageIncrRef(vmminfo_t* vmInfo)
{
	MemPageIncrRef(vmInfo->curPage);
}

static inline void VmmPageDecrRef(vmminfo_t* vmInfo)
{
	MemPageDecrRef(vmInfo->curPage);
}
