#include "common.h"

struct tag_processInfo
{
	int refCount;
	int asid; // only lower 8 bits matter
	const u32* svcTable; // vectors.s depends on this field being at this offset
	vu32* vmTable;
	semaphore_t vmMutex;
};

// Private functions
processInfo* PsInit(void);
void PsCtxSwitch(processInfo* p);
