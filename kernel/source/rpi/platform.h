#pragma once
#include "../common.h"

// BCM2835 IRQ controller map:
// 0 - Interrupts 0..31
// 1 - Interrupts 32..63
// 2 - "Base" interrupts
#define MAX_IRQCTRL 3
#define MAX_TIMER 2 // timers 0 and 2 are reserved by GPU
#define TIMER_TICK_FREQ 1000000

#define IRQ0_TIMER(n)   BIT(0+(n)) // 0..3
#define IRQ0_CODEC(n)   BIT(4+(n)) // 0..2
#define IRQ0_JPEG       BIT(7)
#define IRQ0_ISP        BIT(8)
#define IRQ0_USB        BIT(9)
#define IRQ0_3D         BIT(10)
#define IRQ0_TRANSPOSER BIT(11)
#define IRQ0_MCSYNC(n)  BIT(12+(n)) // 0..3
#define IRQ0_DMA(n)     BIT(16+(n)) // 0..12
#define IRQ0_AUX        BIT(29)
#define IRQ0_ARM        BIT(30)
#define IRQ0_VPUDMA     BIT(31)

#define IRQ1_HOSTPORT    BIT(0)
#define IRQ1_VIDEOSCALER BIT(1)
#define IRQ1_CCP2TX      BIT(2)
#define IRQ1_SDC         BIT(3)
#define IRQ1_DSI0        BIT(4)
#define IRQ1_AVE         BIT(5)
#define IRQ1_CAM(n)      BIT(6+(n)) // 0..1
#define IRQ1_HDMI(n)     BIT(8+(n)) // 0..1
#define IRQ1_PIXELVALVE1 BIT(10)
#define IRQ1_I2CSPISLV   BIT(11)
#define IRQ1_DSI1        BIT(12)
#define IRQ1_PWA(n)      BIT(13+(n)) // 0..1
#define IRQ1_CPR         BIT(15)
#define IRQ1_SMI         BIT(16)
#define IRQ1_GPIO(n)     BIT(17+(n)) // 0..3
#define IRQ1_I2C         BIT(21)
#define IRQ1_SPI         BIT(22)
#define IRQ1_I2SPCM      BIT(23)
#define IRQ1_SDIO        BIT(24)
#define IRQ1_UART        BIT(25)
#define IRQ1_SLIMBUS     BIT(26)
#define IRQ1_VEC         BIT(27)
#define IRQ1_CPG         BIT(28)
#define IRQ1_RNG         BIT(29)
#define IRQ1_ARASANSDIO  BIT(30)
#define IRQ1_AVSPMON     BIT(31)

#define IRQ2_ARM_TIMER       BIT(0)
#define IRQ2_ARM_MAILBOX     BIT(1)
#define IRQ2_ARM_DOORBELL(n) BIT(2+(n)) // 0..1
#define IRQ2_VPU_HALTED(n)   BIT(4+(n)) // 0..1
#define IRQ2_ILLEGAL_TYPE(n) BIT(6+(n)) // 0..1
