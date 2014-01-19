#include "common.h"

typedef struct
{
	word_t reg[13];
	word_t pc;
	word_t cpsr;
} excptArgs_t;

enum
{
	Excpt_UndInstr = 0,
	Excpt_PrefAbort = 1,
	Excpt_DataAbort = 2,
};

static const char* const excptTypeString[] =
{
	"Undefined Instruction",
	"Prefetch Abort",
	"Data Abort",
};

static const char* getModeName(int mode)
{
	switch (mode)
	{
		case CPSR_MODE_USR: return "User";
		case CPSR_MODE_SVC: return "Kernel";
		case CPSR_MODE_IRQ: return "IRQ";
		case CPSR_MODE_ABT: return "ExcptAbort";
		case CPSR_MODE_UND: return "ExcptUndef";
		case CPSR_MODE_SYS: return "BAD:System";
		case CPSR_MODE_FIQ: return "BAD:FIQ";
		default:            return "BAD:Unknown";
	}
}

void KeExcptHandler(int type, excptArgs_t* args)
{
	word_t cpsr = args->cpsr;
	if (type == Excpt_UndInstr)
		args->pc -= (cpsr & BIT(5)) ? 2 : 4;
	word_t excptPc = args->pc;

	int mode = cpsr&0xF;
	const char* modeName = getModeName(mode);
	int i;
	word_t excptSp, excptLr;
	CpuSaveModeRegs(mode, &excptSp, &excptLr);

	kputs("\n>> KRNLB0RK\n");
	kprintf(">> %s at %p (%s mode)\n", excptTypeString[type], excptPc, modeName);
	for (i = 0; i < 5; i ++)
		kprintf("> r%d  = %x | r%d  = %x\n", i*2, args->reg[i*2], i*2+1, args->reg[i*2+1]);
	kprintf("> r10 = %x | r11 = %x\n", args->reg[10], args->reg[10]);
	kprintf("> r12 = %x | sp  = %x\n", args->reg[12], excptSp);
	kprintf("> lr  = %x | pc  = %x\n", excptLr, excptPc);

	printf("\n<<General Kernel b0rk>>\n");
	printf("Something went horribly wrong and FeOS/High became completely b0rked.\n");
	printf("We apologize for any inconvenience and/or data loss caused by the b0rk.\n");
	printf("Exact details of the b0rk:\n");
	printf(">> %s at %p (%s mode)\n", excptTypeString[type], (void*)excptPc, modeName);
	for (i = 0; i < 5; i ++)
		printf("> r%d  = %x | r%d  = %x\n", i*2, args->reg[i*2], i*2+1, args->reg[i*2+1]);
	printf("> r10 = %x | r11 = %x\n", args->reg[10], args->reg[10]);
	printf("> r12 = %x | sp  = %x\n", args->reg[12], excptSp);
	printf("> lr  = %x | pc  = %x\n", excptLr, excptPc);

	for (;;);
}
