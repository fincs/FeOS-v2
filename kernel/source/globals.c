#include "common.h"

u32** g_SyscallTablePtr;
u32 g_totalMem, g_usableMem, g_physBase;
vu32* g_curPageTable;

schedulerInfo g_schedInfo;
