#pragma once
#include "common.h"

// Kernel debug I/O routines

// Implemented by HAL:
void KioInit(void);
void KioWrite(const void* buf, u32 size);
void KioWriteByte(int);

// Implemented by main kernel code
#define kputc KioWriteByte
void kputs(const char*);
void kputd(int);
void kputx(u32);
void kputu(u32);
void kprintf(const char* fmt, ...);
void kvprintf(const char* fmt, va_list va);

// k(v)printf arg list:
// %s - NULL-terminated string
// %d - signed int, decimal
// %p - pointer
// %x - u32, hexadecimal
// %u - u32, decimal
// %% - literal percent % sign
