#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

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

void irqSet(int ctrlId, u32 mask, isr_t isr);
void irqEnable(int ctrlId, u32 mask); // implemented by HAL
void irqDisable(int ctrlId, u32 mask); // implemented by HAL
u32 irqFlags(int ctrlId);

#define irqClear(ctrlId, mask) irqSet((ctrlId), (mask), nullptr)

static inline bool irqSuspend(void)
{
	bool ret = !(CpuGetCPSR() & BIT(7));
	CpuIrqDisable();
	return ret;
}

static inline void irqRestore(bool state)
{
	if (state)
		CpuIrqEnable();
}
