#pragma once
#include "common.h"

void irqInit(); // implemented in HAL

void KeIrqEntry(u32* regs); // implemented by HAL

// Should only be called by HAL code
void irqDispatch(int ctrlId, u32 mask, u32* regs);
