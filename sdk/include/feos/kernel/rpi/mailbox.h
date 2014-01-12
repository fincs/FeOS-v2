#pragma once
#include "../feos.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
