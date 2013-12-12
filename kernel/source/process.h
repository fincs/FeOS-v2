#include "common.h"

typedef struct tag_processInfo processInfo;

struct tag_processInfo
{
	int refCount;
	int asid; // only lower 8 bits matter
	vu32* vmTable;
	const u32* svcTable;
};

// Private functions
processInfo* PsInit(void);
processInfo* PsCreate(void);
void PsCtxSwitch(processInfo* p);

processInfo* PsFindByASID(int asid);
