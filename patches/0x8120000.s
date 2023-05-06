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

; patch out restrictions on syscall 0x22 (otp_read)
.org 0x08120374
	.arm
	mov r12, #0x3
.org 0x081203C4
	.arm
	nop

; kern main thread hook
.org 0x0812A690
	.arm
	bl kern_main_hook

.if HOOK_SEMIHOSTING
.org 0x0812DD14
semihost_pocket:
	ldr r0, =semihosting_hook
	bx r0
.pool
.org 0x0812DD68
	b semihost_pocket
.endif

.if OTP_IN_MEM
; patch otp_read to just copy from memory
.org 0x0812026C
	nop
	nop
	ldr r1, =_otp_store_start
	b otp_jumpover
.pool
.org 0x0812028C
otp_jumpover:
	nop
	nop
	nop
	nop
	nop
	nop
	add r1, r8,lsl#2
	mov r2, r6
	mov r0, r7

	; BL              memcpy_krnl
.org 0x081202B4
	nop
	nop
.endif

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

; take down the hardware memory protection (XN) driver just as it's finished initializing
.org 0x8122490
    bl 0x08122374   ; __iosMemMapDisableProtection

; guru meditation patch !
.org 0x0812A134 ; prefetch abort
	mov r3, #0
	b crash_handler
.org 0x0812A1AC ; data abort
	mov r3, #1
	b crash_handler
.org 0x08129E50 ; undef inst
	mov r3, #2
	b crash_handler

; print iosPanic errors to screen
.org 0x8129AA0
	b ios_panic_print

; skip 49mb memclear for faster loading.
.org 0x812A088
	.arm
	mov r1, r5
	mov r2, r6
	mov r3, r7
	bx r4

.org CODE_BASE
kern_main_hook:
		mov r9, r2
		push {r0-r11, lr} ; orig insn
		ldr r0, =0x27f00000 ; ELF
		ldr r1, [r0] ; magic
		ldr r2, =0x7F454C46
		cmp r1, r2
		bne skip_proc

		add r1, r0, #0x18
		ldr r1, [r1] ; thread entrypoint
		add r0, r1, r0
		blx r0

skip_proc:
		pop {r0-r11, pc}


