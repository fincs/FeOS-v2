#pragma once
#include "../common.h"

// BCM2835 GPIO register definitions

#define GPIO_BASE 0xA0200000

#define REG_GPFSEL ((vu32*)(GPIO_BASE + 0x00)) // 0..5
#define REG_GPSET  ((vu32*)(GPIO_BASE + 0x1C)) // 0..1
#define REG_GPCLR  ((vu32*)(GPIO_BASE + 0x28)) // 0..1

// Controls actuation of pull up/down to ALL GPIO pins.
#define REG_GPPUD     (*(vu32*)(GPIO_BASE + 0x94))
// Controls actuation of pull up/down for specific GPIO pin.
#define REG_GPPUDCLK0 (*(vu32*)(GPIO_BASE + 0x98))

enum
{
	GPF_INPUT = 0,
	GPF_OUTPUT,
	GPF_ALT5,
	GPF_ALT4,
	GPF_ALT0,
	GPF_ALT1,
	GPF_ALT2,
	GPF_ALT3,
	GPF_MASK = GPF_ALT3,
};

#define GPIO_PIN_ACTLED 16

static inline void gpioSetPinFunc(int pin, int function)
{
	CpuSyncBarrier();
	int regId = pin/10;
	int regBit = (pin%10)*3;
	REG_GPFSEL[regId] = (REG_GPFSEL[regId] &~ (GPF_MASK<<regBit)) | ((function&GPF_MASK)<<regBit);
	CpuSyncBarrier();
}

static inline void gpioSetPinVal(int pin, bool value)
{
	CpuSyncBarrier();
	int x = 0;
	if (pin >= 32)
		x = 1, pin -= 32;
	if (value)
		REG_GPSET[x] = BIT(pin);
	else
		REG_GPCLR[x] = BIT(pin);
	CpuSyncBarrier();
}
