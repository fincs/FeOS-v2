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
	ldr r12, =g_SyscallTablePtr
	ldr r12, [r12]
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
	mov r0, #2
	sub lr, #8 @ adjust saved PC

__ExcptHandler:
	push {r12}
	mrs r12, spsr
	tst r12, #(1<<15) @ test for T bit
	pop {r12}

	@HALT
	push {r1-r3}
	mov r1, lr
	ldr r3, =WhateverExcpt
	blx r3
	pop {r1-r3}

	@ EEEEE Return
	pop {r0}
	movs pc, lr
