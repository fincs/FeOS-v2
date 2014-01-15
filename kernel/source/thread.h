#pragma once
#include "common.h"

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

static inline cpuContext* ThrGetContext(threadInfo* t)
{
	return &t->ctx;
}

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
	processInfo* curProcess; // vectors.s depends on this field being at this offset
	threadInfo* curThread;
	threadQueue ready[16][2];
	threadQueue finished[2];
	threadQueue irqQueue;
	int readyCount[2];
	bool which;
};

void ThrInit(void);

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
