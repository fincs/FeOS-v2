.section ".vectors", "ax", %progbits
.align 2
.arm

.macro HALT
1: b 1b
.endm

.global __VectorBase
__VectorBase:
	@ Reset vector
	HALT
	@ Undefined instruction vector
	b __UNDHandler
	@ SVC vector
	b __SVCHandler
	@ Prefetch abort vector
	b __PABTHandler
	@ Data abort vector
	b __DABTHandler
	@ Unused vector
	HALT
	@ IRQ vector
	b __IRQHandler
	@ FIQ vector
	HALT

__SVCHandler:
	push {lr}

	@ Save SPSR
	mrs lr, spsr
	push {lr}

	@ Get address of target function
	ldrb lr, [lr, #-2] @ syscall ID
	ldr r12, =g_schedInfo+4 @ g_schedInfo.curProcess
	ldr r12, [r12, #8] @ curProcess->svcTable
	ldr r12, [r12, lr, lsl #2]

	@ Restore IRQs and call target function
	cpsie i
	blx r12

	@ Restore SPSR
	pop {lr}
	msr spsr, lr

	@ Return
	pop {lr}
	movs pc, lr

@ IRQ-bonded context structure:
@ r0-r12: common
@ sp,lr : USR/SYS mode (NOT used by __IRQHandler)
@ pc    : common
@ spsr  : obvious
@--------- (The following are NOT used by __IRQHandler)
@ sp,lr : SVC mode
@ fpscr : FP control reg
@ s0-s15: FP regs

__IRQHandler:
	@ Save context
	sub lr, #4 @ adjust saved PC
	sub sp, #36*4
	stm sp, {r0-r12}
	str lr, [sp, #15*4]
	mrs r0, spsr
	str r0, [sp, #16*4]

	@ Call main IRQ handler
	mov r0, sp
	bl KeIrqEntry

	@ Restore context
	ldr r0, [sp, #16*4]
	msr spsr, r0
	ldr lr, [sp, #15*4]
	ldm sp, {r0-r12}
	add sp, #36*4
	movs pc, lr

__UNDHandler:
	push {r0}
	mov r0, #0
	b __ExcptHandler

__PABTHandler:
	push {r0}
	sub lr, #4 @ adjust saved PC
	mov r0, #1
	b __ExcptHandler

__DABTHandler:
	push {r0}
	sub lr, #8 @ adjust saved PC
	mov r0, #2

__ExcptHandler:
	push {r12}
	mrs r12, spsr
	tst r12, #(1<<15) @ test for T bit
	pop {r12}

	sub sp, #14*4
	stm sp, {r0-r12,lr}
	ldr r12, [sp, #14*4] @ grab original r0
	str r12, [sp] @ save it in the right position
	mrs r12, spsr
	str r12, [sp, #14*4] @ save spsr

	@ Call exception handler
	mov r1, sp
	ldr r3, =KeExcptHandler
	blx r3

	@ Restore registers & return
	ldr r3, [sp, #14*4]
	msr spsr, r3
	ldm sp, {r0-r12,lr}
	add sp, #15*4
	movs pc, lr
