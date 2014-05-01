#include "common.h"

u32 g_totalMem, g_usableMem, g_physBase;
word_t g_defModUsageRefCnt;
void* g_initRd;
u32 g_initRdSize;

schedulerInfo g_schedInfo;
