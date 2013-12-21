#include "../common.h"

#define IRQCTRL_BASE 0xA000b200

#define REG_IRQFlags ((vu32*)(IRQCTRL_BASE+0)) //[0..2], 0=base, 1=IRQ1, 2=IRQ2
#define REG_FIQCtrl (*(vu32*)(IRQCTRL_BASE+0xC))
#define REG_IRQEnable ((vu32*)(IRQCTRL_BASE+0x10)) //[0..2], 0=IRQ1, 1=IRQ2, 2=base
#define REG_IRQDisable ((vu32*)(IRQCTRL_BASE+0x1C)) //[0..2], 0=IRQ1, 1=IRQ2, 2=base

void irqInit()
{
	register int i;
	for (i = 0; i < 3; i ++)
		REG_IRQDisable[i] = ~0;
	REG_FIQCtrl = 0; // not using FIQ
	CpuIrqEnable();
}

void irqEnable(int ctrlId, u32 mask)
{
	REG_IRQEnable[ctrlId] |= mask;
}

void irqDisable(int ctrlId, u32 mask)
{
	REG_IRQDisable[ctrlId] |= mask;
}

void KeIrqEntry(u32* regs)
{
	u32 mask;
	while ((mask = REG_IRQFlags[2]))
	{
		u32 mask0 = mask & 0x7F;
		if (mask0)
		{
			irqDispatch(2, mask0, regs);
			mask &= ~mask0;
		}
		if (mask & 0x7D00) // BIT(8) or any of BIT(10..14) are active
		{
			irqDispatch(0, REG_IRQFlags[0], regs);
			mask &= ~0x7D00;
		}
		if (mask)
			irqDispatch(1, REG_IRQFlags[1], regs);
	}
}
