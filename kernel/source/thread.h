#pragma once
#include "common.h"

#ifndef SCHEDULER_FREQ
#define SCHEDULER_FREQ TIMER_HZ(100)
#endif
#ifndef SCHEDULER_RR_DEFAULT_Q
#define SCHEDULER_RR_DEFAULT_Q 10 // 10 100Hz ticks = 100 ms
#endif

typedef vu32 spinlock_t;

static inline void SpinlockAcquire(spinlock_t* l)
{
	while (AtomicSwap(l, 1));
}

static inline void SpinlockRelease(spinlock_t* l)
{
	*l = 0;
}

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
typedef struct tag_schedulerInfo schedulerInfo;

enum
{
	THRFLAG_CRITICAL = BIT(0),
	THRFLAG_DETACHED = BIT(1),
	THRFLAG_BLOCKED = BIT(2),
	THRFLAG_FINISHED = BIT(3),
};

struct tag_threadInfo
{
	threadInfo* next;
	threadInfo* prev;
	threadQueue* queue;
	schedulerInfo* sched;
	processInfo* process;
	cpuContext ctx;

	u32 flags;
	int quantum;
	int prio;
	
	union
	{
		// For IRQ wait
		struct
		{
			int irqCtrl; // -1 = wait for any IRQ
			u32 irqMask;
		};
		int exitCode;
	};
};

// Private stuff

struct tag_threadQueue
{
	threadInfo* first;
	threadInfo* last;
};

static inline void threadQueue_add(threadQueue* q, threadInfo* t)
{
	t->next = nullptr;
	t->prev = q->last;
	t->queue = q;
	q->last = t;
	if (t->prev)
		t->prev->next = t;
	else
		q->first = t;
}

static inline threadInfo* threadQueue_pop(threadQueue* q)
{
	threadInfo* t = q->first;
	if (t)
	{
		if (t->next)
			t->next->prev = nullptr;
		else
			q->last = nullptr;
		t->queue = nullptr;
		q->first = t->next;
	}
	return t;
}

static inline void threadQueue_remove(threadInfo* t)
{
	threadQueue* q = t->queue;
	t->queue = nullptr;
	if (t->prev)
	{
		t->prev->next = t->next;
		if (t->next)
			t->next->prev = t->prev;
		else
			q->last = t->prev;
	} else
	{
		q->first = t->next;
		if (t->next)
			t->next->prev = nullptr;
		else
			q->last = nullptr;
	}
}

struct tag_schedulerInfo
{
	spinlock_t lock;
	threadQueue ready[16][2];
	threadQueue finished[2];
	threadQueue irqQueue;
	int readyCount[2];
	bool which;
	processInfo* curProcess;
	threadInfo* curThread;
};

#define IS_FIFO_PRIO(n) ((n) >= 12)
#define IS_STATICRR_PRIO(n) ((n) >= 8)

void ThrInit(void);
void ThrTestCreate(void);
void ThrYield(void);
void __attribute__((noreturn)) ThrExit(int exitCode);
void ThrWaitForIRQ(int ctrlId, u32 mask);

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

//-----------------------------------------------------------------------------
// Beyond this point everything is internal
//-----------------------------------------------------------------------------

static inline schedulerInfo* ThrSchedInfo(void)
{
	// Done in this way so that in the future SMP is supported
	extern schedulerInfo g_schedInfo;
	return &g_schedInfo;
}

// for preempt
void ThrCtxCopy(cpuContext* outCtx, const cpuContext* inCtx, bool restore);
// for yield
bool ThrCtxSetJmp(cpuContext* ctx);
void __attribute__((noreturn)) ThrCtxLongJmp(const cpuContext* outCtx);
