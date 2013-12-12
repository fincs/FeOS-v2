
/* Spec for Raspberry Pi (BCM2835) */

MEMORY
{
	loadmem : ORIGIN = 0x00008000, LENGTH = 4K
}

__pagetables = 0x00000000;
__mainmem_addr = 0x00000000;
__devmem_addr = 0x20000000;
__devmem_size = 0x1000000;
