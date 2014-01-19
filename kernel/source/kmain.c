#include "common.h"

static void memtest()
{
	pageinfo_t* p1 = MemAllocPage(PAGEORDER_4K);
	pageinfo_t* p2 = MemAllocPage(PAGEORDER_8K);
	pageinfo_t* p3 = MemAllocPage(PAGEORDER_1M);
	printf("<memtest> page 1 @ %p -- pageinfo at %p\n", page2vphys(p1), p1);
	printf("<memtest> page 2 @ %p -- pageinfo at %p\n", page2vphys(p2), p2);
	printf("<memtest> page 3 @ %p -- pageinfo at %p\n", page2vphys(p3), p3);

	MemFreePage(p1, PAGEORDER_4K);
	MemFreePage(p2, PAGEORDER_8K);
	MemFreePage(p3, PAGEORDER_1M);
}

static void maptest()
{
	vu32* mem1 = (vu32*)MemMapPage((void*)0x02000000, PAGE_W);
	vu32* mem2 = (vu32*)MemMapPage((void*)0xC0100000, PAGE_W | PAGE_X);
	printf("<maptest> page 1 @ %p\n", mem1);
	*mem1 = 24;
	printf("<maptest> page 2 @ %p\n", mem2);
	//*mem2 = 42;

	mem2[0] = 0xE3A0002A; // mov r0, #42
	mem2[1] = 0xE12FFF1E; // bx lr
	DC_FlushRange((void*)mem2, 8);
	IC_InvalidateRange((void*)mem2, 8);

	MemProtectPage((void*)mem2, PAGE_X);

	typedef int (*myfp)();
	myfp func = (myfp)mem2;

	int ret = func();
	printf("<maptest> Got return value %d out of the dynamic function!\n", ret);

	MemUnmapPage((void*)mem1);
	MemUnmapPage((void*)mem2);
}

static void heaptest()
{
	void* mem1 = malloc(124);
	void* mem2 = memalign(32, 35661);
	printf("<heaptest> mem1 @ %p\n", mem1);
	printf("<heaptest> mem2 @ %p\n", mem2);
	free(mem1);
	free(mem2);
	malloc_trim(0);
}

static void vspacetest()
{
	HVSPACE h = vspace_create((void*)0xC0000000, 0x20000000);
	HVSPACE sp1 = vspace_alloc(h, 1234);
	HVSPACE sp2 = vspace_alloc(h, 76543);

	printf("<vspacetest> sp1 @ %p, page count: %u\n", (void*)vspace_getAddr(sp1), vspace_getPageCount(sp1));
	printf("<vspacetest> sp2 @ %p, page count: %u\n", (void*)vspace_getAddr(sp2), vspace_getPageCount(sp2));

	// These are just for testing coalescing - it's not really necessary since we free everything after this
	vspace_free(sp1);
	vspace_free(sp2);

	vspace_freeAll(h);
}

static void contest()
{
	printf("FeOS/RPi Console Demo\n");
	printf("Hello World!\n");
	printf("Te\tst\tting tabs\n\n");
	printf("\tHello World #FeOS\n");
	printf("\thttp://feos.mtheall.com\n");
}

//static semaphore_t mySem;

static int threadEnt(void* param)
{
	const char* str = (const char*)param;
	printf("<ThrTestMain> I was given %s\nWoohoo!\n", str);

	int i;
	//SemaphoreDown(&mySem);
	for (i = 0; i < 128; i ++)
	{
		printf("[%d]", i);
		ThrSleep(50);
	}
	//SemaphoreUp(&mySem);

	return 0;
}

static void thrtest()
{
	//SemaphoreInit(&mySem, 1);
	threadInfo* t = ThrCreateK(threadEnt, "Hello world!", 5, DEFAULT_STACKSIZE);
	if (!t)
		printf("<thrtest> Could not create thread!\n");

	//SemaphoreDown(&mySem);

	int i;
	for (i = 0; /*i < 64*/; i ++)
	{
		printf("<%d>", i);
		ThrSleep(250);
	}

	//SemaphoreUp(&mySem);
}

int kmain(u32 memSize)
{
	KioInit();
	kputs("<kmain> started\n");
	MemInit(memSize);
	irqInit();
	timerInit();
	ThrInit();
	DevInit();

	printf(
		"FeOS/High Kernel v0.0-" FEOS_PLAT "-prerelease\n"
		"  by FeOS Team, 2013-2014\n"
		"\n");

	memtest();
	maptest();
	heaptest();
	vspacetest();
	contest();
	thrtest();

	printf("<kmain> entering idle loop\n");

	for (;;)
		ThrWaitForIRQ(0, 0);
}
