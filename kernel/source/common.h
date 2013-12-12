#pragma once
#include "types.h"

#define memcpy __builtin_memcpy
#define memset __builtin_memset
#define malloc __builtin_malloc
#define calloc __builtin_calloc
#define realloc __builtin_realloc
#define free __builtin_free

void* memalign(u32 alignment, u32 size);
void* realloc_in_place(void* ptr, u32 size);
int malloc_trim(u32 pad);
u32 malloc_usable_size(void* ptr);

#include FEOS_PLATINCLUDE
#include "intrinsics.h"
#include "memory.h"
#include "vspace.h"
#include "irq.h"
#include "timer.h"
#include "process.h"
#include "thread.h"

extern u32** g_SyscallTablePtr;
