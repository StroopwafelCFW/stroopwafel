.arm.big

.open "sections/0x10800000.bin","patched_sections/0x10800000.bin",0x10800000

.include "config.s"

.org 0x10800000

.if FAT32_USB
.org 0x108015B0
    .ascii "ext"
    .byte 0x0
.endif

.Close
