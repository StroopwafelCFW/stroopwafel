.arm.big

.open "sections/0xE0000000.bin","patched_sections/0xE0000000.bin",0xE0000000

SECTION_BASE equ 0xE0000000
SECTION_SIZE equ 0x000DB658
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)
CODE_END equ 0xe10c0000

ACP_SYSLOG_OUTPUT equ 0xE00C4D54

; Enable all logging
.org 0xE00D6DE8
.arm
;    b ACP_SYSLOG_OUTPUT
    
; skip SHA1 hash checks (used for bootMovie, bootLogoTex)
.org 0xE0030BC0
    .arm
    mov r0, #0
    bx lr

; allow any region title to be launched (results may vary!)
.org 0xE0030474
    .arm
    mov r0, #0
    bx lr

; custom code starts here
.org CODE_BASE
.area (CODE_END - CODE_BASE)

.endarea
.Close
