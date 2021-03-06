#include "common.h"

extern char __kmem_top[];
static void* heapPtr = (void*)__kmem_top;
static void* nextPage = (void*)__kmem_top;
static u32 heapSize = 0;
static u32 heapPages = 0;

static inline u32 remainingHeap()
{
	return (heapPages<<12) - heapSize;
}

static inline u32 toPageCount(u32 x)
{
	return (x+0xFFF) >> 12;
}

void* sbrk(int delta)
{
	kprintf("<sbrk> delta=%d\n", delta);

	if (!delta) return heapPtr;

#ifdef DEBUG
	kputs("<sbrk> real work\n");
#endif

	void* ret = heapPtr;
	int i;

	if (delta > 0)
	{
		u32 rh = remainingHeap();
		if (rh < delta)
		{
			// We need to add more pages
			int reqPages = toPageCount(delta - rh);
			for (i = 0; i < reqPages; i ++)
			{
				void* page = MemMapPage(nextPage, PAGE_W);
				if (!page)
					return ((void*)-1); // Out of memory
#ifdef DEBUG
				kprintf("<sbrk> mapped %p\n", page);
#endif
				nextPage = (char*)page + 0x1000;
				heapPages ++;
			}
		}

		// Advance heap pointer
		heapPtr = (char*)heapPtr + delta;
		heapSize += delta;
	} else
	{
		if ((-delta) > heapSize)
			return ((void*)-1); // bad

		heapPtr = (char*)heapPtr + delta;
		heapSize += delta;

		// Free up unused pages
		int rh = remainingHeap() >> 12;
		for (i = 0; i < rh; i ++)
		{
			nextPage = (char*)nextPage - 0x1000;
			heapPages --;
			MemUnmapPage(nextPage);
#ifdef DEBUG
			kprintf("<sbrk> unmapped %p\n", nextPage);
#endif
		}
	}

#ifdef DEBUG
	kputs("<sbrk> successful return\n");
#endif
	return ret;
}
