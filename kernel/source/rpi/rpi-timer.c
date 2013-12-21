#include "../common.h"

#define TMRCTRL_BASE 0xA0003000

#define REG_TMRCNT (*(vu32*)(TMRCTRL_BASE + 0x0))
#define REG_TMRVALLO (*(vu32*)(TMRCTRL_BASE + 0x4))
#define REG_TMRVALHI (*(vu32*)(TMRCTRL_BASE + 0x8))
#define REG_TMRCMP ((vu32*)(TMRCTRL_BASE + 0xC)) // 0..3

static inline u32 _freq2cnt(int freq)
{
	return (u32)TIMER_HZ(TIMER_TICK_FREQ) / (u32)freq;
}

static isr_t tmrIsrs[MAX_TIMER];
static u8 tmrHandles[MAX_TIMER];
static u32 tmrPeriods[MAX_TIMER];
static int tmrHandlePos;

#define EMIT_ISR(n) \
static void tmrIsr##n(u32* regs) \
{ \
	REG_TMRCNT |= BIT(n); \
	REG_TMRCMP[n] += tmrPeriods[n]; \
	if (tmrIsrs[n]) \
		tmrIsrs[n](regs); \
}

EMIT_ISR(0);
EMIT_ISR(1);
EMIT_ISR(2);
EMIT_ISR(3);

void timerInit()
{
	register int i;
	REG_TMRCNT = 0xF;
	for (i = 0; i < MAX_TIMER; i ++)
	{
		REG_TMRCMP[i] = 0;
		tmrHandles[i] = i;
	}

	irqSet(0, IRQ0_TIMER(0), tmrIsr0);
	irqSet(0, IRQ0_TIMER(1), tmrIsr1);
	irqSet(0, IRQ0_TIMER(2), tmrIsr2);
	irqSet(0, IRQ0_TIMER(3), tmrIsr3);
}

int timerStart(int freq, isr_t isr)
{
	if (tmrHandlePos == MAX_TIMER) return -1;
	int timer = tmrHandles[AtomicPostIncrement(&tmrHandlePos)];

	tmrIsrs[timer] = isr;
	tmrPeriods[timer] = _freq2cnt(freq);
	REG_TMRCMP[timer] = REG_TMRVALLO + tmrPeriods[timer];
	irqEnable(0, IRQ0_TIMER(timer));
	return timer;
}

int timerRead(int timer)
{
	return REG_TMRVALLO + tmrPeriods[timer] - REG_TMRCMP[timer];
}

void timerPause(int timer, bool bPause)
{
	//TODO: implement
}

void timerStop(int timer)
{
	irqDisable(0, IRQ0_TIMER(timer));
	tmrHandles[AtomicDecrement(&tmrHandlePos)] = timer;
}
