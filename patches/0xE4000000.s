.arm.big

.open "sections/0xE4000000.bin","patched_sections/0xE4000000.bin",0xE4000000

SECTION_BASE equ 0xE4000000
SECTION_SIZE equ 0x00019704
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)
CODE_END equ 0xE4040000

; Tell TEST we're debug
.org 0xE4016A78
.arm
;    mov r0, #0

; enable cafeos event log (CEL)
.org 0xE40079E4
.arm
;    nop
; fixup shutdown
.org 0xE4007904
.arm
;    mov r0, #0xA

; enable system profiler
.org 0xE4006648
.arm
;    mov r0, #0xA

    

; stuff extra code here
.org CODE_BASE
.area (CODE_END - CODE_BASE)

.endarea
.Close
