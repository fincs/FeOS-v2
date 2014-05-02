#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

// Maps pages into the VM address space of a process at the specified address.
// proc  - The handle of the process to manipulate. If NULL, the current process is manipulated.
//         This handle is ignored when the address ís a kernel address, in which case the kernel
//         VM address space is manipulated instead.
// addr  - The address at which to (sequentially) map the pages; which is forcefully aligned down
//         to a page boundary.
// size  - The size of the memory region to map (in bytes); which is forcefully rounded up to
//         a multiple of the page size.
// flags - The access permissions (a combination of PAGE_R, PAGE_W and PAGE_X).
//         Kernel-space addresses are never readable by user mode.
// pages - An array of handles to pages to map. If NULL, new pages are allocated.
//         If the array is specified, there *MUST* be enough pages in it to cover the whole
//         specified memory region, otherwise the function exhibits undefined behaviour.
//         The kernel takes ownership of all pages that are successfully mapped.
// Returns:
//   The number of successfully mapped pages times the page size until the first error occured.
size_t MemVmMap(processInfo* proc, void* addr, size_t size, int flags, pageinfo_t* pages[]);

// Changes the access permissions for a memory region inside the VM address space of a process.
// proc  - The handle of the process to manipulate. If NULL, the current process is manipulated.
//         This handle is ignored when the address ís a kernel address, in which case the kernel
//         VM address space is manipulated instead.
// addr  - The start address of the memory region whose access permissions to change; which is
//         forcefully aligned down to a page boundary.
// size  - The size of the memory region whose access permissions to change (in bytes); which is
//         forcefully rounded up to a multiple of the page size.
// flags - The access permissions to apply (a combination of PAGE_R, PAGE_W and PAGE_X).
//         Kernel-space addresses are never readable by user mode.
// Returns:
//   The number of successfully re-protected pages times the page size until the first error occured.
size_t MemVmProtect(processInfo* proc, void* addr, size_t size, int flags);

// Unmaps a memory region from the VM address space of a process.
// proc  - The handle of the process to manipulate. If NULL, the current process is manipulated.
//         This handle is ignored when the address ís a kernel address, in which case the kernel
//         VM address space is manipulated instead.
// addr  - The start address of the memory region to unmap; which is forcefully aligned down to
//         a page boundary.
// size  - The size of the memory region to unmap (in bytes); which is forcefully rounded up to
//         a multiple of the page size.
// Returns:
//   The number of successfully unmapped pages times the page size until the first error occured.
size_t MemVmUnmap(processInfo* proc, void* addr, size_t size);

// Translates a virtual address to a physical address. ~0 = failure
u32 MemTranslateAddr(void* address);

static inline void* MemMapPage(void* vaddr, int flags)
{
	return MemVmMap(nullptr, vaddr, 0x1000, flags, nullptr) ? vaddr : nullptr;
}

static inline bool MemProtectPage(void* vaddr, int flags)
{
	return MemVmProtect(nullptr, vaddr, 0x1000, flags) != 0;
}

static inline bool MemUnmapPage(void* vaddr)
{
	return MemVmUnmap(nullptr, vaddr, 0x1000) != 0;
}