map_mem_perm_fixup:
    ldr r6, [sp, #0x4C]
    ;bics r6, 0x18   ; clear kernel-only and user read-only flags
    ;orr r6, #1  ; set execute
    str r6, [sp, #0x4C]
    bx lr

.area (0x08140000 - CODE_BASE)
.if OTP_IN_MEM
_otp_store_start:
.word 0x4F545053
.word 0x544F5245
.word 0x4F545053
.word 0x544F5245
.fill ((_otp_store_start + 0x400) - .), 0x45
_otp_store_end:
.endif

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

ios_panic_print:
	mov r4, r0

	mov r0, r4
	bl serial_send_str
	mov r0, #0xa
	bl serial_send

; r0 : str, r1 : x, r2 : y
	mov r1, #0
	mov r2, #0
	mov r0, r4
	bl drawString

	mov r0, r4
	mov r1, #64
	ldr r4, =KERNEL_SMC_SET_PANIC
	blx r4
ios_panic_loop:
	b ios_panic_loop

; this is *so* not AAPCS
exi_wait:
	ldr r7, =0x0D806800
_exi_wait:
	ldr r8, [r7]
	ands r8, r8, #8
	beq _exi_wait
	bx lr

crash_handler:
	sub sp, #4
	mov r4, r0
	mov r5, r3

	ldr r0, =FRAMEBUFFER_ADDRESS
	ldr r1, =0xFF
	ldr r2, =896*504*4
	bl KERNEL_MEMSET

	mov r0, #0
	mov r1, #0
	cmp r5, #1
	adrlt r2, crash_handler_format_prefetch
	adreq r2, crash_handler_format_data
	adrgt r2, crash_handler_format_undef
	ldr r3, [r4, #0x40]
	bl _printf

	; write32(EXI0_CSR, 0x108)
	mov r6, #0
	ldr r7, =0x0D806800
	ldr r8, =0x108
	str r8, [r7]
	; write32(EXI0_DATA, 0xA0000100)
	ldr r8, =0xA0000100
	str r8, [r7, #0x10]
	; write32(EXI0_CR, 0x35)
	ldr r8, =0x35
	str r8, [r7, #0x0C]
	bl exi_wait

	crash_handler_loop:

		mov r0, #20
		mul r1, r6, r0
		add r1, #40
		cmp r6, #10
		adrlt r2, crash_handler_format2
		adrge r2, crash_handler_format3
		add r3, r4, #4
		ldr r9, [r3, r6, lsl 2]
		str r9, [sp]
		mov r3, r6
		bl _printf

		; write32(EXI0_CSR, 0x108)
		ldr r8, =0x108
		str r8, [r7]
		; write32(EXI0_DATA, r9)
		str r9, [r7, #0x10]
		ldr r8, =0x35
		; write32(EXI0_CR, 0x35)
		str r8, [r7, #0x0C]
		bl exi_wait

		add r6, #1
		cmp r6, #0x10
		blt crash_handler_loop

		; write32(EXI0_CSR, 0)
		mov r8, #0
		str r8, [r7]

		mov r0, #400
		mov r1, #20
		adr r2, crash_handler_format4
		ldr r3, [r4, #0x40]
        cmp r5, #1
        ldreq r3, [r3, #-8]
		ldrne r3, [r3]
		bl _printf

	crash_handler_loop2:
		b crash_handler_loop2
	crash_handler_format_data:
		.ascii "GURU MEDITATION ERROR (data abort)"
		.byte 0x00
		.align 0x4
	crash_handler_format_prefetch:
		.ascii "GURU MEDITATION ERROR (prefetch abort)"
		.byte 0x00
		.align 0x4
	crash_handler_format_undef:
		.ascii "GURU MEDITATION ERROR (undefined instruction)"
		.byte 0x00
		.align 0x4
	crash_handler_format2:
		.ascii "r%d  = %08X"
		.byte 0x00
		.align 0x4
	crash_handler_format3:
		.ascii "r%d = %08X"
		.byte 0x00
		.align 0x4
	crash_handler_format4:
		.ascii "%08X"
		.byte 0x00
		.align 0x4

; r0 : x, r1 : y, r2 : format, ...
; NOT threadsafe so dont even try you idiot
_printf:
	ldr r12, =_printf_xylr
	str r0, [r12]
	str r1, [r12, #4]
	str lr, [r12, #8]
	ldr r0, =_printf_string
	mov r1, #_printf_string_end-_printf_string
	bl KERNEL_SNPRINTF

	ldr r0, =_printf_string
	bl serial_send_str
	mov r0, #0xa
	bl serial_send

	ldr r12, =_printf_xylr
	ldr r1, [r12]
	ldr r2, [r12, #4]
	ldr lr, [r12, #8]
	push {lr}
	ldr r0, =_printf_string
	bl drawString
	pop {pc}

; r0 : str, r1 : x, r2 : y
drawString:
	push {r4-r6,lr}
	mov r4, r0
	mov r5, r1
	mov r6, r2
	drawString_loop:
		ldrb r0, [r4], #1
		cmp r0, #0x00
		beq drawString_end
		mov r1, r5
		mov r2, r6
		bl drawCharacter
		add r5, #CHARACTER_SIZE
		b drawString_loop
	drawString_end:
	pop {r4-r6,pc}

; r0 : char, r1 : x, r2 : y
drawCharacter:
	subs r0, #32
	; bxlt lr
	push {r4-r7,lr}
	ldr r4, =FRAMEBUFFER_ADDRESS ; r4 : framebuffer address
	add r4, r1, lsl 2 ; add x * 4
	mov r3, #FRAMEBUFFER_STRIDE
	mla r4, r2, r3, r4
	adr r5, font_data ; r5 : character data
	add r5, r0, lsl 3 ; font is 1bpp, 8x8 => 8 bytes represents one character
	mov r1, #0xFFFFFFFF ; color
	mov r2, #0x0 ; empty color
	mov r6, #8 ; i
	drawCharacter_loop1:
		mov r3, #CHARACTER_MULT
		drawCharacter_loop3:
			mov r7, #8 ; j
			ldrb r0, [r5]
			drawCharacter_loop2:
				tst r0, #1
				; as many STRs as CHARACTER_MULT (would be nice to do this in preproc...)
				streq r1, [r4], #4
				streq r1, [r4], #4
				strne r2, [r4], #4
				strne r2, [r4], #4
				mov r0, r0, lsr #1
				subs r7, #1
				bne drawCharacter_loop2
			add r4, #FRAMEBUFFER_STRIDE-CHARACTER_SIZE*4
			subs r3, #1
			bne drawCharacter_loop3
		add r5, #1
		subs r6, #1
		bne drawCharacter_loop1
	pop {r4-r7,pc}
    

	.pool

font_data:
	.incbin "patches/font.bin"

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
