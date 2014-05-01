#include "common.h"

// Kernel L1 MMU table mutex
static semaphore_t kvmMutex = SEMAPHORE_STATIC_INIT(1);

static inline void* _CleanMapAddr(void* addr)
{
	return (void*)((u32)addr &~ 0xFFF);
}

static inline bool _IsKernel(void* addr)
{
	return (u32)addr >= 0x80000000;
}

static inline vu32* _GetVMTable(void* addr, semaphore_t** ppSem)
{
	if (_IsKernel(addr))
	{
		if (ppSem)
			*ppSem = &kvmMutex;
		return MASTER_PAGETABLE;
	} else
	{
		processInfo* ps = ThrSchedInfo()->curProcess;
		if (ppSem)
			*ppSem = &ps->vmMutex;
		return ps->vmTable;
	}
}

static bool _GetL2PageTable(l2info_t* cInfo, void* vaddr, bool bCreate)
{
	int i;
	if (!vaddr)
		return false; // The NULL pointer region is unmappable

	int idx = L1_INDEX(vaddr);

	if (_IsKernel(vaddr) && (idx == L1_INDEX(0xfff00000) || idx == L1_INDEX(0xe0000000)))
		return false; // Can't touch that memory region! (Kernel code and system vectors)

	cInfo->mutex = nullptr;
	vu32* vmTable = _GetVMTable(vaddr, &cInfo->mutex);
	vu32* entry = vmTable + idx;
	cInfo->l1Entries = vmTable + (idx &~ 1);

#ifdef DEBUG
	kprintf("<_GetL2PageTable> vaddr=%p bCreate=%d idx=%d idx2=%d\n", vaddr, bCreate, idx, idx&~1);
#endif

	// Take VM mutex
	SemaphoreDown(cInfo->mutex);
	switch (*entry & 3)
	{
		case 0:
		{
			// This 1MB section has no associated L2 table
			if (!bCreate)
			{
				SemaphoreUp(cInfo->mutex);
				return false; // We are not allowed to create it
			}

			// Allocate a page for usage with the pair of L2 tables
			cInfo->page = MemAllocPage(PAGEORDER_4K);
			if (!cInfo->page)
			{
				SemaphoreUp(cInfo->mutex);
				return false; // Out of memory
			}

			// Zerofill the L2 tables
			vu32* l2Table = (vu32*)page2vphys(cInfo->page);
			for (i = 0; i < 1024; i ++)
				l2Table[i] = 0;

			// Register the L2 tables
			u32 l1Entries[2];
			l1Entries[0] = MMU_L1_COARSE | (u32)virt2phys((void*)l2Table);
			l1Entries[1] = l1Entries[0] + 2*1024;

			if (idx != L1_INDEX(0xe0100000)) // Avoid touching the kernel executable L2 table
				cInfo->l1Entries[0] = l1Entries[0];
			else
				cInfo->page->flags |= PAGEFLAG_STICKY;
			if (idx != L1_INDEX(0xffe00000)) // Avoid touching the system vector L2 table
				cInfo->l1Entries[1] = l1Entries[1];
			else
				cInfo->page->flags |= PAGEFLAG_STICKY;

			// Retrieve the L2 table associated to the address the user specified
			cInfo->table = l2Table + ((idx & 1) << 9);

#ifdef DEBUG
			kprintf("<_GetL2PageTable> created. ent1=%x ent2=%x page=%p table=%p retTable=%p\n",
				cInfo->l1Entries[0], cInfo->l1Entries[1], cInfo->page,
				l2Table, cInfo->table);
#endif

			// Keep the reference count of the page alive
			return true;
		}

		case 1:
		{
			// Retrieve the L2 table associated to the address the user specified
			cInfo->table = (vu32*)phys2virt(*entry &~ 0x3FF);

			// Retrieve the page associated to the L2 table and increment its reference count
			cInfo->page = vphys2page((void*)cInfo->table);
			MemPageIncrRef(cInfo->page);

#ifdef DEBUG
			kprintf("<_GetL2PageTable> found. ent1=%x ent2=%x page=%p retTable=%p\n",
				cInfo->l1Entries[0], cInfo->l1Entries[1], cInfo->page, cInfo->table);
#endif
			return true;
		}

		default:
		{
			// Invalid or unsupported situation
			SemaphoreUp(cInfo->mutex);
			return false;
		}
	}
}

