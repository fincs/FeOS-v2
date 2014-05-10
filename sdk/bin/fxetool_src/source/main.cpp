#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>
#include <algorithm>
#include "elf.h"
#include "fxe.h"
#include "FileClass.h"

using std::vector;
using std::map;

#define die(msg) do { fputs(msg "\n\n", stderr); return 1; } while(0)
#define safe_call(a) do { int rc = a; if(rc != 0) return rc; } while(0)

#ifdef WIN32
static inline void FixMinGWPath(char* buf)
{
	if (*buf == '/')
	{
		buf[0] = buf[1];
		buf[1] = ':';
	}
}
#endif

struct FxeSectConv
{
	u32 fileOff, flags, memSize, fileSize, memPos;
};

struct FxeSymConv
{
	const char* name;
	u32 addr;
	bool isFunc;

	inline FxeSymConv(const char* n, u32 a, bool i) : name(n), addr(a), isFunc(i) { }
};

class FxeConvert
{
	FileClass fout;
	byte_t* img;
	int platFlags;
	bool isRelocatable;

	Elf32_Shdr* elfSects;
	int elfSectCount;
	const char* elfSectNames;

	Elf32_Sym* elfSyms;
	int elfSymCount;
	const char* elfSymNames;

	bool hasExidx;
	u32 exidxAddr, exidxCount;

	vector<FxeSectConv> fxeSects;
	u32 baseAddr, topAddr;

	vector<bool> relocMap;
	vector<FXEReloc> relocData;
	map<u32,u32> fixupData;

	vector<FxeSymConv> expTable;
	size_t expNameSize;

	vector<FxeSymConv> impTable;
	size_t impNameSize;
	const char* lastImpSect;

	int ScanSections();

	int ScanRelocSection(u32 vsect, byte_t* sectData, Elf32_Sym* symTab, Elf32_Rel* relTab, int relCount);
	int ScanRelocations();
	int ScanSymbols();

	int WriteSymTable(const vector<FxeSymConv>& table, size_t nameSize, bool print);

	void SetReloc(u32 address)
	{
		address -= baseAddr;
		if (address >= relocMap.size()) return;
		relocMap[address / 4] = true;
	}

public:
	FxeConvert(const char* f, byte_t* i, int x)
		: fout(f, "wb"), img(i), platFlags(x), isRelocatable(false), elfSyms(nullptr), hasExidx(false)
		, relocMap(), relocData(), fixupData(), expTable(), expNameSize(0), impTable(), impNameSize(0), lastImpSect(nullptr)
	{
	}
	int Convert();
};

int FxeConvert::ScanRelocSection(u32 vsect, byte_t* sectData, Elf32_Sym* symTab, Elf32_Rel* relTab, int relCount)
{
	for (int i = 0; i < relCount; i ++)
	{
		auto rel = relTab + i;
		u32 relInfo = le_word(rel->r_info);
		int relType = ELF32_R_TYPE(relInfo);
		auto relSym = symTab + ELF32_R_SYM(relInfo);
		auto relSymSect = le_hword(relSym->st_shndx);
		Elf32_Shdr* symSect = nullptr;
		if ( !(relSymSect >= SHN_LORESERVE && relSymSect <= SHN_HIRESERVE) )
			symSect = elfSects + relSymSect;

		u32 relSymAddr = le_word(relSym->st_value);
		u32 relSrcAddr = le_word(rel->r_offset);
		auto& relSrc = *(u32*)(sectData + relSrcAddr - vsect);

		const char* symName = elfSymNames + le_word(relSym->st_name);

		if (symSect && strncmp(elfSectNames + le_word(symSect->sh_name), ".imp.", 5) == 0 // It's an import section
			&& ELF32_ST_TYPE(le_word(relSym->st_info)) != STT_FUNC // The target symbol is NOT a function
			&& *symName && strncmp(symName, "__imp_", 6) != 0) // It's *not* a direct import
		{
			// Fixup required!
#ifdef DEBUG
			fprintf(stderr, "Fixup for %s at %08X required!\n", symName, relSrcAddr);
#endif

			u32 relocOff = 0;
			if (relType == R_ARM_ABS32 || relType == R_ARM_TARGET1)
			{
				relocOff = (u32)((int)relSrc - (int)relSymAddr);
				relSrc = 0;
			} else if (relType == R_ARM_TARGET2 || relType == R_ARM_REL32)
			{
				relocOff = (u32)((int)relSrc - ((int)relSymAddr - (int)relSrcAddr));
				relSrc = 1;
			} else
			{
				fprintf(stderr, "%d %s\n", relType, symName);
				die("Unsupported type of non-function import relocation!");
			}

			u32 temp = relocOff >> 24;
			if (temp != 0 && temp != 0xF)
				die("Offset from non-function import target is too big!");

			// Add fixup
			relSrc |= relocOff << 8;
			fixupData[relSrcAddr] = relSymAddr;
			continue;
		}

		if (!isRelocatable)
			continue;

		switch (relType)
		{
			// Notes:
			// R_ARM_TARGET2 is equivalent to R_ARM_REL32
			// R_ARM_PREL32 is an address-relative signed 31-bit offset

			case R_ARM_ABS32:
			case R_ARM_TARGET1:
			{
				if(relSrcAddr & 3)
					die("Unaligned relocation!");

				// Ignore unbound weak symbols (keep them 0)
				if (ELF32_ST_BIND(le_word(relSym->st_info)) == STB_WEAK && relSymAddr == 0) break;

				// Add relocation
				SetReloc(relSrcAddr);
				break;
			}
		}
	}
	return 0;
}

