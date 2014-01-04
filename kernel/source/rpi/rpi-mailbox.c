#include "../common.h"
#include "mailbox.h"

static semaphore_t mbSem = SEMAPHORE_STATIC_INIT(1);

u32 mailboxExecCmd(int channel, u32 value)
{
	channel &= MBCHN__MASK;
	value &= ~MBCHN__MASK;
	value |= channel;
	SemaphoreDown(&mbSem);
	CpuSyncBarrier();

	// Perform write operation
	while (REG_MBSTAT & MB_FULL);
	REG_MBWRITE = value;

	// Perform read operation
	for (;;)
	{
		while (REG_MBSTAT & MB_EMPTY);
		value = REG_MBREAD;
		if ((value & MBCHN__MASK) == channel)
		{
			value &= ~MBCHN__MASK;
			break;
		}
	}

	CpuSyncBarrier();
	SemaphoreUp(&mbSem);
	return value;
}
