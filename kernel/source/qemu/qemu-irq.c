#include "../common.h"

#define _PICBASE 0xA0140000
#define _SICBASE 0xA0003000

#define PIC_IRQStatus (*(vu32*)(_PICBASE + 0x0))
#define PIC_IntSelect (*(vu32*)(_PICBASE + 0xC))
#define PIC_IntEnable (*(vu32*)(_PICBASE + 0x10))
#define PIC_IntEnClear (*(vu32*)(_PICBASE + 0x14))

#define SIC_STATUS (*(vu32*)(_SICBASE + 0x0))
#define SIC_ENSET (*(vu32*)(_SICBASE + 0x8))
#define SIC_ENCLR (*(vu32*)(_SICBASE + 0xC))
#define SIC_PICENCLR (*(vu32*)(_SICBASE + 0x24))

static void isr_SIC(u32* regs);

void irqInit()
{
	PIC_IntEnClear = ~0; // disable all IRQ sources
	PIC_IntSelect = 0; // disable FIQ support
	SIC_ENCLR = ~0; // disable all SIC IRQ sources
	SIC_PICENCLR = ~0; // disable weird SIC->PIC IRQ source routing mechanism
	irqSet(0, BIT(31), isr_SIC); // set ISR for SIC IRQ
	PIC_IntEnable = BIT(31); // enable SIC PIC IRQ source

	CpuIrqEnable();
}

void irqEnable(int ctrlId, u32 mask)
{
	switch (ctrlId)
	{
		case 0: PIC_IntEnable |= mask &~ BIT(31); break;
		case 1: SIC_ENSET = mask; break;
	}
}

void irqDisable(int ctrlId, u32 mask)
{
	switch (ctrlId)
	{
		case 0: PIC_IntEnClear = mask &~ BIT(31); break;
		case 1: SIC_ENCLR = mask; break;
	}
}

void KeIrqEntry(u32* regs)
{
	u32 mask;
	while ((mask = PIC_IRQStatus))
		irqDispatch(0, mask, regs);
}

void isr_SIC(u32* regs)
{
	irqDispatch(1, SIC_STATUS, regs);
}
