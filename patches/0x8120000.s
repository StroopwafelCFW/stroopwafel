.arm.big

.open "sections/0x8120000.bin","patched_sections/0x8120000.bin",0x8120000

.include "config.s"

CODE_SECTION_BASE equ 0x08120000
CODE_SECTION_SIZE equ 0x00015000
CODE_BASE equ (CODE_SECTION_BASE + CODE_SECTION_SIZE)

RODATA_SECTION_BASE equ 0x08140000
RODATA_SECTION_SIZE equ 0x00002478
RODATA_BASE equ (RODATA_SECTION_BASE + RODATA_SECTION_SIZE)

KERNEL_BSS_START equ (0x8150000 + 0x61230)

FRAMEBUFFER_ADDRESS equ (0x14000000+0x38C0000)
; FRAMEBUFFER_ADDRESS equ (0x00708000 + 0x1B9000)
FRAMEBUFFER_STRIDE equ (0xE00)
CHARACTER_MULT equ (2)
CHARACTER_SIZE equ (8*CHARACTER_MULT)

KERNEL_MEMSET equ 0x08131DA0
KERNEL_SNPRINTF equ 0x08132988
KERNEL_MCP_IOMAPPINGS_STRUCT equ 0x08140DE0

KERNEL_SMC_SET_PANIC equ 0xFFFFDF5C

.if HOOK_SEMIHOSTING
.org 0x0812DD14
semihost_pocket:
	ldr r0, =semihosting_hook
	bx r0
.pool
.org 0x0812DD68
	b semihost_pocket
.endif ; HOOK_SEMIHOSTING

