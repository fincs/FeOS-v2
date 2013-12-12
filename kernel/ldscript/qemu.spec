
/* Spec for QEMU versatilepb machine */

MEMORY
{
	loadmem : ORIGIN = 0x00010000, LENGTH = 4K
}

__pagetables = 0x00000000;
__mainmem_addr = 0x00000000;
__devmem_addr = 0x10000000;
__devmem_size = 0x200000;
