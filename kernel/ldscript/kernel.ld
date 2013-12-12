OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__entrypoint)

MEMORY
{
	kmem : ORIGIN = 0xe0000000, LENGTH = 1M
	vectors : ORIGIN = 0xFFFF0000, LENGTH = 4K
}

__load_addr = ORIGIN(loadmem);
__kmem_start = ORIGIN(kmem);
__kmem_top   = ORIGIN(kmem) + LENGTH(kmem);

SECTIONS
{
	.init :
	{
		KEEP (*(.init))
		*(.init2)
		*(.initd)
	} >loadmem

	__lma_code = ALIGN(4K);

	.text __kmem_start : AT(__lma_code)
	{
		*(.text*)
		*(.stub)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		. = ALIGN(4);

		/* Constructor table */
		__feos_init_array = ABSOLUTE(.);
		KEEP( *(SORT(.preinit_array.*)) )
		KEEP( *(.preinit_array) )
		KEEP( *(SORT(.init_array.*)) )
		KEEP( *(.init_array) )
		KEEP( *(SORT(.ctors.*)) )
		KEEP( *(.ctors) )
		__feos_init_size = ABSOLUTE(.);
		LONG((__feos_init_size - __feos_init_array) / 4);

		/* Destructor table */
		__feos_fini_array = ABSOLUTE(.);
		KEEP( *(SORT(.dtors.*)) )
		KEEP( *(.dtors) )
		KEEP( *(SORT(.fini_array.*)) )
		KEEP( *(.fini_array) )
		__feos_fini_size = ABSOLUTE(.);
		LONG((__feos_fini_size - __feos_fini_array) / 4);
		. = ALIGN(4K);
	} >kmem

	__lma_code_size = SIZEOF(.text);
	__lma_vectors = __lma_code + SIZEOF(.text);
	__backup1 = .;

	.vectors 0xFFFF0000 : AT(__lma_vectors)
	{
		KEEP (*(.vectors))
		. = ALIGN(4K);
	} >vectors

	__lma_rodata = __lma_vectors + SIZEOF(.vectors);

	ARM.exidx __backup1 : AT(__lma_rodata)
	{
		__exidx_start = ABSOLUTE(.);
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = ABSOLUTE(.);
	} >kmem

	.rodata :
	{
		. = ALIGN(4);

		*(.rodata)
		*(.roda)
		*(.rodata.*)
		*all.rodata*(*)
		*(.gnu.linkonce.r*)
		SORT(CONSTRUCTORS)
		. = ALIGN(4);

		*(.ARM.extab* .gnu.linkonce.armextab.*)

		. = ALIGN(4K);
	} >kmem

	__lma_rodata_size = SIZEOF(ARM.exidx) + SIZEOF(.rodata);
	__lma_data = __lma_rodata + __lma_rodata_size;

	.data :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		. = ALIGN(4);
	} >kmem

	.bss (NOLOAD) :
	{
		__bss_start = ABSOLUTE(.);

		*(.dynbss)
		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b*)
		*(COMMON)
		. = ALIGN(4);

		__bss_size = ABSOLUTE(.) - __bss_start;

		. = ALIGN(4K);

		__kmem_end = ABSOLUTE(.);
	} >kmem

	__lma_data_size = SIZEOF(.data) + SIZEOF(.bss);

	/* Discard sections that difficult fxe2tool post-processing */
	/DISCARD/ : { *(.group /*.comment*/ .note) }

	/* Stabs debugging sections. */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }

	/* DWARF debug sections.
	   Symbols in the DWARF debugging sections are relative to the beginning
	   of the section so we begin them at 0. */

	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }

	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }

	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }

	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
}
