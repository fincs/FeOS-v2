#pragma once
#ifndef FEOS_INCLUDED
#error "You must include <feos.h> first!"
#endif

#ifndef SCHEDULER_FREQ
#define SCHEDULER_FREQ TIMER_HZ(100)
#endif
#ifndef SCHEDULER_RR_DEFAULT_Q
#define SCHEDULER_RR_DEFAULT_Q 10 // 10 100Hz ticks = 100 ms
#endif

typedef struct
{
	u32 r[16]; // sp,lr: USR/SYS mode
	u32 cpsr;
	u32 svcSp, svcLr;

	/*
	u32 fpscr;
	u32 s[32];
	*/
} cpuContext;

typedef vu32 spinlock_t;

static inline void SpinlockAcquire(spinlock_t* l)
{
	while (AtomicSwap(l, 1));
}

static inline void SpinlockRelease(spinlock_t* l)
{
	*l = 0;
}

#define CPSR_MODE(n) ((n) & 0xF)
#define CPSR_MODE_SYS 0xF
#define CPSR_MODE_USR 0x0
#define CPSR_MODE_SVC 0x3
#define CPSR_MODE_IRQ 0x2
#define CPSR_MODE_FIQ 0x1
#define CPSR_MODE_ABT 0x7
#define CPSR_MODE_UND 0xB

typedef struct tag_threadInfo threadInfo;
typedef struct tag_threadQueue threadQueue;

_STATIC cpuContext* ThrGetContext(threadInfo* t);

struct tag_threadQueue
{
	threadInfo* first;
	threadInfo* last;
};

#define IS_FIFO_PRIO(n) ((n) >= 12)
#define IS_STATICRR_PRIO(n) ((n) >= 8)

typedef int (* ThrEntrypoint)(void*);
#define DEFAULT_STACKSIZE 8192

threadInfo* ThrCreateK(ThrEntrypoint ep, void* userParam, int prio, size_t stackSize);
void ThrYield(void);
void __attribute__((noreturn)) ThrExit(int exitCode);
void ThrWaitForIRQ(int ctrlId, u32 mask);

void ThrSleep(int ms); // NOT to be used for accurate timing!

typedef struct
{
	vu32 counter;
	threadQueue waiting;
} semaphore_t;

#define SEMAPHORE_STATIC_INIT(counter) { (counter), { nullptr, nullptr } }

static inline void SemaphoreInit(semaphore_t* s, int counter)
{
	s->counter = counter;
	s->waiting.first = nullptr;
	s->waiting.last = nullptr;
}

void SemaphoreDown(semaphore_t* s);
void SemaphoreUp(semaphore_t* s);

static inline bool isIrqMode()
{
	return CPSR_MODE(CpuGetCPSR()) == CPSR_MODE_IRQ;
}
