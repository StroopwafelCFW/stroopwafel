.arm.big

.open "sections/0x8120000.bin","patched_sections/0x8120000.bin",0x8120000

.include "config.s"

CODE_SECTION_BASE equ 0x08120000
CODE_SECTION_SIZE equ 0x00014000
CODE_BASE equ (CODE_SECTION_BASE + CODE_SECTION_SIZE)

RODATA_SECTION_BASE equ 0x08140000
RODATA_SECTION_SIZE equ 0x00001df0
RODATA_BASE equ (RODATA_SECTION_BASE + RODATA_SECTION_SIZE)


.if OTP_IN_MEM
; patch otp_read to just copy from memory
.org 0x08120244
	nop
	nop
	ldr r1, =_otp_store_start
	b otp_jumpover
.pool
.org 0x08120264
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
.org 0x0812028C
	nop
	nop
.endif

.org CODE_BASE

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

.endarea

.Close