; patch domains
.org 0x081253C4
	str r3, [r7,#0x10]
	str r3, [r7,#0x0C]
	str r3, [r7,#0x04]
	str r3, [r7,#0x14]
	str r3, [r7,#0x08]
	str r3, [r7,#0x34]

; ARM MMU Access Permissions patches
; set AP bits for 1MB pages to r/w for all
.org 0x08124678
    mov r6, #3
; set AP bits for section descriptors to r/w for all
.org 0x08125270
    orr r3, #0x650
; set AP bits for second-level table entries to be r/w for all
.org 0x08124D88
    mov r12, #3

.org 0x081223FC
	bx lr

; take down the hardware memory protection (XN) driver just as it's finished initializing
.org 0x8122490
    bl 0x08122374   ; __iosMemMapDisableProtection

.org CODE_BASE

.area (0x08140000 - CODE_BASE)

.if GPIO_SERIAL_CONSOLE
.align 4
.arm
delay_ticks:
	ldr r2, =0x0d800010
	ldr r3, [r2] ; now
	add r0, r3, r0 ; then

	cmp r0, r3 ; then < now?
	bge delay_ticks_loop2

delay_ticks_loop1:
	ldr r1, [r2] ; cur_read
	cmp r1, r3 ; cur, now
	bge delay_ticks_loop1

delay_ticks_loop2:
	ldr r3, [r2] ; now
	cmp r3, r0 ; now < then
	blt delay_ticks_loop2
    
    bx lr

gpio_debug_send:
	LDR     R2, =0xD8000FC
	MOV     R3, #0xFF0000
	LDR     R1, [R2]
	BIC     R1, R1, R3
	STR     R1, [R2]
	SUB     R2, R2, #0x20 ; ' '
	LDR     R1, [R2]
	ORR     R1, R1, R3
	STR     R1, [R2]
	ADD     R2, R2, #8
	LDR     R1, [R2]
	ORR     R1, R1, R3
	STR     R1, [R2]
	MOV     R0, R0,LSL#16
	SUB     R2, R2, #4
	LDR     R1, [R2]
	BIC     R1, R1, R3
	ORR     R1, R1, R0
	STR     R1, [R2]
	MOV     R0, #2
	b delay_ticks

serial_send:
	PUSH    {R4-R6,LR}
	MOV     R4, R0
	MOV     R6, #7
serial_send_loop1:
	MOV     R5, R4,ASR R6
	MOV     R0, #0
	BL      gpio_debug_send
	AND     R5, R5, #1
	MOV     R0, #2
	BL      delay_ticks
	MOV     R0, R5
	BL      gpio_debug_send
	MOV     R0, #2
	BL      delay_ticks
	ORR     R0, R5, #0x80
	BL      gpio_debug_send
	MOV     R0, #2
	BL      delay_ticks
	MOV     R0, R5
	BL      gpio_debug_send
	MOV     R0, #2
	BL      delay_ticks
	SUBS    R6, R6, #1
	BCS     serial_send_loop1

	AND     R4, R4, #1
	MOV     R0, #0xFFFFFF80
	ORR     R0, R4, R0
	AND     R0, R0, #0x81
	BL      gpio_debug_send
	MOV     R0, #2
	BL      delay_ticks
	MOV     R0, R4
	BL      gpio_debug_send
	MOV     R0, #2
	BL      delay_ticks
	POP     {R4-R6,LR}
	B       serial_force_terminate

serial_force_terminate:
	push {r4,lr}
	mov r0, #0xF
    bl gpio_debug_send
    mov r0, #2
    bl delay_ticks

    mov r0, #0x8F
    bl gpio_debug_send
    mov r0, #2
    bl delay_ticks

    mov r0, #0xF
    bl gpio_debug_send
    mov r0, #2
    bl delay_ticks

    mov r0, #0x0
    bl gpio_debug_send
    mov r0, #2
    bl delay_ticks

    pop {r4,lr}
    bx lr

serial_irq_dis:
.word 0xE10F1000 ;MRS             R1, CPSR
AND             R0, R1, #0xC0
ORR             R1, R1, #0xC0
.word 0xE121F001 ;MSR             CPSR_c, R1
BX              LR

serial_irq_en:
.word 0xE10F1000 ;MRS             R1, CPSR
BIC             R1, R1, #0xC0
ORR             R1, R1, R0
.word 0xE121F001 ;MSR             CPSR_c, R1
BX              LR

.endif ; GPIO_SERIAL_CONSOLE

.align 4
serial_send_str:
.if GPIO_SERIAL_CONSOLE
	push {r4-r5,lr}
	mov r4, r0
	bl serial_irq_dis
	mov r5, r0
serial_send_str_loop1:
	ldrb r0, [r4]
	cmp r0, #0x0
	beq serial_send_str_loop1_end
	bl serial_send
	add r4, r4, #0x1
	b serial_send_str_loop1

serial_send_str_loop1_end:
	mov r0, r5
	bl serial_irq_en
	pop {r4-r5,lr}
.endif
    bx lr

semihosting_hook:
	ldr r0, [sp, #0x0]
	ldr r1, [sp, #0x4]

	cmp r0, #0x4
	beq semihosting_write0

semihosting_return:
	; POP {R0-R12,PC}^
	.word 0xE8FD9FFF

semihosting_write0:
	mov r0, r1
	bl serial_send_str
	b semihosting_return

.pool
    .endarea
.Close


.open "sections/0x8140000.bin","patched_sections/0x8140000.bin",0x8140000

.org KERNEL_MCP_IOMAPPINGS_STRUCT
	.word mcpIoMappings_patch ; ptr to iomapping structs
	.word 0x00000003 ; number of iomappings
	.word 0x00000001 ; pid (MCP)

.org RODATA_BASE
	mcpIoMappings_patch:
		; mapping 1
			.word 0x0D000000 ; vaddr
			.word 0x0D000000 ; paddr
			.word 0x001C0000 ; size
			.word 0x00000000 ; ?
			.word 0x00000003 ; ?
			.word 0x00000000 ; ?
		; mapping 2
			.word 0x0D800000 ; vaddr
			.word 0x0D800000 ; paddr
			.word 0x001C0000 ; size
			.word 0x00000000 ; ?
			.word 0x00000003 ; ?
			.word 0x00000000 ; ?
		; mapping 3
			.word 0x0C200000 ; vaddr
			.word 0x0C200000 ; paddr
			.word 0x00100000 ; size
			.word 0x00000000 ; ?
			.word 0x00000003 ; ?
			.word 0x00000000 ; ?

.Close


.create "patched_sections/0x8150000.bin",0x8150000
.area (0x081C0000 - 0x8150000)
.org KERNEL_BSS_START
	_printf_xylr:
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
	_printf_newline:
		.byte 0x0a
		.byte 0x00
		.byte 0x00
		.byte 0x00
	_printf_string:
		.fill ((_printf_string + 0x100) - .), 0x00
	_printf_string_end:
	.align 0x40

.endarea
.Close
