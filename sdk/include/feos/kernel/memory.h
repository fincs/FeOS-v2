#pragma once
#include "feos.h"

#define PHYSICAL_MEMORY ((u8*)0x80000000)
#define PAGE_COLOUR(addr) (((u32)(addr) >> 12) & 3)

enum
{
	PAGE_R = 0,
	PAGE_W = 1,
	PAGE_X = 2,
};

_STATIC u32 MemGetPhysicalBase(void);

static inline void* phys2virt(u32 x)
{
	return PHYSICAL_MEMORY + x - MemGetPhysicalBase();
}

static inline u32 virt2phys(void* x)
{
	return MemGetPhysicalBase() + (u8*)x - PHYSICAL_MEMORY;
}

_STATIC void* safe_phys2virt(u32 x);

typedef struct tag_pageinfo pageinfo_t;

_STATIC pageinfo_t* vphys2page(void* x);
_STATIC void* page2vphys(pageinfo_t* p);

enum
{
	PAGEORDER_4K = 0,
	PAGEORDER_8K,
	PAGEORDER_16K,
	PAGEORDER_32K,
	PAGEORDER_64K,
	PAGEORDER_128K,
	PAGEORDER_256K,
	PAGEORDER_512K,
	PAGEORDER_1M,
};

// Allocation and deallocation of pages.
pageinfo_t* MemAllocPage(int order);
void MemFreePage(pageinfo_t* page, int order);

// Allocates a new page and maps it to the specified address.
void* MemMapPage(void* address, int flags);
// Sets the permissions of a page.
bool MemProtectPage(void* address, int flags);
// Frees a page previously mapped by MemMapPage.
bool MemUnmapPage(void* address);

// Translates a virtual address to a physical address. ~0 = failure
u32 MemTranslateAddr(void* address);