static void _FreeL2PageTable(l2info_t* cInfo)
{
	if (!MemPageDecrRef(cInfo->page) && !(cInfo->page->flags & PAGEFLAG_STICKY))
	{
		// We need to free the L2 table's page
		cInfo->l1Entries[0] = 0;
		cInfo->l1Entries[1] = 0;
		MemFreePage(cInfo->page, PAGEORDER_4K);
	}

	// Release VM mutex
	SemaphoreUp(cInfo->mutex);
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
	l2info_t cInfo;
	if (!_GetL2PageTable(&cInfo, vaddr, true))
		return nullptr; // bail out

	vu32* entry = cInfo.table + L2_INDEX(vaddr);
	bool isK = _IsKernel(vaddr);

	if (*entry & 3)
	{
		_FreeL2PageTable(&cInfo);
		return nullptr; // Page already exists
	}

	// Allocate a new page
	pageinfo_t* pPage = MemAllocPage(PAGEORDER_4K);
	if (!pPage)
	{
		_FreeL2PageTable(&cInfo);
		return nullptr; // Out of memory
	}

	// Initialize page structure
	pPage->flags |= PAGEFLAG_HASCOLOUR | PAGE_COLOUR(vaddr);
#ifdef DEBUG
	kprintf("{DBG} cInfo: {%p,%p,[%x,%x]}\n", cInfo.table, cInfo.mutex, cInfo.l1Entries[0], cInfo.l1Entries[1]);
	kprintf("{DBG} pPage: %p\n", pPage);
#endif
	MemPageIncrRef(cInfo.page);

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
	_FreeL2PageTable(&cInfo);
	return vaddr;
}

bool MemProtectPage(void* vaddr, int flags)
{
	vaddr = _CleanMapAddr(vaddr);
	l2info_t cInfo;
	if (!_GetL2PageTable(&cInfo, vaddr, false))
		return nullptr; // bail out

	vu32* entry = cInfo.table + L2_INDEX(vaddr);
	bool isK = _IsKernel(vaddr);

	if (!(*entry & 2))
	{
		_FreeL2PageTable(&cInfo);
		return false; // Missing or unsupported page
	}

	// Change the protection settings
	u32 setting = *entry;
	setting &= ~MMU_L2_4K_PMASK;
	setting |= _GetPermissions(flags, isK);
	*entry = setting;
	CpuFlushTlbByAddr(vaddr);

	_FreeL2PageTable(&cInfo);
	return true;
}

bool MemUnmapPage(void* vaddr)
{
	vaddr = _CleanMapAddr(vaddr);
#ifdef DEBUG
	kprintf("<MemUnmapPage> %p to be freed\n", vaddr);
#endif
	l2info_t cInfo;
	if (!_GetL2PageTable(&cInfo, vaddr, false))
		return nullptr; // bail out

	vu32* entry = cInfo.table + L2_INDEX(vaddr);

	if (!(*entry & 2))
	{
		_FreeL2PageTable(&cInfo);
		return false; // Missing or unsupported page
	}

	// Retrieve page structure
	pageinfo_t* pPage = vphys2page(phys2virt(*entry & ~0xFFF));

	// Free page
	if (!MemPageDecrRef(pPage))
	{
#ifndef HAS_PIPT_CACHE
		// Flush page from the caches
		DC_FlushRange(vaddr, 0x1000);
		IC_InvalidateRange(vaddr, 0x1000);
#endif
		// Delete page
		if (!(pPage->flags & PAGEFLAG_STICKY))
			MemFreePage(pPage, PAGEORDER_4K);
	}

	// Zap entry and clear TLB
	*entry = 0;
	entry[256] = 0;
	MemPageDecrRef(cInfo.page);
	CpuFlushTlbByAddr(vaddr);

	// Return - this will also take care of deleting the L2 table if necessary
	_FreeL2PageTable(&cInfo);
	return true;
}

u32 MemTranslateAddr(void* address)
{
	u32 vaddr = (u32)address;
	u32 L1entry = _GetVMTable(address, nullptr)[L1_INDEX(vaddr)];
	switch (L1entry & 3)
	{
		case MMU_L1_COARSE:
		{
			L1entry &= ~(BIT(10)-1);
			u32 L2entry = ((vu32*)safe_phys2virt(L1entry))[L2_INDEX(vaddr)];
			if (L2entry & MMU_L2_PAGE4K)
			{
				L2entry &= ~(BIT(12)-1); // Clear 4K
				L2entry |= vaddr & (BIT(12)-1); // Add offset
				return L2entry;
			}
			return ~0; // Either fault or unsupported
		}

		case MMU_L1_SECT1MB:
		{
			if (L1entry & BIT(18)) // 16MB Section
				return ~0; // Unsupported
			L1entry &= ~(BIT(20)-1); // Clear MB
			L1entry |= vaddr & (BIT(20)-1); // Add offset
			return L1entry;
		}

		default: // Invalid
			return ~0;
	}
}
