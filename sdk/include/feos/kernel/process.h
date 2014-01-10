#pragma once
#include "feos.h"

typedef struct tag_processInfo processInfo;

processInfo* PsCreate(void);
processInfo* PsFindByASID(int asid);
