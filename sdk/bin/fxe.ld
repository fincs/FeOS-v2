OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

PHDRS
{
	code   PT_LOAD FLAGS(0x100005) /* Shared | Read | Execute */;
	rodata PT_LOAD FLAGS(0x100004) /* Shared | Read */;
	shared PT_LOAD FLAGS(0x100006) /* Shared | Read | Write */;
	data   PT_LOAD FLAGS(0x6)      /* Read | Write */;
}

SECTIONS
{
	/* =========== CODE section =========== */

	. = __modulebase;
	__start__ = . ;

	.text ALIGN(4) :
	{
		/* .init */
		KEEP( *(.init) )
		. = ALIGN(4);

		/* .text */
		*(.text)
		*(.text.*)
		/* *(.imp.*) */ /* Let ld do it for us and preserve section names */
		*(.glue_7)
		*(.glue_7t)
		*(.stub)
		*(.gnu.warning)
		*(.gnu.linkonce.t*)
		. = ALIGN(4);
	} : code

	/* =========== RODATA section =========== */

	. = ALIGN(0x1000);

	.rodata ALIGN(4) :
	{
		*(.rodata)
		*(.roda)
		*(.rodata.*)
		*all.rodata*(*)
		*(.gnu.linkonce.r*)
		SORT(CONSTRUCTORS)
		. = ALIGN(4);
	} : rodata

	.ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } : rodata
	ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } : rodata

	.ctor_table ALIGN(4) :
	{
		__feos_init_array = . ;
		KEEP( *(SORT(.preinit_array.*)) )
		KEEP( *(.preinit_array) )
		KEEP( *(SORT(.init_array.*)) )
		KEEP( *(.init_array) )
		KEEP( *(SORT(.ctors.*)) )
		KEEP( *(.ctors) )
		__feos_init_array_end = . ;
	} : rodata

	.dtor_table ALIGN(4) :
	{
		__feos_fini_array = . ;
		KEEP( *(SORT(.dtors.*)) )
		KEEP( *(.dtors) )
		KEEP( *(SORT(.fini_array.*)) )
		KEEP( *(.fini_array) )
		__feos_fini_array_end = . ;
	} : rodata

	/* =========== SHARED section =========== */

	. = ALIGN(0x1000);

	.sdata ALIGN(4) :
	{
		*(.sdata)
		. = ALIGN(4);
	} : shared

	.sbss ALIGN(4) :
	{
		*(.sbss)
		. = ALIGN(4);
	} : shared

	/* =========== DATA section =========== */

	. = ALIGN(0x1000);

	.data ALIGN(4) :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		CONSTRUCTORS
		. = ALIGN(4);
	} : data

	.bss ALIGN(4) :
	{
		*(.dynbss)
		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b*)
		*(COMMON)
		. = ALIGN(4);
	} : data

	__end__ = ABSOLUTE(.) ;

	/* ==================
	   ==== Metadata ====
	   ================== */

	/* Discard sections that difficult post-processing */
	/DISCARD/ : { *(.group .comment .note) }

	/* Stabs debugging sections. */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }

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
