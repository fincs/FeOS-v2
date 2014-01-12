#pragma once
#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dynamic memory handling
void* malloc(size_t);
void* memalign(size_t, size_t);
void* calloc(size_t, size_t);
void free(void*);
void* realloc(void*, size_t);
void* realloc_in_place(void* ptr, u32 size);
int malloc_trim(u32 pad);
u32 malloc_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif
