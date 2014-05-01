#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

// Allocates a new page and maps it to the specified address.
void* MemMapPage(void* address, int flags);
// Sets the permissions of a page.
bool MemProtectPage(void* address, int flags);
// Frees a page previously mapped by MemMapPage.
bool MemUnmapPage(void* address);

// Translates a virtual address to a physical address. ~0 = failure
u32 MemTranslateAddr(void* address);
