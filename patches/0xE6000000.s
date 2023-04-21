.arm.big

.open "sections/0xE6000000.bin","patched_sections/0xE6000000.bin",0xE6000000

SECTION_BASE equ 0xE6000000
SECTION_SIZE equ 0x00010A80
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)

.include "config.s"

BSP_SYSLOG_OUTPUT equ 0xE600E624

; skip seeprom writes in eepromDrvWriteWord for safety
.org 0xE600D010
.arm
    mov r0, #0
    bx lr
    
; nop a function used for seeprom write enable, disable, nuking (will stay in write disable)
.org 0xE600CF5C
.arm
    mov r0, #0
    bx lr

.if DISABLE_DISK_DRIVE
; no disk drive
.org 0xE60085F8
.arm
    mov r3, #2

.org 0xE6008BEC
.arm
    mov r3, #2
.endif

.Close
