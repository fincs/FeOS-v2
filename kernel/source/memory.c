#include "common.h"

// TODO: move to header!
void kputs(const char*);
void kputu(u32);
void kputd(int);
void kputx(u32);

// Max orders: 9 (so that order 8 = 1 MB)
#define MAX_ORDERS 9

typedef struct
{
	pageinfo_t* first;
	u8* bitmap;
} buddyorder_t;

static buddyorder_t orderinfo[MAX_ORDERS];

static inline int _ToggleBit(int order, pageinfo_t* page)
{
	int entb = (page-PAGEMAP) >> (1+order);
	int mask = 1 << (entb&7);
	return (orderinfo[order].bitmap[entb>>3] ^= mask) & mask;
}

void MemInit(u32 memSize)
{
	extern u32 __temp_freeMB, __temp_physStart;
	extern int __temp_sectIdx;

	g_totalMem = memSize;
	g_usableMem = __temp_freeMB;
	g_physBase = __temp_physStart;

	kputs("<MemInit> totalMem: ");
	kputu(g_totalMem);
	kputs(" bytes - usableMem: ");
	kputu(g_usableMem);
	kputs(" MB - physBase: 0x");
	kputx(g_physBase);
	kputs("\n");

	// Initialize buddy bitmaps
	u8* pos = PHYSICAL_MEMORY + sizeof(pageinfo_t)*(g_usableMem<<8);
	u8* pos2 = pos;
	u32 i;
	for (i = 0; i < MAX_ORDERS; i ++)
	{
#ifdef DEBUG
		kputs("<MemInit> order #");
		kputu(i);
		kputs(": bitmap at 0x");
		kputx((u32)pos);
		kputs("\n");
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

	kputs("<MemInit> Available memory starts at 0x");
	kputx(0x80000000 + (i<<20));
	kputs(", and ");
	kputu(g_usableMem-i);
	kputs(" MB are usable\n");

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
	g_curPageTable = MASTER_PAGETABLE;
	CpuFlushTlb();
	CpuFlushBtac();
	CpuEnableCaches();

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
	if (!_MemRefillOrder(order))
		return nullptr; // out of memory!

	// Pop a page
	pageinfo_t* ret = orderinfo[order].first;
	orderinfo[order].first = ret->next;
	if (ret->next)
		ret->next->prev = nullptr;
	_ToggleBit(order, ret);
	return ret;
}

void MemFreePage(pageinfo_t* page, int order)
{
	if (_ToggleBit(order, page) || order == (MAX_ORDERS-1))
	{
		// Put this page back into the list
		page->next = orderinfo[order].first;
		page->prev = nullptr;
		if (page->next)
			page->next->prev = page;
		orderinfo[order].first = page;
		return;
	}

	// We need to coalesce the page with its buddy, as both are free.
	// Get the buddy
	pageinfo_t* buddy = (pageinfo_t*)((u32)page ^ ((1<<order)*sizeof(pageinfo_t)));
#ifdef DEBUG
	kputs("<MemFreePage> {");
	kputd(order);
	kputs("} Coalescing 0x");
	kputx((u32)page);
	kputs(" and 0x");
	kputx((u32)buddy);
	kputs("\n");
#endif

	// TODO: implement

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
	kputs("<MemFreePage> Now recursing into 0x");
	kputx((u32)page);
	kputs("\n");
#endif

	// Free the page
	MemFreePage(page, order+1);
}

static inline void* _CleanMapAddr(void* addr)
{
	return (void*)((u32)addr &~ 0xFFF);
}

static inline bool _IsKernel(void* addr)
{
	return (u32)addr >= 0x80000000;
}

static vu32* _GetCoarseTable(void* vaddr, bool bCreate, vu32** ppL1Entry)
{
	int i;
	if (!vaddr)
		return nullptr; // The NULL pointer region is unmappable

	bool isK = _IsKernel(vaddr);
	int idx = L1_INDEX(vaddr);

	if (isK && (idx == L1_INDEX(0xfff00000) || idx == L1_INDEX(0xe0000000)))
		return nullptr; // Can't touch that memory region! (Kernel code and system vectors)

	//vu32* entry = (isK ? MASTER_PAGETABLE : (vu32*)phys2virt(CpuGetLowerPT() &~ 0x1F)) + idx;
	vu32* entry = MASTER_PAGETABLE + idx; // FUTURE: uncomment line above - this is an ugly workaround to let the initial TTB0 to work
	vu32* coarse;

	switch (*entry & 3)
	{
		case 0:
		{
			// This 1MB section has no associated course table
			if (!bCreate)
				return nullptr; // We are not allowed to create it

			pageinfo_t* pCoarsePage = MemAllocPage(PAGEORDER_4K);
			if (!pCoarsePage)
				return nullptr; // Out of memory

			pCoarsePage->refCount = 1;
			pCoarsePage->flags = 0;
			coarse = (vu32*)page2vphys(pCoarsePage);

			// Zerofill stuff
			for (i = 0; i < 1024; i ++)
				coarse[i] = 0;
			*entry = MMU_L1_COARSE | (u32)virt2phys((void*)coarse);
			break;
		}
		case 1:
			// Get the address of the coarse table
			coarse = (vu32*)phys2virt(*entry &~ 0x3FF);
			break;
		default:
			// Unsupported and/or unmappable section
			return nullptr;
	}
	if (ppL1Entry)
		*ppL1Entry = entry;
	return coarse;
}

static inline u32 _GetPermissions(int flags, bool isK)
{
	static u32 flagCnv[8] =
	{
		MMU_L2_RORO | MMU_L2_XN_4K, MMU_L2_RWRW | MMU_L2_XN_4K, MMU_L2_RORO, MMU_L2_RWRW,
		MMU_L2_RONA | MMU_L2_XN_4K, MMU_L2_RWNA | MMU_L2_XN_4K, MMU_L2_RONA, MMU_L2_RWNA,
	};
	return flagCnv[(flags & 3) + (isK ? 4 : 0)];
}

void* MemMapPage(void* vaddr, int flags)
{
	vaddr = _CleanMapAddr(vaddr);
	vu32* entry = _GetCoarseTable(vaddr, true, nullptr);
	if (!entry)
		return nullptr; // bail out

	bool isK = _IsKernel(vaddr);
	coarseinfo_t* coarseInfo = (coarseinfo_t*)(entry + 256);
	entry += L2_INDEX(vaddr);

	if (*entry & 3)
		return nullptr; // Page already exists

	// Allocate a new page
	pageinfo_t* pPage = MemAllocPage(PAGEORDER_4K);
	if (!pPage)
		return nullptr; // Out of memory

	// Initialize page structure
	pPage->refCount = 1;
	pPage->flags = PAGE_COLOUR(vaddr);
	AtomicIncrement(&coarseInfo->pageCount);

	// Zerofill page memory
	u32* pageMem = (u32*)page2vphys(pPage);
	int i;
	for (i = 0; i < 1024; i ++)
		pageMem[i] = 0;

	u32 setting = MMU_L2_PAGE4K | MMU_L2_4K_CACHED | (u32)virt2phys(pageMem) | _GetPermissions(flags, isK);
	if (!isK)
		setting |= MMU_L2_NG; // non-global

	*entry = setting;
	// No need to do anything further:
	// - TLB doesn't store entries that do not exist
	// - Cache doesn't cache invalid addresses
	return vaddr;
}

bool MemProtectPage(void* vaddr, int flags)
{
	vaddr = _CleanMapAddr(vaddr);
	vu32* entry = _GetCoarseTable(vaddr, true, nullptr);
	if (!entry)
		return false; // bail out

	bool isK = _IsKernel(vaddr);
	entry += L2_INDEX(vaddr);

	if (!(*entry & 2))
		return false; // Missing or unsupported page

	u32 setting = *entry;
	setting &= ~MMU_L2_4K_PMASK;
	setting |= _GetPermissions(flags, isK);
	*entry = setting;
	CpuFlushTlbByAddr(vaddr);
	return true;
}

bool MemUnmapPage(void* vaddr)
{
	vaddr = _CleanMapAddr(vaddr);
#ifdef DEBUG
	kputs("<MemUnmapPage> 0x"); kputx((u32)vaddr); kputs(" to be freed\n");
#endif
	vu32* entry2 = nullptr;
	vu32* entry = _GetCoarseTable(vaddr, true, &entry2);
	if (!entry)
		return false; // bail out

	coarseinfo_t* coarseInfo = (coarseinfo_t*)(entry + 256);
	entry += L2_INDEX(vaddr);

	if (!(*entry & 2))
		return false; // Missing or unsupported page

	// Retrieve page structure
	pageinfo_t* pPage = vphys2page(phys2virt(*entry & ~0xFFF));
	if (!AtomicDecrement(&pPage->refCount))
	{
		// Flush page from the caches
		DC_FlushRange(vaddr, 0x1000);
		IC_InvalidateRange(vaddr, 0x1000);
		// Delete page
		MemFreePage(pPage, PAGEORDER_4K);
	}

	if (!AtomicDecrement(&coarseInfo->pageCount))
	{
		// Free coarse table
		pageinfo_t* pPage2 = vphys2page(phys2virt(*entry2 & ~0xFFF));
		MemFreePage(pPage2, PAGEORDER_4K);
		*entry2 = 0;
	}

	// Zap entry
	*entry = 0;
	CpuFlushTlbByAddr(vaddr);
	return true;
}