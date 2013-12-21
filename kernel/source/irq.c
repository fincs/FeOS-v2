#include "common.h"

static isrEntry isrTable[MAX_IRQCTRL][MAX_ISR];
static u32 isrFlags[MAX_IRQCTRL];
static bool bIrqsWereEnabled;

static void __irqSet(int ctrlId, u32 mask, isr_t isr)
{
	isrEntry* isrs = isrTable[ctrlId];

	int i;
	for (i = 0; i < MAX_ISR; i ++)
		if (!isrs[i].mask || isrs[i].mask == mask)
			break;

	if (i == MAX_ISR)
		return;

	isrs[i].mask = mask;
	isrs[i].isr = isr;
}

void irqSuspend(void)
{
	bIrqsWereEnabled = !(CpuGetCPSR() & BIT(7));
	CpuIrqDisable();
}

void irqRestore(void)
{
	if (bIrqsWereEnabled)
		CpuIrqEnable();
}

void irqSet(int ctrlId, u32 mask, isr_t isr)
{
	if (!mask || ctrlId >= MAX_IRQCTRL)
		return;

	irqSuspend();
	__irqSet(ctrlId, mask, isr);
	irqRestore();
}

u32 irqFlags(int ctrlId)
{
	return AtomicSwap(isrFlags + ctrlId, 0);
}

void irqDispatch(int ctrlId, u32 mask, u32* regs)
{
	isrEntry* isrs = isrTable[ctrlId];
	isrFlags[ctrlId] |= mask;

	int i;
	for (i = 0; i < MAX_ISR; i ++)
	{
		if (!isrs[i].mask)
			break;
		if (isrs[i].mask & mask)
		{
			if (isrs[i].isr)
				isrs[i].isr(regs);
			break;
		}
	}
}