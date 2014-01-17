#include "common.h"

u32 g_totalMem, g_usableMem, g_physBase;
vu32* g_curPageTable;
word_t g_defModUsageRefCnt;

schedulerInfo g_schedInfo;
