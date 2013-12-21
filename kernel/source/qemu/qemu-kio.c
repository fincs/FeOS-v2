#include "../common.h"

#define REG_UART0DR (*(vu32*)0xA01f1000)

void KioInit(void)
{
	// Nothing
}

void KioWrite(const void* buf, u32 size)
{
	const u8* d = (const u8*)buf;
	while (size--)
		REG_UART0DR = *d++;
}

void KioWriteByte(int x)
{
	REG_UART0DR = x & 0xFF;
}
