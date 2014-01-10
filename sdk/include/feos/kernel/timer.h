#pragma once
#include "feos.h"

// Simple common timer API

#ifndef MAX_TIMER
#define MAX_TIMER 1
#endif

#define TIMER_HZ(x) ((u32)(x)<<12)

// All of these functions are implemented by the HAL

// Starts a new timer.
// The frequency is in Hz, 12-bit fixed point.
// If ISR is NULL then no ISR is called (however IRQs are still fired)
int timerStart(int freq, isr_t isr);
int timerRead(int timer);
void timerPause(int timer, bool bPause);
void timerStop(int timer);
void timerWaitForIRQ(int timer);
