.section ".init"
.align 4
.arm

#define BIT(n) (1 << (n))
#define ATAG_MEM 0x54410002
#define ATAG_INITRD2 0x54420005

.global __entrypoint
__entrypoint:

#ifndef FEOS_3ds
	@ Get boot parameters from ATAGs
	mov r8, #0
	mov r9, #0
	mov r10, #0
	ldr r3, =ATAG_MEM
	ldr r4, =ATAG_INITRD2
.Latag_loop:
	ldr r0, [r2, #4]
	cmp r0, #0
	beq .Latag_endLoop

	@ ATAG_MEM?
	cmp r0, r3
	ldreq r8, [r2, #8]
	beq .Latag_loopTail

	@ ATAG_INITRD2?
	cmp r0, r4
	bne .Latag_loopTail
	ldr r9, [r2, #8]
	ldr r10, [r2, #12]

.Latag_loopTail:
	ldr r0, [r2, #0]
	add r2, r0, lsl #2
	b .Latag_loop

.Latag_endLoop:
	@ Ensure that we at least retrieved the RAM size
	cmp r8, #0
	beq .Latag_endLoop

#else
	@ Hardcoded boot parameters
	ldr r8, =512*1024 + 128*1024*1024 @ RAM size: 512KB AXI WRAM + 128MB FCRAM
	mov r9, #0  @ No RAM disk
	mov r10, #0 @ No RAM disk
#endif

	@@-----
	@@ INITIALIZE CPU
	@@-----

	@ Disable all interrupts and switch to Supervisor mode
	cpsid if, #0x13

	@ Disable any unwanted features that may be enabled
	@ Enable high vectors, branch prediction, unaligned access and ARMv6 page format
	ldr r0, =BIT(11) | BIT(13) | BIT(22) | BIT(23) | BIT(4) | BIT(5) | BIT(6) | BIT(16) | BIT(18)
	mrc p15, 0, r1, c1, c0, 0
	and r1, #BIT(31) | BIT(30) | BIT(27) | BIT(26) @ preserve UNP'ed bits
	orr r0, r1
	mcr p15, 0, r0, c1, c0, 0

	@ Flush caches
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0  @ Instruction cache
	mcr p15, 0, r0, c7, c14, 0 @ Data cache
	mcr p15, 0, r0, c7, c10, 4 @ Write buffer
	mcr p15, 0, r0, c8, c7, 0  @ Instruction and data TLBs

	@@-----
	@@ SET UP MMU
	@@-----

	@ Set up pagetables
	mov r0, r8
	ldr sp, =.Ltempstack
	bl __init_pagetables

	@ Set up translation table registers
	mov r0, #1
	mcr p15, 0, r0, c2, c0, 2 @ TTBCR N=1 (divides L1 table in two equal 2GB halves)
	ldr r0, =__pagetables @ TODO: add flags
	mcr p15, 0, r0, c2, c0, 0 @ TTBR0 - user part
	add r0, #4*2048
	mcr p15, 0, r0, c2, c0, 1 @ TTBR1 - kernel part

	@ Set up (dummy) domains
	mov r0, #1 @ domain0: client
	mcr p15, 0, r0, c3, c0, 0

	@ TURN ON MMU
	mrc p15, 0, r0, c1, c0, 0
	orr r0, #1
	mcr p15, 0, r0, c1, c0, 0

	@ Set up IRQ stack
	ldr r4, =__kmem_top
	cps #0x12
	mov sp, r4

	@ Set up Abort stack
	sub r4, #0x200
	cps #0x17
	mov sp, r4

	@ Set up Undefined stack
	sub r4, #0x100
	cps #0x1B
	mov sp, r4

	@ Set up SVC stack and stay in SVC mode
	sub r4, #0x100
	cps #0x13
	mov sp, r4

	@ Clear BSS
	ldr r0, =__bss_start
	ldr r1, =__bss_size
	bl .LclearMem

	@ Jump to main kernel code
	mov r0, r8
	mov r1, r9
	mov r2, r10
	ldr lr, =0xFFFF0000
	ldr r4, =kmain
	bx r4

	.space 4*16
.Ltempstack:

@ Function borrowed from devkitARM crt0s
@ r0 - address, r1 - size
.LclearMem:
	add r1, #3  @ Word-align size
	bics r1, #3 @ |
	bxeq lr     @ Exit if nothing to do
	mov r2, #0
.LclearMemLoop:
	stm r0!, {r2}
	subs r1, #4
	bne .LclearMemLoop
	bx lr
