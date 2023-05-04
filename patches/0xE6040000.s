.arm.big

.open "sections/0xE6040000.bin","patched_sections/0xE6040000.bin",0xE6040000

.include "config.s"

.org 0xE6040000

ORIG_SPLL_CONFIG equ 0
OC_SPLL_CONFIG equ 0
OC_UPLL_CONFIG equ 0

; 27 * (0x285ED0 / 0x10000) / (0+1) / (0x4/2)
; SPLL config (GPU)
.if ORIG_SPLL_CONFIG
.org 0xE6040483
.word 0
.word 1
.word 1
.word 1
.word 0
.word 0
.word 0
.hword 0
.hword 0x28
.hword 0x2F68
.hword 4
.hword 4
.hword 0
.hword 0x1C2
.hword 0
.hword 7
.word 4
.word 0
.endif ; ORIG_SPLL_CONFIG

.if OC_SPLL_CONFIG
.org 0xE6040483
.word 0
.word 1
.word 1
.word 1
.word 0
.word 0
.word 0
.hword 0
.hword 0x01
.hword 0x2F68
.hword 4
.hword 4
.hword 0
.hword 0x1C2
.hword 0
.hword 7
.word 4
.word 0
.endif ; OC_SPLL_CONFIG

.if OC_UPLL_CONFIG
.org 0xE60404B9
.word 0
.word 1
.word 1
.word 0
.word 0
.word 0
.word 1
.hword 0
.hword 0x1
.hword 0x0
.hword 0x8
.hword 0xA
.hword 0x54
.hword 0x1C2
.hword 0
.hword 0xD
.word 8
.word 0
.endif ; 

.Close
