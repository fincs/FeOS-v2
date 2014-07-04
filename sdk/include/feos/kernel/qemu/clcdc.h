#pragma once
#include "../feos.h"

// PrimeCell CLCDC register definitions

#define SYS_OSCCLK4 (*(vu32*)(0xA000001c))
#define CLDC_BASE 0xA0120000

#define CLCD_TIM0   (*(vu32*)(CLDC_BASE+0x00))
#define CLCD_TIM1   (*(vu32*)(CLDC_BASE+0x04))
#define CLCD_TIM2   (*(vu32*)(CLDC_BASE+0x08))
#define CLCD_FBADDR (*(vu32*)(CLDC_BASE+0x10))
#define CLCD_CTRL   (*(vu32*)(CLDC_BASE+0x18))

// Timing constants for VGA 640x480
#define CLDC_VGA_CLK  0x2C77
#define CLDC_VGA_TIM0 0x3F1F3F9C
#define CLDC_VGA_TIM1 0x090B61DF
#define CLDC_VGA_TIM2 0x067F1800

// Timing constants for SVGA 800x600
#define CLDC_SVGA_CLK  0x2CAC
#define CLDC_SVGA_TIM0 0x1313A4C4
#define CLDC_SVGA_TIM1 0x0505F657
#define CLDC_SVGA_TIM2 0x071F1800
