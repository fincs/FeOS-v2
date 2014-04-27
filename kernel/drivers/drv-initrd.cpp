#include "common.h"
#include "tinfl.h"
#include "far.h"

static byte_t* farData;
static size_t farSize;

static inline hword_t readLEHWord(byte_t* pos)
{
	return (hword_t)pos[0] | ((hword_t)pos[1] << 8);
}

static inline word_t readLEWord(byte_t* pos)
{
	return (word_t)pos[0] |
		((word_t)pos[1] << 8) |
		((word_t)pos[2] << 16) |
		((word_t)pos[3] << 24);
}

static int ModuleAttach()
{
	if (!g_initRd)
	{
		kprintf("<drv-initrd> No initrd available!\n");
		return -ENOENT;
	}

	byte_t* initRd = (byte_t*)g_initRd;
	if (initRd[0] != 0x1F || initRd[1] != 0x8B || initRd[2] != 8)
	{
		kprintf("<drv-initrd> Invalid initrd format!\n");
		return -EINVAL;
	}

	byte_t* initRdEnd = initRd + g_initRdSize;
	farSize = readLEWord(initRdEnd - 4);

	initRd += 3;
	int flags = *initRd;
	initRd += 7;

	if (flags & 4)
		initRd += 2 + readLEHWord(initRd);
	if (flags & 8)
		while(*initRd++);
	if (flags & 16)
		while(*initRd++);
	if (flags & 2)
		initRd += 2;

	size_t inSize = initRdEnd - initRd - 8;
	size_t outSize = farSize > TINFL_LZ_DICT_SIZE ? farSize : TINFL_LZ_DICT_SIZE;
	farData = (byte_t*)malloc(outSize);
	if (!farData)
	{
		kprintf("<drv-initrd> Cannot allocate memory!\n");
		return -ENOMEM;
	}
	
	kprintf("<drv-initrd> Decompressing initrd...\n");
	tinfl_decompressor inflator;
	tinfl_init(&inflator);
	tinfl_status st = tinfl_decompress(&inflator, initRd, &inSize, farData, farData, &outSize, 0);
	if (st != TINFL_STATUS_DONE)
	{
		free(farData);
		farData = nullptr;
		kprintf("<drv-initrd> Decompression error (%d)!\n", st);
		return -EINVAL;
	}

	kprintf("<drv-initrd> initrd successfully decompressed (%u bytes used)\n", farSize);

	// TODO: actually create fsroot object and register it (aka mount initrd)
	return 0;
}

static void ModuleDetach()
{
	if (farData)
		free(farData);
}

FEOS_EXPORTMODULE(InitRdDrv)
{
	.Attach = ModuleAttach,
	.CanUnload = DefModCanUnload,
	.Detach = ModuleDetach,
};
