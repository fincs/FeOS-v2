#pragma once
#include "../feos.h"

// BCM2835 UART register definitions

#define UART0_BASE 0xA0201000

#define UART0_DR     (*(vu32*)(UART0_BASE + 0x00))
#define UART0_RSRECR (*(vu32*)(UART0_BASE + 0x04))
#define UART0_FR     (*(vu32*)(UART0_BASE + 0x18))
#define UART0_ILPR   (*(vu32*)(UART0_BASE + 0x20))
#define UART0_IBRD   (*(vu32*)(UART0_BASE + 0x24))
#define UART0_FBRD   (*(vu32*)(UART0_BASE + 0x28))
#define UART0_LCRH   (*(vu32*)(UART0_BASE + 0x2C))
#define UART0_CR     (*(vu32*)(UART0_BASE + 0x30))
#define UART0_IFLS   (*(vu32*)(UART0_BASE + 0x34))
#define UART0_IMSC   (*(vu32*)(UART0_BASE + 0x38))
#define UART0_RIS    (*(vu32*)(UART0_BASE + 0x3C))
#define UART0_MIS    (*(vu32*)(UART0_BASE + 0x40))
#define UART0_ICR    (*(vu32*)(UART0_BASE + 0x44))
#define UART0_DMACR  (*(vu32*)(UART0_BASE + 0x48))
#define UART0_ITCR   (*(vu32*)(UART0_BASE + 0x80))
#define UART0_ITIP   (*(vu32*)(UART0_BASE + 0x84))
#define UART0_ITOP   (*(vu32*)(UART0_BASE + 0x88))
#define UART0_TDR    (*(vu32*)(UART0_BASE + 0x8C))
