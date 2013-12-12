#include "common.h"

#define REG_UART0DR (*(vu32*)0xA01f1000)

static inline void kputc(char c)
{
	REG_UART0DR = c;
}

void kputs(const char* s)
{
	for (; *s; s++)
		kputc(*s);
}

void kputx(u32 x)
{
	static const char conv[] = "0123456789ABCDEF";

	int i;
	for (i = 32-4; i >= 0; i -= 4)
	{
		int v = (x >> i) & 0xF;
		kputc(conv[v]);
	}
}

static void _kputu(u32 x)
{
	u32 q = x / 10;
	u32 r = x % 10;
	if (q) _kputu(q);
	kputc('0' + r);
}

void kputu(u32 x)
{
	if (x == 0)
	{
		kputc('0');
		return;
	}

	_kputu(x);
}

void kputd(int x)
{
	if (x < 0)
	{
		kputc('-');
		x = -x;
	}
	_kputu(x);
}

void memtest()
{
	pageinfo_t* p1 = MemAllocPage(PAGEORDER_4K);
	pageinfo_t* p2 = MemAllocPage(PAGEORDER_8K);
	pageinfo_t* p3 = MemAllocPage(PAGEORDER_1M);
	kputs("<memtest> page 1 @ 0x"); kputx((u32)page2vphys(p1)); kputs(" -- pageinfo at 0x"); kputx((u32)p1); kputc('\n');
	kputs("<memtest> page 2 @ 0x"); kputx((u32)page2vphys(p2)); kputs(" -- pageinfo at 0x"); kputx((u32)p2); kputc('\n');
	kputs("<memtest> page 3 @ 0x"); kputx((u32)page2vphys(p3)); kputs(" -- pageinfo at 0x"); kputx((u32)p3); kputc('\n');

	MemFreePage(p1, PAGEORDER_4K);
	MemFreePage(p2, PAGEORDER_8K);
	MemFreePage(p3, PAGEORDER_1M);
}

void maptest()
{
	vu32* mem1 = (vu32*)MemMapPage((void*)0x02000000, PAGE_W);
	vu32* mem2 = (vu32*)MemMapPage((void*)0xC0100000, PAGE_W | PAGE_X);
	kputs("<maptest> page 1 @ 0x"); kputx((u32)mem1); kputc('\n');
	*mem1 = 24;
	kputs("<maptest> page 2 @ 0x"); kputx((u32)mem2); kputc('\n');
	//*mem2 = 42;

	mem2[0] = 0xE3A0002A; // mov r0, #42
	mem2[1] = 0xE12FFF1E; // bx lr
	DC_FlushRange((void*)mem2, 8);
	IC_InvalidateRange((void*)mem2, 8);

	MemProtectPage((void*)mem2, PAGE_X);

	typedef int (*myfp)();
	myfp func = (myfp)mem2;

	int ret = func();
	kputs("<maptest> Got return value "); kputd(ret); kputs(" out of the dynamic function!\n");

	MemUnmapPage((void*)mem1);
	MemUnmapPage((void*)mem2);
}

void heaptest()
{
	void* mem1 = malloc(124);
	void* mem2 = memalign(32, 35661);
	kputs("<heaptest> mem1 @ 0x"); kputx((u32)mem1); kputc('\n');
	kputs("<heaptest> mem2 @ 0x"); kputx((u32)mem2); kputc('\n');
	free(mem1);
	free(mem2);
	malloc_trim(0);
}

void vspacetest()
{
	HVSPACE h = vspace_create((void*)0xC0000000, 0x20000000);
	HVSPACE sp1 = vspace_alloc(h, 1234);
	HVSPACE sp2 = vspace_alloc(h, 76543);

	kputs("<vspacetest> sp1 @ "); kputx(vspace_getAddr(sp1)); kputs(", page count: "); kputu(vspace_getPageCount(sp1)); kputc('\n');
	kputs("<vspacetest> sp2 @ "); kputx(vspace_getAddr(sp2)); kputs(", page count: "); kputu(vspace_getPageCount(sp2)); kputc('\n');

	// These are just for testing coalescing - it's not really necessary since we free everything after this
	vspace_free(sp1);
	vspace_free(sp2);

	vspace_freeAll(h);
}

void myTimerIsr(u32* regs)
{
	kputc('T');
}

void WhateverExcpt(int type, u32 addr)
{
	kputs("\nEXCEPTION ");
	kputd(type);
	kputs(" AT ");
	kputx(addr);
	kputs("\n");

	u32 mySp, myLr;
	CpuSaveModeRegs(CPSR_MODE_IRQ, &mySp, &myLr);
	kputs("IRQ SP: "); kputx(mySp), kputs(" LR: "); kputx(myLr); kputc('\n');
	CpuSaveModeRegs(CPSR_MODE_SVC, &mySp, &myLr);
	kputs("SVC SP: "); kputx(mySp), kputs(" LR: "); kputx(myLr); kputc('\n');
	CpuSaveModeRegs(CPSR_MODE_USR, &mySp, &myLr);
	kputs("USR SP: "); kputx(mySp), kputs(" LR: "); kputx(myLr); kputc('\n');

	for(;;);
}

semaphore_t mySem;

int kmain(u32 memSize)
{
	kputs("<kmain> started\n");
	MemInit(memSize);
	irqInit();
	timerInit();
	ThrInit();

	memtest();
	maptest();
	heaptest();
	vspacetest();

	kputs("<irqtest> Installing Timer ISR...\n");
	timerStart(TIMER_HZ(60), nullptr); //myTimerIsr);
	
	kputs("<kmain> entering idle loop\n");
	SemaphoreInit(&mySem, 1);
	ThrTestCreate();

	SemaphoreDown(&mySem);

	int i;
	for (i = 0; i < 64; i ++)
	{
		kputc('T');
		ThrWaitForIRQ(0, BIT(4));
	}

	SemaphoreUp(&mySem);

	for (;;)
		ThrWaitForIRQ(0, 0);
}
