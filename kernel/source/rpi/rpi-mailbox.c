#include "../common.h"
#include <rpi/mailbox.h>

// BCM2835 Mailbox register definitions

#define MAILBOX_BASE 0xA000b880

#define REG_MBREAD   (*(vu32*)(MAILBOX_BASE + 0x00))
#define REG_MBPEEK   (*(vu32*)(MAILBOX_BASE + 0x10))
#define REG_MBSNDID  (*(vu32*)(MAILBOX_BASE + 0x14)) // sender id, bottom 2 bits
#define REG_MBSTAT   (*(vu32*)(MAILBOX_BASE + 0x18))
#define REG_MBCONFIG (*(vu32*)(MAILBOX_BASE + 0x1C))
#define REG_MBWRITE  (*(vu32*)(MAILBOX_BASE + 0x20))

#define MB_EMPTY BIT(30)
#define MB_FULL  BIT(31)

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
	return value &~ MBCHN__MASK;
}
