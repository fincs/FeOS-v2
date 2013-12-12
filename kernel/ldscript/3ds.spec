
/* Spec for Nintendo 3DS */

MEMORY
{
	loadmem : ORIGIN = 0x1FF88000, LENGTH = 4K
}

__pagetables = 0x1FF80000;
__mainmem_addr = 0x1FF80000;
__devmem_addr = 0x10000000;
__devmem_size = 0xFF80000;
