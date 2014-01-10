#include "common.h"

struct tag_processInfo
{
	int refCount;
	int asid; // only lower 8 bits matter
	vu32* vmTable;
	const u32* svcTable;
};

// Private functions
processInfo* PsInit(void);
void PsCtxSwitch(processInfo* p);
