#include "common.h"

// Max orders: 9 (so that order 8 = 1 MB)
#define MAX_ORDERS 9

typedef struct
{
	pageinfo_t* first;
	u8* bitmap;
} buddyorder_t;

static buddyorder_t orderinfo[MAX_ORDERS];
// Physical memory management mutex
static semaphore_t mmMutex = SEMAPHORE_STATIC_INIT(1);

static inline int _ToggleBit(int order, pageinfo_t* page)
{
	int entb = (page-PAGEMAP) >> (1+order);
	int mask = 1 << (entb&7);
	return (orderinfo[order].bitmap[entb>>3] ^= mask) & mask;
}

void MemInit(u32 memSize, u32 initRdAddr, u32 initRdSize)
{
	extern u32 __temp_freeMB, __temp_physStart;
	extern int __temp_sectIdx;

	g_totalMem = memSize;
	g_usableMem = __temp_freeMB;
	g_physBase = __temp_physStart;

	kprintf("<MemInit> totalMem: %u bytes - usableMem: %u MB - physBase: %p\n", g_totalMem, g_usableMem, g_physBase);
	kprintf("<MemInit> initrd @ %p (%u bytes)\n", initRdAddr, initRdSize);

	// Initialize buddy bitmaps
	u8* pos = PHYSICAL_MEMORY + sizeof(pageinfo_t)*(g_usableMem<<8);
	u8* pos2 = pos;
	u32 i;
	for (i = 0; i < MAX_ORDERS; i ++)
	{
#ifdef DEBUG
		kprintf("<MemInit> order #%u: bitmap at %p\n", i, pos);
#endif
		orderinfo[i].bitmap = pos;

		pos += ((g_usableMem << (8-i)) + 15) >> 4;
	}

	// Clear bitmaps
	while (pos2 != pos)
		*pos2++ = 0;

	// Initialize order-8 info (MB)
	i = (pos - PHYSICAL_MEMORY + 0xFFFFF) >> 20;
	pageinfo_t** last = &orderinfo[8].first;
	pageinfo_t* cur = PAGEMAP + (i << 8);
	pageinfo_t* prev = nullptr;

	kprintf("<MemInit> Available memory starts at %p, and %u MB are usable\n", 0x80000000 + (i<<20), g_usableMem-i);

	for (; i < g_usableMem; i ++)
	{
		*last = cur;
		cur->prev = prev;
		last = &cur->next;
		prev = cur;
		cur += 1 << 8;
	}

	// Finish initializing the MMU
	MASTER_PAGETABLE[__temp_sectIdx] = MMU_FAULT;
	CpuFlushTlb();
	CpuFlushBtac();
	CpuEnableCaches();

	// Fill the initRd globals
	if (initRdSize)
	{
		g_initRd = phys2virt(initRdAddr);
		g_initRdSize = initRdSize;
	}

	kputs("<MemInit> Everything OK!\n");
}

static bool _MemRefillOrder(int order)
{
	if (orderinfo[order].first)
		return true; // we have an available page

	if (order == MAX_ORDERS)
		return false; // out of memory

	if (!_MemRefillOrder(order+1))
		return false; // could not refill higher order = out of memory

	// Pop a page
	pageinfo_t* high = orderinfo[order+1].first;
	orderinfo[order+1].first = high->next;
	if (high->next)
		high->next->prev = nullptr;
	_ToggleBit(order+1, high);
	
	// Register it
	orderinfo[order].first = high;
	high->next = high + (1<<order);
	high->prev = nullptr;
	high->next->next = nullptr;
	high->next->prev = high;
	return true;
}

pageinfo_t* MemAllocPage(int order)
{
	SemaphoreDown(&mmMutex);
	if (!_MemRefillOrder(order))
	{
		SemaphoreUp(&mmMutex);
		return nullptr; // out of memory!
	}

	// Pop a page
	pageinfo_t* ret = orderinfo[order].first;
	orderinfo[order].first = ret->next;
	if (ret->next)
		ret->next->prev = nullptr;
	_ToggleBit(order, ret);
	SemaphoreUp(&mmMutex);

	// Initialize its contents and return it
	ret->refCount = 1;
	ret->flags = 0;
	return ret;
}

void MemFreePage(pageinfo_t* page, int order)
{
	SemaphoreDown(&mmMutex);
	if (_ToggleBit(order, page) || order == (MAX_ORDERS-1))
	{
		// Put this page back into the list
		page->next = orderinfo[order].first;
		page->prev = nullptr;
		if (page->next)
			page->next->prev = page;
		orderinfo[order].first = page;
		SemaphoreUp(&mmMutex);
		return;
	}

	// We need to coalesce the page with its buddy, as both are free.
	// Get the buddy
	pageinfo_t* buddy = (pageinfo_t*)((u32)page ^ ((1<<order)*sizeof(pageinfo_t)));
#ifdef DEBUG
	kprintf("<MemFreePage> {%d} Coalescing %p and %p\n", order, page, buddy);
#endif

	// Remove the buddy from the linked list
	{
		pageinfo_t* prev = buddy->prev;
		if (prev)
			prev->next = buddy->next;
		else
			orderinfo[order].first = buddy->next;
		if (buddy->next)
			buddy->next->prev = prev;
	}

	// Get the first buddy
	page = page < buddy ? page : buddy;

#ifdef DEBUG
	kprintf("<MemFreePage> Now recursing into %p\n", page);
#endif

	// Free the page
	SemaphoreUp(&mmMutex);
	MemFreePage(page, order+1);
}
