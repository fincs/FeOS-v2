#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

// Data structures

typedef struct
{
	size_t size;
	void** data;
} DynArray;

static inline void DynArray_Init(DynArray* a)
{
	a->size = 0;
	a->data = nullptr;
}

static inline void DynArray_Free(DynArray* a)
{
	if (a->data)
	{
		a->size = 0;
		free(a->data);
	}
}

static inline void* DynArray_Get(DynArray* a, size_t pos)
{
	return a->data[pos];
}

bool DynArray_Set(DynArray* a, size_t pos, void* data);
