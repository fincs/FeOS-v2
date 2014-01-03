#include "common.h"

extern u32 __pagetables[];

extern char __kmem_start[];

extern char __load_addr[];

extern char __devmem_addr[];
extern char __devmem_size[];

extern char __lma_code[];
extern char __lma_code_size[];
extern char __lma_rodata_size[];
extern char __lma_data_size[];

#define MEGABYTE 0x100000
#define MEGABYTE_MASK 0xFFFFF

__attribute__((section(".initd")))
u32 __temp_freeMB, __temp_physStart;
int __temp_sectIdx;

__attribute__((section(".init2")))
void __init_pagetables(u32 totalMemSize)
{
	u32* tab = __pagetables;

	// Clear L1 and L2 tables
	int i;
	for (i = 0; i < (4096 + 2*256); i ++)
		tab[i] = MMU_FAULT;

	// TTB0: bottom 2GB, TTB1: top 2GB

	// Prepare dummy temp trampoline
	u32 pos = (u32)__load_addr &~ (MEGABYTE-1);
	__temp_sectIdx = L1_INDEX(pos);
	tab[__temp_sectIdx] = MMU_L1_SECT1MB | pos | MMU_L1_RONA | MMU_L1_UNCACHED;

	// Prepare I/O region (HACKISH)
	pos = (u32)__devmem_addr - MEGABYTE;
	int size = ((u32)__devmem_size + MEGABYTE - 1) >> 20; // MB align - HACKISH
	for (i = 0; i < size; i ++)
		tab[L1_INDEX(0xa0000000) + i] = MMU_L1_SECT1MB | (pos += MEGABYTE) | MMU_L1_RWNA | MMU_L1_DEVICEMEM;

	// Prepare coarse table for kernel
	tab[L1_INDEX((u32)__kmem_start)] = MMU_L1_COARSE | (u32)(tab + 4096);

	// Prepare coarse table for upper 1MB address space (pagetables + vectors)
	tab[L1_INDEX(0xFFF00000)] = MMU_L1_COARSE | (u32)(tab + 4096 + 256);
	tab += 4096;

	u32* tempTab = tab;

	// Fill kernel memory coarse table

	// CODE section - Read-only, executable, cacheable
	pos = (u32)__lma_code - 0x1000;
	size = (u32)__lma_code_size >> 12;
	for (i = 0; i < size; i ++)
		*tab++ = MMU_L2_PAGE4K | (pos += 0x1000) | MMU_L2_RONA | MMU_L2_4K_CACHED;

	u32 vecPos = (pos += 0x1000);

	// RODATA section - Read-only, non-executable, cacheable
	size = (u32)__lma_rodata_size >> 12;
	for (i = 0; i < size; i ++)
		*tab++ = MMU_L2_PAGE4K | (pos += 0x1000) | MMU_L2_RONA | MMU_L2_XN_4K | MMU_L2_4K_CACHED;

	// DATA section - Read/write, non-executable, cacheable
	size = (u32)__lma_data_size >> 12;
	for (i = 0; i < size; i ++)
		*tab++ = MMU_L2_PAGE4K | (pos += 0x1000) | MMU_L2_RWNA | MMU_L2_XN_4K | MMU_L2_4K_CACHED;

	// Stack - Read/write, non-executable, cacheable
	size = 4; // 16KB of stack
	tab = tempTab + 256 - size; // place it at the end of the 1MB kernel memory coarse pagetable
	for (i = 0; i < size; i ++)
		*tab++ = MMU_L2_PAGE4K | (pos += 0x1000) | MMU_L2_RWNA | MMU_L2_XN_4K | MMU_L2_4K_CACHED;

	u32 firstFreePage = pos;

	// Fill vector memory coarse table

	// Pagetable entries
	size = 5; // 20 KB, enough to fit the two L1 and L2 pagetables
	pos = (u32)__pagetables - 0x1000;
	for (i = 0; i < 5; i ++)
		tab[i] = MMU_L2_PAGE4K | (pos += 0x1000) | MMU_L2_RWNA | MMU_L2_XN_4K | MMU_L2_4K_UNCACHED;

	// CPU exception vector memory - Read-only, executable, cacheable
	tab[L2_INDEX(0xFFFF0000)] = MMU_L2_PAGE4K | vecPos | MMU_L2_RONA | MMU_L2_4K_CACHED;

	// Prepare direct physical memory mapping
	pos = (firstFreePage + MEGABYTE_MASK) &~ MEGABYTE_MASK; // MB-align
	u32 freeMem = ((u32)((u8*)__pagetables + totalMemSize) - pos) &~ MEGABYTE_MASK;
	size = freeMem >> 20;
	__temp_freeMB = size;
	__temp_physStart = pos;
	tab = __pagetables + L1_INDEX(0x80000000);
	pos -= MEGABYTE;
	for (i = 0; i < size; i ++)
		*tab++ = MMU_L1_SECT1MB | (pos += MEGABYTE) | MMU_L1_RWNA | MMU_L1_UNCACHED;
}
