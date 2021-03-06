#pragma once
#include "common.h"

#define MASTER_PAGETABLE ((vu32*)0xFFF00000)

#define MMU_FAULT 0

#define MMU_L1_COARSE   1
#define MMU_L1_SECT1MB  2
#define MMU_L1_SECT16MB (MMU_L1_SECT1MB | BIT(18))

#define MMU_L2_PAGE64K 1
#define MMU_L2_PAGE4K  2

// L1 entry bits
#define MMU_L1_APX    BIT(15)
#define MMU_L1_AP(n)  ((n)<<10)
#define MMU_L1_XN     BIT(4)
#define MMU_L1_S      BIT(16)   // S (Shared) bit
#define MMU_L1_TEX(n) ((n)<<12) // Type EXtension field
#define MMU_L1_C      BIT(1) // C (Cacheable) bit
#define MMU_L1_B      BIT(2) // B (Bufferable) bit
#define MMU_L1_IC(n)  ((n)<<1) // In Cached memory mode: Inner cache policy field
#define MMU_L1_OC(n)  MMU_L1_TEX(4|(n)) // Cached mode + outer policy
#define MMU_L1_CP(n)  (MMU_L1_OC(n) || MMU_L1_IC(n)) // Sets cached mode and same policy in inner and outer

// L2 entry bits
#define MMU_L2_APX        BIT(9)
#define MMU_L2_AP(n)      ((n)<<4)
#define MMU_L2_XN_64K     BIT(15)
#define MMU_L2_XN_4K      BIT(0)
#define MMU_L2_S          BIT(10) // S (Shared) bit
#define MMU_L2_NG         BIT(11) // NonGlobal bit (ASID-bound)
#define MMU_L2_TEX_4K(n)  ((n)<<6)  // (4K entry) Type EXtension field
#define MMU_L2_TEX_64K(n) ((n)<<12) // (64K entry) Same as above
#define MMU_L2_C          BIT(4) // C (Cacheable) bit
#define MMU_L2_B          BIT(3) // B (Bufferable) bit
#define MMU_L2_IC(n)      ((n)<<3) // In Cached memory mode: Inner cache policy field
#define MMU_L2_OC_4K(n)   MMU_L2_TEX_4K(4|(n))  // (4K entry) Cached mode + outer policy
#define MMU_L2_OC_64K(n)  MMU_L2_TEX_64K(4|(n)) // (64K entry) Same as above
#define MMU_L2_CP_4K(n)   (MMU_L2_OC_4K(n) | MMU_L2_IC(n))   // (4K entry) Sets cached mode and same policy in inner and outer
#define MMU_L2_CP_64K(n)   (MMU_L2_OC_64K(n) | MMU_L2_IC(n)) // (64K entry) Same as above

// Access permissions
#define MMU_AP_NANA 0
#define MMU_AP_RWNA 1
#define MMU_AP_RWRO 2
#define MMU_AP_RWRW 3

// Cache policies
#define MMU_CP_NONE 0 // uncached memory
#define MMU_CP_WBWA 1 // write-back, write-allocated (also includes read)
#define MMU_CP_WTRA 2 // write-through, read-allocated
#define MMU_CP_WBRA 3 // write-back, read-allocated

// Default cache options
#define MMU_L2_4K_CACHED    MMU_L2_CP_4K(MMU_CP_WBRA)
#define MMU_L2_64K_CACHED   MMU_L2_CP_64K(MMU_CP_WBRA)
#define MMU_L2_4K_UNCACHED  MMU_L2_TEX_4K(1)
#define MMU_L2_64K_UNCACHED MMU_L2_TEX_4K(1)
#define MMU_L2_DEVICEMEM    MMU_L2_B
#define MMU_L1_CACHED       MMU_L1_CP(MMU_CP_WBRA)
#define MMU_L1_UNCACHED     MMU_L1_TEX(1)
#define MMU_L1_DEVICEMEM    MMU_L1_B

// L1 access permissions
#define MMU_L1_NANA MMU_L1_AP(MMU_AP_NANA)
#define MMU_L1_RWNA MMU_L1_AP(MMU_AP_RWNA)
#define MMU_L1_RWRO MMU_L1_AP(MMU_AP_RWRO)
#define MMU_L1_RWRW MMU_L1_AP(MMU_AP_RWRW)
#define MMU_L1_RONA (MMU_L1_AP(MMU_AP_RWNA) | MMU_L1_APX)
#define MMU_L1_RORO (MMU_L1_AP(MMU_AP_RWRO) | MMU_L1_APX)

// L2 access permissions
#define MMU_L2_NANA MMU_L2_AP(MMU_AP_NANA)
#define MMU_L2_RWNA MMU_L2_AP(MMU_AP_RWNA)
#define MMU_L2_RWRO MMU_L2_AP(MMU_AP_RWRO)
#define MMU_L2_RWRW MMU_L2_AP(MMU_AP_RWRW)
#define MMU_L2_RONA (MMU_L2_AP(MMU_AP_RWNA) | MMU_L2_APX)
#define MMU_L2_RORO (MMU_L2_AP(MMU_AP_RWRO) | MMU_L2_APX)

// Access permission masks
#define MMU_L1_PMASK (MMU_L1_APX | MMU_L1_AP(3) | MMU_L1_XN)
#define MMU_L2_4K_PMASK (MMU_L2_APX | MMU_L2_AP(3) | MMU_L2_XN_4K)
#define MMU_L2_64K_PMASK (MMU_L2_APX | MMU_L2_AP(3) | MMU_L2_XN_64K)

#define L1_INDEX(addr) ((u32)(addr) >> 20)
#define L2_INDEX(addr) (((u32)(addr) >> 12) & 0xFF)

extern u32 g_totalMem, g_usableMem, g_physBase;

static inline u32 MemGetPhysicalBase(void)
{
	return g_physBase;
}

// KEEP THIS STRUCTURE SIZED TO A POWER OF TWO!
struct tag_pageinfo
{
	pageinfo_t* next;
	pageinfo_t* prev; // currently unused
	u32 refCount;
	u32 flags;
};

#define PAGEFLAG_STICKY    BIT(31) // Disable freeing
#define PAGEFLAG_HASCOLOUR BIT(30) // Page has a colouring restriction
#define PAGEFLAG_COLOURMASK 3

static inline u32 MemPageIncrRef(pageinfo_t* page)
{
	return AtomicIncrement(&page->refCount);
}

static inline u32 MemPageDecrRef(pageinfo_t* page)
{
	return AtomicDecrement(&page->refCount);
}

static inline void* safe_phys2virt(u32 x)
{
	if (x >= g_physBase)
		return phys2virt(x);
	extern u32 __pagetables[];
	x -= (u32)__pagetables;
	return (u8*)MASTER_PAGETABLE + x;
}

#define PAGEMAP ((pageinfo_t*)0x80000000)

static inline pageinfo_t* vphys2page(void* x)
{
	return PAGEMAP + (((u8*)x - PHYSICAL_MEMORY)>>12);
}

static inline void* page2vphys(pageinfo_t* p)
{
	return PHYSICAL_MEMORY + ((p - PAGEMAP)<<12);
}

// (Internal) Initializes the virtual memory address map system.
void MemInit(u32 memSize, u32 initRdAddr, u32 initRdSize);
