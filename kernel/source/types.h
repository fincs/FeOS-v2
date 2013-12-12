#pragma once

typedef unsigned char byte_t;
typedef unsigned short hword_t;
typedef unsigned int word_t;
typedef unsigned long long dword_t;
typedef signed char char_t;
typedef signed short short_t;
typedef signed int long_t;
typedef signed long long dlong_t;

typedef byte_t u8;
typedef hword_t u16;
typedef word_t u32;
typedef dword_t u64;
typedef char_t s8;
typedef short_t s16;
typedef long_t s32;
typedef dlong_t s64;
typedef float f32;
typedef double f64;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;
typedef volatile f32 vf32;
typedef volatile f64 vf64;

typedef void (* fp_t)(void);

#ifndef __cplusplus
#define nullptr ((void*)0)
typedef enum { false, true } bool;
#endif

#define BIT(n) (1 << (n))
