#pragma once
// These are supplied by the compiler
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

typedef uint8_t byte_t;
typedef uint16_t hword_t;
typedef uint32_t word_t;
typedef uint64_t dword_t;
typedef int8_t char_t;
typedef int16_t short_t;
typedef int32_t long_t;
typedef int64_t dlong_t;

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
#define nullptr NULL
#endif

#define BIT(n) (1 << (n))

#ifdef FEOS_KERNEL
#ifdef FEOS_KMODULE
#define _STATIC
#else
#define _STATIC static
#endif
#endif

#define FEOS_EXPORT __attribute__((__visibility__("default"))) //!< Exported symbol attribute.
#define FEOS_WEAK __attribute__((weak)) //!< Weak symbol attribute.
#define FEOS_DEPRECATED __attribute__((deprecated)) //!< Deprecated symbol attribute.
#define FEOS_INIT __attribute__((constructor)) //!< Initialization function attribute. Makes a function be automatically called when a module is loaded.
#define FEOS_FINI __attribute__((destructor)) //!< Deinitialization function attribute. Makes a function be automatically called when a module is unloaded.