int FxeConvert::ScanRelocations()
{
	for (int i = 0; i < elfSectCount; i ++)
	{
		auto sect = elfSects + i;
		auto sectType = le_word(sect->sh_type);
		if (sectType == SHT_RELA)
			die("Unsupported relocation section");
		else if (sectType != SHT_REL)
			continue;

		auto targetSect = elfSects + le_word(sect->sh_info);
		u32 vsect = le_word(targetSect->sh_addr);
		auto sectData = img + le_word(targetSect->sh_offset);

		auto symTab = (Elf32_Sym*)(img + le_word(elfSects[le_word(sect->sh_link)].sh_offset));
		auto relTab = (Elf32_Rel*)(img + le_word(sect->sh_offset));
		int relCount = (int)(le_word(sect->sh_size) / le_word(sect->sh_entsize));

		safe_call(ScanRelocSection(vsect, sectData, symTab, relTab, relCount));
	}

	if (!isRelocatable)
		return 0; // Return early

	// Scan for interworking thunks that need to be relocated
	for (int i = 0; i < elfSymCount; i ++)
	{
		auto sym = elfSyms + i;
		auto symName = (const char*)(elfSymNames + le_word(sym->st_name));
		if (!*symName) continue;
		if (symName[0] != '_' && symName[1] != '_') continue;
		if (strncmp(symName+strlen(symName)-9, "_from_arm", 9) != 0) continue;
		SetReloc(le_word(sym->st_value));
	}

	// Create relocation table
	int rmSize = relocMap.size();
	for (int i = 0; i < rmSize; i ++)
	{
		FXEReloc reloc;
		u32 rs = 0, rp = 0;
		while ((i < rmSize) && !relocMap[i]) i ++, rs ++;
		while ((i < rmSize) && relocMap[i]) i ++, rp ++;

		// Remove empty trailing relocations
		if (i == rmSize && rs && !rp)
			break;

		// Add excess skip relocations
		for (reloc.skip = 0xFFFF, reloc.patch = 0; rs > 0xFFFF; rs -= 0xFFFF)
			relocData.push_back(reloc);

		// Add excess patch relocations
		for (reloc.skip = rs, reloc.patch = 0xFFFF; rp > 0xFFFF; rp -= 0xFFFF)
		{
			relocData.push_back(reloc);
			rs = reloc.skip = 0;
		}

		// Add remaining relocation
		if (rs || rp)
		{
			reloc.skip = rs;
			reloc.patch = rp;
			relocData.push_back(reloc);
		}
	}

	return 0;
}

int FxeConvert::ScanSections()
{
	for (int i = 0; i < elfSectCount; i ++)
	{
		auto sect = elfSects + i;
		//auto sectName = elfSectNames + le_word(sect->sh_name);
		switch (le_word(sect->sh_type))
		{
			case SHT_ARM_EXIDX:
#ifdef DEBUG
				fprintf(stderr, "Hai thar, found the EXIDX section.\n");
#endif
				hasExidx = true;
				exidxAddr = le_word(sect->sh_addr);
				exidxCount = le_word(sect->sh_size) / 8;
				break;
			case SHT_SYMTAB:
				elfSyms = (Elf32_Sym*) (img + le_word(sect->sh_offset));
				elfSymCount = le_word(sect->sh_size) / sizeof(Elf32_Sym);
				elfSymNames = (const char*)(img + le_word(elfSects[le_word(sect->sh_link)].sh_offset));
				break;
		}
	}

	if (!elfSyms)
		die("ELF has no symbol table!");

	return 0;
}

