#include "common.h"

static u8 asidStack[256];
static int asidStackPos = 0;

static void asidStackInit()
{
	int i;
	for (i = 0; i < 256; i ++)
		asidStack[i] = i;
}

// TODO: thread safety
static int asidAlloc()
{
	if (asidStackPos == 256)
		return -1;
	return asidStack[AtomicPostIncrement(&asidStackPos)];
}

static void asidFree(int x)
{
	asidStack[AtomicDecrement(&asidStackPos)] = x;
}

static processInfo* psTable[256];

processInfo* PsCreate(void)
{
	int asid = asidAlloc();
	if (asid < 0)
		return nullptr;

	processInfo* p = (processInfo*) malloc(sizeof(processInfo));
	if (!p)
	{
		asidFree(asid);
		return nullptr;
	}

	p->refCount = 1;
	p->asid = asid;
	p->svcTable = nullptr;
	p->vmTable = (vu32*)safe_phys2virt(CpuGetLowerPT() &~ 0x1FFF);
	SemaphoreInit(&p->vmMutex, 1);
	psTable[asid] = p;
	return p;
}

processInfo* PsInit(void)
{
	asidStackInit();
	return PsCreate();
}

void PsCtxSwitch(processInfo* p)
{
	// Caller is responsible for disabling IRQs before calling this function
#ifndef HAS_PIPT_CACHE
	// This is necessary because the cache deals somehow with virtual addresses
	// (its indexing and/or tagging) and we are changing the virtual address map!
	DC_FlushAll();
	IC_InvalidateAll();
#endif
	CpuSetLowerPT(MemTranslateAddr((void*)p->vmTable));
	CpuSyncBarrier(); // ARM docs say this is needed
	CpuSetASID(p->asid);
	CpuFlushBtac();
}
