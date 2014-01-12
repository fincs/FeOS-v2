#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

typedef struct tag_processInfo processInfo;

processInfo* PsCreate(void);
processInfo* PsFindByASID(int asid);
