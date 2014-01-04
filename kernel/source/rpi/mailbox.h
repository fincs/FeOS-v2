#pragma once
#include "../common.h"

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

enum
{
	MBCHN_PM = 0,  // Power Management
	MBCHN_FB,      // Framebuffer
	MBCHN_VUART,   // Virtual UART
	MBCHN_VCHIQ,   // VCHIQ interface
	MBCHN_LED,     // LEDs interface
	MBCHN_BUTTONS, // Buttons interface
	MBCHN_TOUCH,   // Touch screen interface
	MBCHN__UNUSED,
	MBCHN_PTAGS,   // Property tags
	MBCHN__MASK = 0xF,
};

u32 mailboxExecCmd(int channel, u32 value);
