#include "common.h"

void memtest()
{
	pageinfo_t* p1 = MemAllocPage(PAGEORDER_4K);
	pageinfo_t* p2 = MemAllocPage(PAGEORDER_8K);
	pageinfo_t* p3 = MemAllocPage(PAGEORDER_1M);
	kprintf("<memtest> page 1 @ %p -- pageinfo at %p\n", page2vphys(p1), p1);
	kprintf("<memtest> page 2 @ %p -- pageinfo at %p\n", page2vphys(p2), p2);
	kprintf("<memtest> page 3 @ %p -- pageinfo at %p\n", page2vphys(p3), p3);

	MemFreePage(p1, PAGEORDER_4K);
	MemFreePage(p2, PAGEORDER_8K);
	MemFreePage(p3, PAGEORDER_1M);
}

void maptest()
{
	vu32* mem1 = (vu32*)MemMapPage((void*)0x02000000, PAGE_W);
	vu32* mem2 = (vu32*)MemMapPage((void*)0xC0100000, PAGE_W | PAGE_X);
	kprintf("<maptest> page 1 @ %p\n", mem1);
	*mem1 = 24;
	kprintf("<maptest> page 2 @ %p\n", mem2);
	//*mem2 = 42;

	mem2[0] = 0xE3A0002A; // mov r0, #42
	mem2[1] = 0xE12FFF1E; // bx lr
	DC_FlushRange((void*)mem2, 8);
	IC_InvalidateRange((void*)mem2, 8);

	MemProtectPage((void*)mem2, PAGE_X);

	typedef int (*myfp)();
	myfp func = (myfp)mem2;

	int ret = func();
	kprintf("<maptest> Got return value %d out of the dynamic function!\n", ret);

	MemUnmapPage((void*)mem1);
	MemUnmapPage((void*)mem2);
}

void heaptest()
{
	void* mem1 = malloc(124);
	void* mem2 = memalign(32, 35661);
	kprintf("<heaptest> mem1 @ %p\n", mem1);
	kprintf("<heaptest> mem2 @ %p\n", mem2);
	free(mem1);
	free(mem2);
	malloc_trim(0);
}

void vspacetest()
{
	HVSPACE h = vspace_create((void*)0xC0000000, 0x20000000);
	HVSPACE sp1 = vspace_alloc(h, 1234);
	HVSPACE sp2 = vspace_alloc(h, 76543);

	kprintf("<vspacetest> sp1 @ %p, page count: %u\n", vspace_getAddr(sp1), vspace_getPageCount(sp1));
	kprintf("<vspacetest> sp2 @ %p, page count: %u\n", vspace_getAddr(sp2), vspace_getPageCount(sp2));

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
	kprintf("\nEXCEPTION %d AT %p\n", type, addr);

	u32 mySp, myLr;
	CpuSaveModeRegs(CPSR_MODE_IRQ, &mySp, &myLr);
	kprintf("IRQ SP: %x LR: %x\n", mySp, myLr);
	CpuSaveModeRegs(CPSR_MODE_SVC, &mySp, &myLr);
	kprintf("SVC SP: %x LR: %x\n", mySp, myLr);
	CpuSaveModeRegs(CPSR_MODE_USR, &mySp, &myLr);
	kprintf("USR SP: %x LR: %x\n", mySp, myLr);

	for(;;);
}

semaphore_t mySem;

int kmain(u32 memSize)
{
	KioInit();
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
