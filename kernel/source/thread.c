#include "common.h"

static void ThrTimerISR(u32* regs);
static int thrTimerId;

static int ThrWakeUpIRQ(schedulerInfo* sched);
static threadInfo* ThrScheduler(schedulerInfo* sched, bool isPreempt);

static threadInfo* ThrCreateInfo(int prio)
{
	threadInfo* t = (threadInfo*) malloc(sizeof(threadInfo));
	if (!t) return nullptr;
	t->sched = ThrSchedInfo();
	t->process = t->sched->curProcess;
	t->flags = 0;
	t->prio = prio;
	t->quantum = SCHEDULER_RR_DEFAULT_Q;
	return t;
}

void ThrInit(void)
{
	schedulerInfo* sched = ThrSchedInfo();
	sched->curProcess = PsInit();
	threadInfo* t = ThrCreateInfo(0);
	sched->curThread = t;
	t->process = sched->curProcess;
	t->flags |= THRFLAG_CRITICAL;

	// Install scheduler timer
	thrTimerId = timerStart(SCHEDULER_FREQ, ThrTimerISR);
}

void ThrExit(int exitCode)
{
	schedulerInfo* sched = ThrSchedInfo();
	SpinlockAcquire(&sched->lock);
	threadInfo* t = sched->curThread;
	if (t->flags & THRFLAG_CRITICAL)
	{
		kputs("<FATAL ERROR KERNEL ERROR CRITICAL THREAD EXITED>\n");
		for (;;);
	}
	t->flags |= THRFLAG_FINISHED;
	t->exitCode = exitCode;
	threadQueue_add(&sched->finished[!!(t->flags & THRFLAG_DETACHED)], t);
	SpinlockRelease(&sched->lock);
	ThrYield();
	for(;;); // shouldn't reach
}

static int ThrTestMain(const char* str)
{
	extern semaphore_t mySem;

	kprintf("<ThrTestMain> I was given %s\nWoohoo!\n", str);

	int i;
	SemaphoreDown(&mySem);
	for (i = 0; i < 64; i ++)
	{
		kputc('R');
		timerWaitForIRQ(thrTimerId);
	}
	SemaphoreUp(&mySem);

	return 0;
}

void ThrTestCreate(void)
{
	static u32 buf[1024];

	schedulerInfo* sched = ThrSchedInfo();

	threadInfo* t = ThrCreateInfo(5);
	t->ctx.r[0] = (u32)"Hello world!";
	t->ctx.r[15] = (u32)ThrTestMain;
	t->ctx.cpsr = 0x10 | CPSR_MODE_SVC;
	t->ctx.svcSp = (u32)(buf + 1024);
	t->ctx.svcLr = (u32)ThrExit;

	SpinlockAcquire(&sched->lock);
	threadQueue_add(&sched->ready[t->prio][sched->which], t);
	sched->readyCount[sched->which] ++;
	bool bNeedYield = t->prio > sched->curThread->prio;
	SpinlockRelease(&sched->lock);
	kputs("Added thread!\n");
	if (bNeedYield)
		ThrYield();
}

void ThrYield(void)
{
	schedulerInfo* sched = ThrSchedInfo();
	SpinlockAcquire(&sched->lock);

	threadInfo* curT = sched->curThread;
	threadInfo* nextT;

	bool bIsBlock = curT->flags & THRFLAG_BLOCKED;
	if (!bIsBlock)
	{
		nextT = ThrScheduler(sched, false);
		if (!nextT)
		{
			// No thread to run - oops sorry :p
			SpinlockRelease(&sched->lock);
			return;
		}
	} else
	{
		for (;;)
		{
			nextT = ThrScheduler(sched, false);
			if (nextT) break;
			CpuIdle();
		}
		if (nextT == curT)
		{
			SpinlockRelease(&sched->lock);
			return;
		}
	}

	if (!bIsBlock)
	{
		// Add current thread to expired queue
		threadQueue_add(&sched->ready[curT->prio][!sched->which], curT);
		sched->readyCount[!sched->which] ++;
		curT->quantum = SCHEDULER_RR_DEFAULT_Q;
	}

	CpuIrqDisable();
	SpinlockRelease(&sched->lock);

	// Perform process context switch
	if (nextT->process != sched->curProcess)
	{
		sched->curProcess = nextT->process;
		PsCtxSwitch(sched->curProcess);
	}

	// Perform thread context switch
	sched->curThread = nextT;
	if (ThrCtxSetJmp(&curT->ctx))
		return;

	CpuSyncBarrier();
	ThrCtxLongJmp(&nextT->ctx);
}

void ThrWaitForIRQ(int ctrlId, u32 mask)
{
	schedulerInfo* sched = ThrSchedInfo();
	SpinlockAcquire(&sched->lock);
	threadInfo* curT = sched->curThread;
	curT->irqCtrl = ctrlId;
	curT->irqMask = mask;
	curT->flags |= THRFLAG_BLOCKED;
	threadQueue_add(&sched->irqQueue, curT);
	SpinlockRelease(&sched->lock);
	ThrYield();
}

void ThrSleep(int ms)
{
	schedulerInfo* sched = ThrSchedInfo();
	u32 schedPeriod = 1000/(SCHEDULER_FREQ>>12);
	u32 target = sched->tickCount + ((u32)ms+schedPeriod-1) / schedPeriod;
	
	while (sched->tickCount < target)
		timerWaitForIRQ(thrTimerId);
}