int FxeConvert::ScanSymbols()
{
	// Sort symbol table by address
	std::sort(elfSyms, elfSyms + elfSymCount, [](const Elf32_Sym& a, const Elf32_Sym& b)
	{
		return le_word(a.st_value) < le_word(b.st_value);
	});

	for (int i = 0; i < elfSymCount; i ++)
	{
		auto sym = elfSyms + i;
		auto symInfo = le_word(sym->st_info);
		if (ELF32_ST_BIND(symInfo) == STB_LOCAL) continue; // Ignore local symbols
		if (ELF32_ST_BIND(symInfo) == STB_WEAK && !sym->st_shndx) continue; // Ignore undefined weak symbols
		auto symName = (const char*)(elfSymNames + le_word(sym->st_name));
		if (!*symName) continue;

		if (le_word(sym->st_other) != STV_DEFAULT)
		{
			// This may be an import...

			if (strncmp(symName, "__imp_", 6) != 0) continue; // Bad
			symName += 6;
			if (!*symName) continue; // Bad

			auto sectName = elfSectNames + le_word(elfSects[le_hword(sym->st_shndx)].sh_name);
			if (strncmp(sectName, ".imp.", 5) != 0) continue; // Bad
			sectName += 5;

			if (lastImpSect != sectName)
			{
				impTable.emplace_back(sectName, ~0, false);
				impNameSize += strlen(sectName)+1;
				lastImpSect = sectName;
			}

			impTable.emplace_back(symName, le_word(sym->st_value), false);
			impNameSize += strlen(symName)+1;

			continue; // NOTIMPL
		}

		// Filter out internal symbols
		if (symName[0] == '_' && symName[1] == '_' && (
			strncmp(symName+2, "feos_", 5) == 0 ||
			strcmp(symName+2, "start__") == 0 ||
			strcmp(symName+2, "modulebase") == 0 ||
			strcmp(symName+2, "end__") == 0
			))
			continue;

		bool isFunc = ELF32_ST_TYPE(symInfo) == STT_FUNC;
#ifdef DEBUG
		fprintf(stderr, "Found symbol: %s\n", symName);
#endif
		expTable.emplace_back(symName, le_word(sym->st_value), isFunc);
		expNameSize += strlen(symName)+1;
	}

	// Sort the export table by name
	std::sort(expTable.begin(), expTable.end(), [](const FxeSymConv& a, const FxeSymConv& b)
	{
		return strcmp(a.name, b.name) < 0;
	});

	return 0;
}

int FxeConvert::WriteSymTable(const vector<FxeSymConv>& table, size_t nameSize, bool print)
{
	size_t symCount = table.size();
	size_t curOff = symCount * 8;
	fout.WriteWord(symCount);
	fout.WriteWord(curOff + nameSize);
	for (auto& sym : table)
	{
		if (print)
			printf("%s%s\n", sym.name, sym.isFunc ? "" : " OBJ");
		fout.WriteWord(curOff);
		fout.WriteWord(sym.addr);
		curOff += strlen(sym.name)+1;
	}
	for (auto& sym : table)
		fout.WriteRaw(sym.name, strlen(sym.name)+1);
	return 0;
}

