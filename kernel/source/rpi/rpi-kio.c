#include "../common.h"
#include "gpio.h"
#include "uart.h"

// Code adapted from http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C#uart.c

void KioInit(void)
{
	CpuSyncBarrier();
	// Disable UART0
	UART0_CR = 0;
	CpuSyncBarrier();

	// Disable pull up/down for all GPIO pins
	REG_GPPUD = 0;
	CpuLoopDelay(150);

	// Disable pull up/down for pin 14,15
	REG_GPPUDCLK0 = BIT(14) | BIT(15);
	CpuLoopDelay(150);

	// Clear pending interrupts.
	UART0_ICR = 0x7FF;

	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200

	// Divider = 3000000/(16 * 115200) = 1.627 = ~1
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40
	UART0_IBRD = 1;
	UART0_FBRD = 40;

	// Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
	UART0_LCRH = BIT(4) | BIT(5) | BIT(6);

	// Mask all interrupts.
	UART0_IMSC = BIT(1) | BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | BIT(10);

	// Enable UART0, receive & transfer part of UART.
	UART0_CR = BIT(0) | BIT(8) | BIT(9);

	gpioSetPinFunc(GPIO_PIN_ACTLED, GPF_OUTPUT);
	gpioSetPinVal(GPIO_PIN_ACTLED, false); // 0=LED on, 1=LED off
}

static inline void _WriteByte(int x)
{
	while (UART0_FR & BIT(5));
	UART0_DR = x & 0xFF;
}

void KioWrite(const void* buf, u32 size)
{
	CpuSyncBarrier();
	const u8* d = (const u8*)buf;
	while (size--)
		_WriteByte(*d++);
	CpuSyncBarrier();
}

void KioWriteByte(int x)
{
	CpuSyncBarrier();
	_WriteByte(x);
	CpuSyncBarrier();
}
