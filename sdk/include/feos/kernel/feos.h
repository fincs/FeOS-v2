#pragma once
#define FEOS_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intrinsics.h"
#include "../errorcodes.h"
#include FEOS_PLATINCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include "irq.h"
#include "timer.h"
#include "kio.h"
#include "vspace.h"
#include "process.h"
#include "thread.h"
#include "memory.h"
#include "data.h"

#ifdef __cplusplus
}
#endif