int FxeConvert::Convert()
{
	if (fout.openerror())
		die("Cannot open output file!");

	auto ehdr = (Elf32_Ehdr*) img;
	if(memcmp(ehdr->e_ident, ELF_MAGIC, 4) != 0)
		die("Invalid ELF file!");
	if(le_hword(ehdr->e_type) != ET_EXEC)
		die("ELF file must be executable! (hdr->e_type should be ET_EXEC)");

	elfSects = (Elf32_Shdr*)(img + le_word(ehdr->e_shoff));
	elfSectCount = (int)le_hword(ehdr->e_shnum);
	elfSectNames = (const char*)(img + le_word(elfSects[le_hword(ehdr->e_shstrndx)].sh_offset));

	auto phdr = (Elf32_Phdr*)(img + le_word(ehdr->e_phoff));
	baseAddr = 1, topAddr = 0;
	for (int i = 0; i < ehdr->e_phnum; i ++)
	{
		auto cur = phdr + i;
		FxeSectConv s;
		s.fileOff = le_word(cur->p_offset);
		s.flags = le_word(cur->p_flags);
		s.memSize = le_word(cur->p_memsz);
		s.fileSize = le_word(cur->p_filesz);
		s.memPos = le_word(cur->p_vaddr);

		if (!s.memSize) continue;

#ifdef DEBUG
		fprintf(stderr, "PHDR[%d]: fOff(%X) memPos(%08X) memSize(%u) fileSize(%u) flags(%08X)\n",
			i, s.fileOff, s.memPos, s.memSize, s.fileSize, s.flags);
#endif

		if (i == 0) baseAddr = s.memPos;
		else if (s.memPos != topAddr) die("Non-contiguous section!");

		if (s.memSize & 3) die("The section is not word-aligned!");
		if (s.fileSize & 3) die("The loadable part of the section is not word-aligned!");

		topAddr = s.memPos + ((s.memSize + 0xFFF) &~ 0xFFF);
		fxeSects.push_back(s);
	}

	if (baseAddr == 0)
		isRelocatable = true;
	else if (baseAddr != 0x10000)
		die("Invalid executable base address!");

	u32 entrypoint = le_word(ehdr->e_entry);
	if (entrypoint < baseAddr || entrypoint >= topAddr)
		die("Entrypoint out of bounds!");

	if (isRelocatable)
		// Create relocation bitmap
		relocMap.assign((topAddr - baseAddr) / 4, false);

	safe_call(ScanSections());
	safe_call(ScanRelocations()); // even for non-relocatable modules due to fixups
	safe_call(ScanSymbols());

	u32 fxeFlags = (platFlags & 0xFFF) | (fxeSects.size() << FXEFlags_SectCountShift);
	if (isRelocatable)
		fxeFlags |= FXEFlags_HasRelocs;
	if (hasExidx)
		fxeFlags |= FXEFlags_HasExidx;
	if (!impTable.empty())
		fxeFlags |= FXEFlags_HasImports;
	if (!fixupData.empty())
		fxeFlags |= FXEFlags_HasFixups;
	if (!expTable.empty())
		fxeFlags |= FXEFlags_HasExports;

	// Write header
	fout.WriteWord(FXE_MAGIC);
	fout.WriteWord(fxeFlags);
	fout.WriteHword(2); // OS Major Ver
	fout.WriteHword(0); // OS Minor Ver
	fout.WriteWord(entrypoint);

	// Write section headers
	for (auto& sect : fxeSects)
	{
		char name[6];
		memset(name, 0, sizeof(name));
		u16 sectFlags = 0;
		if (sect.flags & PF_W)
			sectFlags |= FXESectFlag_Writable;
		if (sect.flags & PF_X)
			sectFlags |= FXESectFlag_Executable;
		if (sect.flags & PF_OS_SHARED)
			sectFlags |= FXESectFlag_Shared;
		switch (sectFlags)
		{
			case FXESectFlag_Shared | FXESectFlag_Executable:
				memcpy(name, "CODE", 4);
				break;
			case FXESectFlag_Shared:
				memcpy(name, "RODATA", 6);
				break;
			case FXESectFlag_Shared | FXESectFlag_Writable:
				memcpy(name, "SHDATA", 6);
				break;
			case FXESectFlag_Writable:
				memcpy(name, "DATA", 4);
				break;
			default:
				memcpy(name, "WEIRD", 5);
				break;
		}
		fout.WriteRaw(name, sizeof(name));
		fout.WriteHword(sectFlags);
		fout.WriteWord(sect.memSize);
		fout.WriteWord(sect.fileSize);
	}

	// Write sections
	for (auto& sect : fxeSects)
		fout.WriteRaw(img + sect.fileOff, sect.fileSize);

	// Write relocations
	if (isRelocatable)
	{
		fout.WriteWord((word_t)relocData.size());
		for (auto& reloc : relocData)
		{
#ifdef DEBUG
			fprintf(stderr, "RELOC {skip: %d, patch: %d}\n", (int)reloc.skip, (int)reloc.patch);
#endif
			fout.WriteHword(reloc.skip);
			fout.WriteHword(reloc.patch);
		}
	}

	// Write exidx table information
	if (hasExidx)
	{
		fout.WriteWord(exidxAddr);
		fout.WriteWord(exidxCount);
	}

	// Write import table
	if (!impTable.empty())
		safe_call(WriteSymTable(impTable, impNameSize, false));

	// Write fixup data
	if (!fixupData.empty())
	{
		fout.WriteWord((word_t)fixupData.size());
		for (auto& pair : fixupData)
		{
			fout.WriteWord(pair.second);
			fout.WriteWord(pair.first);
		}
	}

	// Write export table
	if (!expTable.empty())
		safe_call(WriteSymTable(expTable, expNameSize, true));

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage:\n\t%s [inputFile] [outputFile] [platform]\n", argv[0]);
		return 1;
	}

#ifdef WIN32
	FixMinGWPath(argv[1]);
	FixMinGWPath(argv[2]);
#endif

	int platform = strtol(argv[3], nullptr, 0);

	auto elf_file = fopen(argv[1], "rb");
	if (!elf_file) die("Cannot open input file!");

	fseek(elf_file, 0, SEEK_END);
	size_t elfSize = ftell(elf_file);
	rewind(elf_file);

	auto b = (byte_t*) malloc(elfSize);
	if (!b) { fclose(elf_file); die("Cannot allocate memory!"); }

	fread(b, 1, elfSize, elf_file);
	fclose(elf_file);

	int rc = 0;
	{
		FxeConvert cnv(argv[2], b, platform);
		rc = cnv.Convert();
	}
	free(b);

	if (rc != 0)
		remove(argv[2]);

	return rc;
}
