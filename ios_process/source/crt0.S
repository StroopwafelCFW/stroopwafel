.section ".start"
.arm
.align 4

.extern kern_main
.type kern_main, %function

.extern mcp_main
.type mcp_main, %function

.extern memset
.type memset, %function

.extern __ios_dynamic
.type __ios_dynamic, %function

.globl kern_entry
.globl mcp_entry
.globl _start
.type _start, %function
_start:
kern_entry:
	b kern_setup
mcp_entry:
	b mcp_setup
	nop
	nop

kern_setup:
	ldr sp, =(0x28000000-0x80)
	@ get bss start addr
	adr r2, _start
	ldr r1, _start_offs
	sub r2, r2, r1
	ldr r0, =__bss_start__
	add r0, r2, r0
	
	@ get size (why won't GCC allow me to subtract those two ptrs? is it trying to relocate them?)
	ldr r2, =__bss_end__-4
	ldr r1, =__bss_start__
	sub r2, r2, r1

	mov r1, #0x00

1:  subs r2, r2, #4
    str  r1, [r0], #4
    bne  1b

    @ set up all of the relocations
	adr r0, _start
	ldr r1, _start_offs
	sub r0, r0, r1
	ldr r1, =__dynamic_start__
	add r1, r0
	bl __ios_dynamic

	bl kern_main
	ldr r0, =0xFFFF0000
	bx r0

.pool

mcp_setup:
	mov r11, r0
	push {r0-r11,lr}
	bl mcp_main
	pop {r0-r11,pc}

_start_offs:
	.word __start__

.pool