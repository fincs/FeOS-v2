# FeOS Executable Format (FXE3)

## File structure

The data is laid out in the following order inside the file:

- File header     (`FXEHeader`)
- Section headers (`SectionCount*FXESection`)
- Section data    (length: sum each section's `loadSize` together)
- Relocation data (if present: `word_t(relocCount) + relocCount*FXEReloc`)
- Exidx table     (if present: `FXEExidx`)
- Import table    (if present: `FXESymHdr + entryCount*FXESym`)
- Fixup table     (if present: `word_t(fixupCount) + fixupCount*FXEFixup`)
- Export table    (if present: same as imports)

## File header

    struct FXEHeader
    {
        u32 magic; // 'FXE0'
        u32 flags; // see FXEFlags below
        u16 osMajVer, osMinVer;
        u32 entrypoint;
    };
    
    enum FXEFlags
    {
        Platform_Mask = 0xFFF,
        Platform_Generic  = 0x000,// Generic ARMv6 machine (i.e. no specific platform)
        Platform_Emulator = 0x001,// Generic emulator (i.e. QEMU)
        Platform_RPi = 0x314,     // Raspberry RPi (ARM1176JZF-S)
        Platform_3DS = 0x3D5,     // Nintendo 3DS (ARM11MPCore)
        //...
        
        SectionCount_Mask = 0xFF000,
        
        HasRelocs  = BIT(20), // if set = shared lib, if not set = executable loaded at 0x10000
        HasExidx   = BIT(21), // exception index table used for unwinding and exception recovery
        HasImports = BIT(22), // import table
        HasFixups  = BIT(23), // data import address fixups
        HasExports = BIT(24), // export table
    };

## Sections

All sections are laid out in memory with 4KB alignment, starting from the module base.

In the file, the header is immediately followed by an array of section headers;
then the raw section data in order.

    struct FXESection
    {
        char name[6]; // null-padded
        u16 flags; // see FXESectionFlags
        u32 sectSize; // Bits 0 and 1 must be zero
        u32 loadSize; // As above. loadSize is always <= sectSize, uninitialized part filled with zeroes (BSS)
    };
    
    enum FXESectionFlags
    {
        Executable = BIT(0), // sets XN (eXecute-Never) bit to zero
        Writable   = BIT(1), // allows write operations
        Shared     = BIT(2), // shared amongst processes
    };

Standard section names:

- `CODE`   (Executable, Shared)
- `RODATA` (Shared)
- `DATA`   (Writable)
- `SHDATA` (Writable, Shared)

Sections that are not shared are implemented using Copy-On-Write in order to save memory.

## Relocation data

Each relocation has the following format:

    struct FXEReloc
    {
        u16 skip, patch;
    };

The relocations are processed sequentially, with the aid of a current patching
position pointer that is initialized to the module load address (coinciding with
the start of the code section). The `skip` field indicates how many 32-bit words
to skip from the current patching position until the first word to patch (i.e.
add the module load address to it), while the `patch` field indicates how many
words to patch. This system is identical to that of the FXE2 format used by FeOS/DS.

## Exception index data

If the module has the `HasExidx` flag set, the following structure is appended to
the file:

    struct FXEExidx
    {
        u32 tableAddress;
        u32 entryCount;
    };

This structure defines the location in memory (relative to the module base) and the
number of entries of the Exception Index table, required for implementing stack unwinding
and structured exception handling.

## Imports and exports

    struct FXESymHdr
    {
        u32 count, size;
    };

    struct FXESym
    {
        u32 nameOffset;
        u32 address;
    };

TODO: Describe

## Fixup data

    struct FXEFixup
    {
        u32 sourceAddr;
        u32 targetAddr;
    };

TODO: Describe
