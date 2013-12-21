#pragma once
#include "../common.h"

// BCM2835 GPIO register definitions

#define GPIO_BASE 0xA0200000

// Controls actuation of pull up/down to ALL GPIO pins.
#define REG_GPPUD     (*(vu32*)(GPIO_BASE + 0x94))
// Controls actuation of pull up/down for specific GPIO pin.
#define REG_GPPUDCLK0 (*(vu32*)(GPIO_BASE + 0x98))
