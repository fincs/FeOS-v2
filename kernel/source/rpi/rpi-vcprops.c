#include "../common.h"
#include <rpi/vcprops.h>

bool vcPropsAlloc(VCPROPS* pProps, u32 bufSize)
{
	if (bufSize < (6*4))
		return false; // This buffer is too small

	bufSize = (bufSize+15)&~15;
	vu32* mem = (vu32*)memalign(16, bufSize);
	if (!mem) return false;
	pProps->buffer = mem;
	pProps->size = bufSize/4;
	pProps->pos = 2;
	mem[0] = bufSize;
	mem[1] = 0; // request
	return true;
}

vu32* vcPropsAddTag(VCPROPS* pProps, u32 tagId, u32 requestSize, u32 responseSize)
{
	u32 bufSizeBytes = responseSize > requestSize ? responseSize : requestSize;
	u32 bufSize = (bufSizeBytes+3)/4 + 3;

	u32 pos = pProps->pos;
	u32 remainingSize = pProps->size - pos - 1;
	if (remainingSize < bufSize)
		return nullptr;

	vu32* mem = pProps->buffer;
	mem[pos++] = tagId;
	mem[pos++] = bufSizeBytes;
	mem[pos++] = requestSize;
	pProps->pos += bufSize;
	return mem + pos;
}

bool vcPropsExecute(VCPROPS* pProps)
{
	vu32* mem = pProps->buffer;
	u32 size = pProps->size*4;
	mem[pProps->pos] = 0; // end tag
	DC_FlushRange((void*)mem, size);
	mailboxExecCmd(MBCHN_PTAGS, MemTranslateAddr((void*)mem));
	return mem[1] == BIT(31);
}
