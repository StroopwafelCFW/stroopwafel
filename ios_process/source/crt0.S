.section ".start"
.arm
.align 4

.extern _main
.type _main, %function

.extern memset
.type memset, %function

.extern __ios_dynamic
.type __ios_dynamic, %function

.globl _start
.type _start, %function
_start:

	@ set sp
	adr r2, _start
	ldr r1, _start_offs
	sub r2, r2, r1

	ldr r0, =__stack_start
	add r0, r2, r0
	@mov sp, r0

	@ get bss start addr
	adr r2, _start
	ldr r1, _start_offs
	sub r2, r2, r1
	ldr r0, =__bss_start__
	add r0, r2, r0
	
	@ get size (why won't GCC allow me to subtract those two ptrs? is it trying to relocate them?)
	ldr r2, =__bss_end__
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

	b _main

_start_offs:
	.word __start__