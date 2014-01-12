#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

typedef struct _tag_HVSPACE* HVSPACE;

HVSPACE vspace_create(void* addr, u32 size);
void    vspace_freeAll(HVSPACE h);
HVSPACE vspace_alloc(HVSPACE h, u32 size);
void    vspace_free(HVSPACE vs);
u32     vspace_getAddr(HVSPACE vs);
u32     vspace_getPageCount(HVSPACE vs);
