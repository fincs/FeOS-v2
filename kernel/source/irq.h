#pragma once
#include "common.h"

#ifndef MAX_ISR
// Max # of ISRs per IRQ controller
#define MAX_ISR 32
#endif
#ifndef MAX_IRQCTRL
// Max # of IRQ controllers
#define MAX_IRQCTRL 2
#endif

typedef void (* isr_t)(u32* regs);

typedef struct
{
	u32 mask;
	isr_t isr;
} isrEntry;

void irqInit(); // implemented in HAL

void irqSet(int ctrlId, u32 mask, isr_t isr);
void irqEnable(int ctrlId, u32 mask); // implemented by HAL
void irqDisable(int ctrlId, u32 mask); // implemented by HAL
u32 irqFlags(int ctrlId);

#define irqClear(ctrlId, mask) irqSet((ctrlId), (mask), nullptr)

void irqSuspend(void); // implemented by HAL
void irqRestore(void); // implemented by HAL

void KeIrqEntry(u32* regs); // implemented by HAL

// Should only be called by HAL code
void irqDispatch(int ctrlId, u32 mask, u32* regs);
