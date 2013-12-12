.text
.arm

.global ThrCtxSetJmp
.hidden ThrCtxSetJmp
.align 2
ThrCtxSetJmp:
	mov r3, r0
	mov r0, #0
	cps #0x1F @ switch to SYS mode
	str sp, [r3, #4*13]
	str lr, [r3, #4*14]
	cps #0x13 @ switch back to SVC mode
	mrs r1, cpsr
	bic r1, #(1<<7) @ clear IRQ disable flag
	str r1, [r3, #4*16]
	str sp, [r3, #4*17]
	str lr, [r3, #4*18]
	mov r1, #1
	str r1, [r3]
	add r3, #4*4
	stm r3, {r4-r11}
	mov r1, pc
	str r1, [r3, #4*(15-4)]
	bx lr

.global ThrCtxLongJmp
.hidden ThrCtxLongJmp
.align 2
ThrCtxLongJmp:
	ldr sp, [r0, #4*17]
	ldr lr, [r0, #4*18]
	ldr r1, [r0, #4*16]
	push {r1}
	ldr r1, [r0, #4*15]
	push {r1}
	cps #0x1F
	ldr sp, [r0, #4*13]
	ldr lr, [r0, #4*14]
	cps #0x13
	ldm r0, {r0-r12}
	rfeia sp!
