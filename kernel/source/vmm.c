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

static inline vu32* _GetVMTable(void* addr, semaphore_t** ppSem, processInfo* ps)
{
	if (_IsKernel(addr))
	{
		if (ppSem)
			*ppSem = &kvmMutex;
		return MASTER_PAGETABLE;
	} else
	{
		if (!ps)
			ps = ThrSchedInfo()->curProcess;
		if (ppSem)
			*ppSem = &ps->vmMutex;
		return ps->vmTable;
	}
}

bool VmmOpen(vmminfo_t* vmInfo, processInfo* proc, void* addr)
{
	vmInfo->mutex = nullptr;
	vmInfo->l2_table = nullptr;
	vmInfo->curIndex = -1;
	vmInfo->curPage = nullptr;
	vmInfo->l1_table = _GetVMTable(addr, &vmInfo->mutex, proc);
	if (vmInfo->l1_table)
	{
		SemaphoreDown(vmInfo->mutex);
		return true;
	}
	return false;
}

static void _VmmCleanL2(vmminfo_t* vmInfo)
{
	if (vmInfo->curIndex < 0)
		return;

	if (!MemPageDecrRef(vmInfo->curPage) && !(vmInfo->curPage->flags & PAGEFLAG_STICKY))
	{
		// We need to free the L2 table's page
		vu32* ent = vmInfo->l1_table + vmInfo->curIndex;
		ent[0] = 0;
		ent[1] = 0;
		MemFreePage(vmInfo->curPage, PAGEORDER_4K);
	}

	vmInfo->curIndex = -1;
}

void VmmClose(vmminfo_t* vmInfo)
{
	_VmmCleanL2(vmInfo);
	SemaphoreUp(vmInfo->mutex);
}

vu32* VmmGetEntry(vmminfo_t* vmInfo, void* addr, bool bCreate)
{
	int i, idx = L1_INDEX(addr), idx2 = idx &~ 1;

	// Disallow forbidden memory regions (Zero page, Kernel code and System Vectors)
	if (!idx || idx == L1_INDEX(0xe0000000) || idx == L1_INDEX(0xfff00000))
		return nullptr;

	// Check if we hadn't already had retrieved the region
	if (idx2 != vmInfo->curIndex)
	{
		_VmmCleanL2(vmInfo);
		u32 entry = vmInfo->l1_table[idx];
		switch (entry & 3)
		{
			case 0:
			{
				// This 1MB section pair has no associated L2 table pair
				if (!bCreate)
					return nullptr; // We are not allowed to create it

				// Allocate a page for usage with the pair of L2 tables
				vmInfo->curPage = MemAllocPage(PAGEORDER_4K);
				if (!vmInfo->curPage)
					return nullptr; // Out of memory

				// Zerofill the L2 tables
				vmInfo->l2_table = (vu32*)page2vphys(vmInfo->curPage);
				for (i = 0; i < 1024; i ++)
					vmInfo->l2_table[i] = 0;

				// Register the L2 tables
				u32 ent[2];
				ent[0] = MMU_L1_COARSE | (u32)virt2phys((void*)vmInfo->l2_table);
				ent[1] = ent[0] + 2*1024;

				if (idx != L1_INDEX(0xe0100000)) // Avoid touching the kernel executable L2 table
					vmInfo->l1_table[idx2+0] = ent[0];
				else
					vmInfo->curPage->flags |= PAGEFLAG_STICKY;
				if (idx != L1_INDEX(0xffe00000)) // Avoid touching the system vector L2 table
					vmInfo->l1_table[idx2+1] = ent[1];
				else
					vmInfo->curPage->flags |= PAGEFLAG_STICKY;
				break;
			}

			case 1:
			{
				// Retrieve the L2 table pair associated to this 1MB section pair
				vmInfo->l2_table = (vu32*)phys2virt(entry &~ 0xFFF);

				// Retrieve the page associated to the L2 table pair and increment its reference count
				vmInfo->curPage = vphys2page((void*)vmInfo->l2_table);
				MemPageIncrRef(vmInfo->curPage);
				break;
			}

			default:
				// Invalid or unsupported situation
				return nullptr;
		}
	}

	// Retrieve the pointer to the L2 entry
	vmInfo->curIndex = idx;
	return vmInfo->l2_table + ((idx & 1) << 9) + L2_INDEX(addr);
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
	vmminfo_t vmInfo;
	if (!VmmOpen(&vmInfo, nullptr, vaddr))
		return nullptr; // bail out

	vu32* entry = VmmGetEntry(&vmInfo, vaddr, true);
	if (!entry)
	{
		VmmClose(&vmInfo);
		return nullptr; // bail out
	}

	bool isK = _IsKernel(vaddr);

	if (*entry & 3)
	{
		VmmClose(&vmInfo);
		return nullptr; // Page already exists
	}

	// Allocate a new page
	pageinfo_t* pPage = MemAllocPage(PAGEORDER_4K);
	if (!pPage)
	{
		VmmClose(&vmInfo);
		return nullptr; // Out of memory
	}

	// Initialize page structure
	pPage->flags |= PAGEFLAG_HASCOLOUR | PAGE_COLOUR(vaddr);
	VmmPageIncrRef(&vmInfo);

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
	VmmClose(&vmInfo);
	return vaddr;
}

bool MemProtectPage(void* vaddr, int flags)
{
	vaddr = _CleanMapAddr(vaddr);
	vmminfo_t vmInfo;
	if (!VmmOpen(&vmInfo, nullptr, vaddr))
		return nullptr; // bail out

	vu32* entry = VmmGetEntry(&vmInfo, vaddr, false);
	if (!entry)
	{
		VmmClose(&vmInfo);
		return nullptr; // bail out
	}

	bool isK = _IsKernel(vaddr);

	if (!(*entry & 2))
	{
		VmmClose(&vmInfo);
		return false; // Missing or unsupported page
	}

	// Change the protection settings
	u32 setting = *entry;
	setting &= ~MMU_L2_4K_PMASK;
	setting |= _GetPermissions(flags, isK);
	*entry = setting;
	CpuFlushTlbByAddr(vaddr);

	VmmClose(&vmInfo);
	return true;
}

bool MemUnmapPage(void* vaddr)
{
	vaddr = _CleanMapAddr(vaddr);
#ifdef DEBUG
	kprintf("<MemUnmapPage> %p to be freed\n", vaddr);
#endif
	vmminfo_t vmInfo;
	if (!VmmOpen(&vmInfo, nullptr, vaddr))
		return nullptr; // bail out

	vu32* entry = VmmGetEntry(&vmInfo, vaddr, false);
	if (!entry)
	{
		VmmClose(&vmInfo);
		return nullptr; // bail out
	}

	if (!(*entry & 2))
	{
		VmmClose(&vmInfo);
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
	VmmPageDecrRef(&vmInfo);
	CpuFlushTlbByAddr(vaddr);

	// Return - this will also take care of deleting the L2 table if necessary
	VmmClose(&vmInfo);
	return true;
}

u32 MemTranslateAddr(void* address)
{
	u32 vaddr = (u32)address;
	u32 L1entry = _GetVMTable(address, nullptr, nullptr)[L1_INDEX(vaddr)];
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
