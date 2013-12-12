#include "../common.h"

#define _TMRBASE1 0xA01e2000
#define _TMRBASE2 0xA01e3000

static inline u32 _tmr2off(int tmr)
{
	return _TMRBASE1 + ((tmr & 2) ? (_TMRBASE2-_TMRBASE1) : 0) + ((tmr & 1) ? 0x20 : 0);
}

static inline u32 _freq2cnt(int freq)
{
	return (u32)TIMER_HZ(TIMER_TICK_FREQ) / (u32)freq;
}

#define REG_TMRLOAD(x) (*(vu32*)(_tmr2off(x) + 0x0))
#define REG_TMRVAL(x) (*(vu32*)(_tmr2off(x) + 0x4))
#define REG_TMRCNT(x) (*(vu8*)(_tmr2off(x) + 0x8))
#define REG_TMRINTCLR(x) (*(vu32*)(_tmr2off(x) + 0xC))
#define REG_TMRINT(x) (*(vu32*)(_tmr2off(x) + 0x14))

static void tmrIsr0(u32*);
static void tmrIsr1(u32*);

static isr_t tmrIsrs[MAX_TIMER];
static u8 tmrHandles[MAX_TIMER];
static int tmrHandlePos;

void timerInit()
{
	int i;
	for (i = 0; i < MAX_TIMER; i ++)
	{
		REG_TMRCNT(i) = 0;
		tmrHandles[i] = i;
	}

	irqSet(0, BIT(4), tmrIsr0);
	irqSet(0, BIT(5), tmrIsr1);
	irqEnable(0, BIT(4) | BIT(5));
}

int timerStart(int freq, isr_t isr)
{
	if (tmrHandlePos == MAX_TIMER) return -1;
	int timer = tmrHandles[tmrHandlePos++];

	tmrIsrs[timer] = isr;
	REG_TMRLOAD(timer) = _freq2cnt(freq);
	REG_TMRCNT(timer) = BIT(7) | BIT(6) | BIT(5) | BIT(1);
	return timer;
}

int timerRead(int timer)
{
	return (int)REG_TMRVAL(timer);
}

void timerPause(int timer, bool bPause)
{
	int reg = REG_TMRCNT(timer);
	if (bPause)
		reg &= ~BIT(7);
	else
		reg |= BIT(7);
	REG_TMRCNT(timer) = reg;
}

void timerStop(int timer)
{
	REG_TMRCNT(timer) = 0;
	tmrHandles[--tmrHandlePos] = timer;
}

void tmrIsr0(u32* regs)
{
	if (REG_TMRINT(0) & 1)
	{
		REG_TMRINTCLR(0) = 1;
		if (tmrIsrs[0]) tmrIsrs[0](regs);
	}
	if (REG_TMRINT(1) & 1)
	{
		REG_TMRINTCLR(1) = 1;
		if (tmrIsrs[1]) tmrIsrs[1](regs);
	}
}

void tmrIsr1(u32* regs)
{
	if (REG_TMRINT(2) & 1)
	{
		REG_TMRINTCLR(2) = 1;
		if (tmrIsrs[2]) tmrIsrs[2](regs);
	}
	if (REG_TMRINT(3) & 1)
	{
		REG_TMRINTCLR(3) = 1;
		if (tmrIsrs[3]) tmrIsrs[3](regs);
	}
}