void ThrTimerISR(u32* regs)
{
	u32 spsr = regs[16];
	if (CPSR_MODE(spsr) != CPSR_MODE_USR && CPSR_MODE(spsr) != CPSR_MODE_SVC)
		return; // Do not disturb other modes

	schedulerInfo* sched = ThrSchedInfo();
	sched->tickCount ++;

	if (sched->lock)
		return; // already scheduling

	threadInfo* curT = sched->curThread;
	if (ThrWakeUpIRQ(sched) <= curT->prio)
	{
		if (IS_FIFO_PRIO(curT->prio))
			return;
		if ((--curT->quantum) > 0)
			return; // still have enough quantum
	}

	threadInfo* nextT = ThrScheduler(sched, true);
	if (!nextT)
		return; // no thread to switch to

	// Add current thread to expired queue
	threadQueue_add(&sched->ready[curT->prio][!sched->which], curT);
	sched->readyCount[!sched->which] ++;
	curT->quantum = SCHEDULER_RR_DEFAULT_Q;

	// Perform process context switch
	if (nextT->process != sched->curProcess)
	{
		sched->curProcess = nextT->process;
		PsCtxSwitch(sched->curProcess);
	}

	// Perform thread context switch
	sched->curThread = nextT;
	ThrCtxCopy(&curT->ctx, (const cpuContext*)regs, false);
	ThrCtxCopy((cpuContext*)regs, &nextT->ctx, true);
	CpuSyncBarrier();
}

int ThrWakeUpIRQ(schedulerInfo* sched)
{
	int i, maxPrio = -1;
	threadInfo *t, *tN;

	u32 irqMasks[MAX_IRQCTRL];
	for (i = 0; i < MAX_IRQCTRL; i ++)
		irqMasks[i] = irqFlags(i);

	// Wake up IRQ wait threads.
	for (t = sched->irqQueue.first; t; t = tN)
	{
		tN = t->next;
		if (irqMasks[t->irqCtrl] & t->irqMask)
		{
			t->flags &= ~THRFLAG_BLOCKED;
			if (t->prio > maxPrio)
				maxPrio = t->prio;
			threadQueue_remove(t);
			threadQueue_add(&sched->ready[t->prio][sched->which], t);
			t->quantum = SCHEDULER_RR_DEFAULT_Q;
			sched->readyCount[sched->which] ++;
		}
	}

	return maxPrio;
}

threadInfo* ThrScheduler(schedulerInfo* sched, bool isPreempt)
{
	threadInfo* t;
	int i;

	if (!isPreempt) // otherwise already done
		ThrWakeUpIRQ(sched);

	if (!sched->readyCount[sched->which])
	{
		sched->which = !sched->which;
		if (!sched->readyCount[sched->which])
		{
			// No thread can be executed
			// =Deadlock if there aren't IRQ waiting threads
			return nullptr;
		}
	}

	for (i = 15; i >= 0; i --)
	{
		threadQueue* q = &sched->ready[i][sched->which];
		if (q->first)
		{
			sched->readyCount[sched->which] --;
			t = threadQueue_pop(q);
#ifdef DEBUG
			kprintf("<ThrScheduler> Switching to %p prio=%d svcLr=%x\n", t, i, t->ctx.svcLr);
#endif
			return t;
		}
	}

	// This part shouldn't be reached
	return nullptr;
}

void SemaphoreUp(semaphore_t* s)
{
	if (!s->waiting.first)
	{
		AtomicIncrement(&s->counter);
		return;
	}

	// Wake up first thread
	schedulerInfo* sched = ThrSchedInfo();
	SpinlockAcquire(&sched->lock);
	threadInfo* t = threadQueue_pop(&s->waiting);
	t->flags &= ~THRFLAG_BLOCKED;
	threadQueue_add(&sched->ready[t->prio][sched->which], t);
	sched->readyCount[sched->which] ++;
	bool bNeedYield = t->prio > sched->curThread->prio;
	SpinlockRelease(&sched->lock);
	if (bNeedYield)
		ThrYield();
}

void SemaphoreDown(semaphore_t* s)
{
	if (!AtomicDecrementCompareZero(&s->counter))
		return;

	// Put thread to wait
	schedulerInfo* sched = ThrSchedInfo();
	SpinlockAcquire(&sched->lock);
	threadInfo* curT = sched->curThread;
	curT->flags |= THRFLAG_BLOCKED;
	threadQueue_add(&s->waiting, curT);
	SpinlockRelease(&sched->lock);
	ThrYield();
}

static void crapmemcpy32(u32* dest, const u32* source, int size)
{
	size >>= 2;
	while (size--)
		*dest++ = *source++;
}

void ThrCtxCopy(cpuContext* outCtx, const cpuContext* inCtx, bool restore)
{
	crapmemcpy32(outCtx->r, inCtx->r, 12*4);
	outCtx->r[15] = inCtx->r[15];
	outCtx->cpsr = inCtx->cpsr;

	if (restore)
	{
		CpuRestoreModeRegs(CPSR_MODE_USR, inCtx->r[13], inCtx->r[14]);
		CpuRestoreModeRegs(CPSR_MODE_SVC, inCtx->svcSp, inCtx->svcLr);
		// TODO: fp
	} else
	{
		CpuSaveModeRegs(CPSR_MODE_USR, &outCtx->r[13], &outCtx->r[14]);
		CpuSaveModeRegs(CPSR_MODE_SVC, &outCtx->svcSp, &outCtx->svcLr);
		// TODO: fp
	}
}
