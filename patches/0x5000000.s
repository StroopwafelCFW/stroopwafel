.arm.big

.open "sections/0x5000000.bin","patched_sections/0x5000000.bin",0x05000000

SECTION_BASE equ 0x05000000
SECTION_SIZE equ 0x000598f0
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)
MCP_BSS_START equ (0x5074000 + 0x48574)

.include "config.s"

.if MCP_PATCHES

MCP_C2W_LAUNCH_FAIL equ 0x5008824
MCP_MEMCMP equ 0x05054D6C
MCP_MEMCPY_T equ 0x05059018
MCP_MEMCPY equ 0x05054E54
MCP_MEMSET_T equ 0x05059020
MCP_FSA_OPEN equ 0x05035294
MCP_FSA_OPEN_T equ 0x05059160
MCP_FSA_MOUNT equ 0x050397B0
MCP_FSA_MOUNT_T equ 0x05059530
MCP_SYSLOG_OUTPUT equ 0x0503DCF8
MCP_SYSLOG_OUTPUT_T equ 0x05059140
MCP_PRINTF_T equ 0x05059200

MCP_READFILE_T equ 0x050170FC

MCP_SVC_CREATETHREAD equ 0x050567EC
MCP_SVC_STARTTHREAD equ 0x05056824

MCP_SVC_INVALIDATEDCACHE equ 0x05056A74
MCP_SVC_FLUSHDCACHE equ 0x05056A7C

MCP_SVC_MAPPPCLOAD equ 0x05056BA4
MCP_SVC_FLUSHIPCSERVER equ 0x05056AC4
MCP_STR_KERNEL_IMG equ 0x05068AEC

; hook in a few dirs to delete on mcp startup
.org 0x05016C8C
	.thumb
    push {lr}
    bl boot_dirs_clear_t
    mov r0, #0
    pop {pc}

; some selective logging function : enable all the logging !
.org 0x05055438
	.arm
	b MCP_SYSLOG_OUTPUT

.if OTP_IN_MEM
; - Wii SEEPROM gets copied to 0xFFF07F00.
; - Hai params get copied to 0xFFFE7D00.
; - We want to place our OTP at 0xFFF07C80, and our payload pocket at 0xFFF07B00
;   so we move the SEEPROM +0x400 in its buffer,
;   And repoint 0xFFF07F00->0xFFF07B00.
; - Since Hai params get copied after SEEPROM, bytes 0x200~0x3FF will get clobbered, but that's fine.
;   The maximum buffer size for SEEPROM is 0x1000 (even though it could never fit at 0xFFF07F00).

; Shift Wii SEEPROM up 0x300 and read in OTP
.org 0x05008BB6
.thumb
	bl c2w_seeprom_hook_t

.org 0x05008C18
.thumb
	mov r4, #0x50
	lsl r4, r4, #0x4

.org 0x05008810
.thumb
	bl c2w_boot_hook_t
	nop
.endif ; OTP_IN_MEM

.if USB_SHRINKSHIFT
.org 0x050078A0
.thumb
    bl hai_shift_data_offsets_t
.endif

.if SYSLOG_SEMIHOSTING_WRITE
.org 0x05055328
.arm
	bl syslog_hook
.endif
    
; CUSTOM CODE STARTS HERE
.org CODE_BASE
.area (0x05060000 - CODE_BASE)

.if SYSLOG_SEMIHOSTING_WRITE
.align 4
syslog_hook:
	.arm
	mov r5, r2
	mov r6, r0
	mov r4, r1
	push {r4-r6, lr}
	mov r0, r4
	bl svc_sys_write
	pop {r4-r6, lr}
	mov r2, r5
	mov r0, r6
	mov r1, r4
	bx lr

.endif

.align 4
svc_sys_write:
	.arm
	ldr r12, =svc_sys_write_thumb+1
	bx r12

.thumb
.align 4
svc_sys_write_thumb:
	mov r2, lr
	mov r1, r0
	mov r0, #4
	swi 0xAB ;.short 0xDFAB
	bx r2

.align 4
hai_shift_data_offsets_t:
    .thumb
    bx pc
    nop
hai_shift_data_offsets:
    .arm
    ldr r0, [r7] ; get imginfo pointer
    ldr r1, [r0] ; get number of items (TODO: are all 3 of these nums always the same?)
    add r0, #8 ; skip straight to entries list
    
.if 1
hai_shift_loop:
    ldr r2, [r0]    ; get offset in 512k blocks
    add r2, #2
    str r2, [r0],#8 ; add usb shrink offset (two 512k blocks)
    subs r1, #1
    bge hai_shift_loop
.endif
    
    
    ldr r3, [r7, #8] ;replaced
    ldr r0, [r5]
    bx lr

.if OTP_IN_MEM
c2w_read_otp_t:
	.thumb
	bx pc
	nop
c2w_read_otp:
	.arm
	.word 0xE7F022F0 ; UND 0x220
	bx lr

.align 4
c2w_seeprom_hook_t:
	.thumb

	; Do the original memset, but with 0x400 of the data
	ldr r4, =0x500
	mov r2, r4
	push {lr}
	push {r0-r5}
	bl MCP_MEMSET_T
	pop {r0-r5}

	; Read in OTP
	push {r0-r5}
	mov r1, r0
	mov r0, #0x0
	mov r2, #0x80
    add r1, r1, r2
    add r1, r1, r2
    add r1, r1, r2 ; +0x180
	bl c2w_read_otp_t
	pop {r0-r5}

    ; Read in patch pocket
    push {r0-r5}
    ldr r1, =vwii_pocket_patches_start
    ldr r2, =vwii_pocket_end-vwii_pocket_patches_start
    bl MCP_MEMCPY_T
    pop {r0-r5}

    push {r0-r5}
    mov r4, r0
    ldr r0, =c2w_boot_hook_print_3
    mov r1, r4
    ldr r1, [r1, #0x4]
    bl MCP_PRINTF_T
    pop {r0-r5}

	; Shift EEPROM up 0x400
	ldr r0, =0x400
	add r5, r5, r0
	pop {pc}
.pool

.align 4
vwii_pocket_patches_start:
.include "vwii_pocket_patches.s"
vwii_pocket_end:

.align 4
c2w_boot_hook_fixup_c2w_ptrs:
	.thumb
	push {r4-r5, lr}
	ldr r0, =0x01000000
	ldr r1, =0x01F80000
	ldr r3, =0xFFF07F00
	ldr r4, =0xFFF07B00

	; Replace all instances of 0xFFF07F00 (Wii SEEPROM addr) with 0xFFF07B00 in c2w.elf
c2w_boot_hook_search:
	cmp r0, r1
	bge c2w_boot_hook_search_done
	ldr r2, [r0]
	cmp r2, r3
	bne c2w_boot_hook_search_skip_replace
	str r4, [r0]

	push {r0-r5}
	mov r4, r0
	ldr r0, =c2w_boot_hook_print_1
	mov r1, r4
	bl MCP_PRINTF_T
	pop {r0-r5}

c2w_boot_hook_search_skip_replace:
	add r0, r0, #0x4
	b c2w_boot_hook_search

c2w_boot_hook_search_done:
	pop {r4-r5, pc}

.align 4
c2w_boot_hook_find_and_replace_otpread:
	.thumb
	push {r4-r5, lr}
	ldr r0, =0x1018
	ldr r0, [r0] ; ios paddr
	ldr r1, =0x260000
	ldr r3, =0x0D8001EC

	add r1, r0, r1

	; Replace OTP read routine w/ memcpy
c2w_boot_hook_search_2:
	cmp r0, r1
	bge c2w_boot_hook_search_2_done
	ldr r2, [r0]
	cmp r2, r3
	bne c2w_boot_hook_search_skip_replace_2

	bl c2w_boot_hook_replace_3

	b c2w_boot_hook_search_2_done
	
c2w_boot_hook_search_skip_replace_2:
	add r0, r0, #0x4
	b c2w_boot_hook_search_2

c2w_boot_hook_search_2_done:
	pop {r4-r5, pc}


.align 4
c2w_boot_hook_replace_3:
	.thumb
	push {r0-r5, lr}

	ldr r3, =0xB5F0 ; PUSH {R4-R7,LR}

	;sub r1, r0, #0x200
c2w_boot_hook_search_3:

	ldrh r2, [r0]
	cmp r2, r3
	bne c2w_boot_hook_search_skip_replace_3

	; We found the OTP read, now to patch it and exit
	mov r4, r0
	ldr r1, =c2w_otp_replacement_t
	ldr r2, =c2w_otp_replacement_t_end-c2w_otp_replacement_t
	bl MCP_MEMCPY_T

	ldr r0, =c2w_boot_hook_print_2
	mov r1, r4
	bl MCP_PRINTF_T

	pop {r0-r5, pc}

c2w_boot_hook_search_skip_replace_3:
	sub r0, r0, #0x2
	b c2w_boot_hook_search_3

.align 4
c2w_boot_hook_fixup_ios_ptrs:
	.thumb
	push {r4-r5, lr}

	; Search for 0xFFFE7CC0 in IOS and change it to 0xFFFE7B00
	ldr r0, =0x1018
	ldr r0, [r0] ; ios paddr
	ldr r1, =0x260000
	ldr r3, =0xFFFE7CC0
	ldr r4, =0xFFFE7B00
c2w_boot_hook_search_4:
	cmp r0, r1
	bge c2w_boot_hook_search_4_done
	ldr r2, [r0]
	cmp r2, r3
	bne c2w_boot_hook_search_skip_replace_4

	str r4, [r0]
	;push {r0-r5, lr}
	;mov r4, r0
	;ldr r0, =c2w_boot_hook_print_3
	;mov r1, r4
	;bl MCP_PRINTF_T
	;pop {r0-r5, r14}

	b c2w_boot_hook_search_4_done

c2w_boot_hook_search_skip_replace_4:
	add r0, r0, #0x4
	b c2w_boot_hook_search_4

c2w_boot_hook_search_4_done:
    pop {r4-r5, pc}

.if WIP_VWII_JUNK

.align 4
c2w_boot_hook_fixup_ios_reload_hookcode_start:
.thumb
mov r4, #0xc2
lsl r4, r4, #0x1
mvn r4, r4
lsl r4, r4, #0x8 ; 0xFFFE7B00
blx r4
.pool
c2w_boot_hook_fixup_ios_reload_hookcode_end:

c2w_boot_hook_ios_semihosting_hookcode_start:
.include "vwii_semihosting.s"
c2w_boot_hook_ios_semihosting_hookcode_end:


; Look for 0x24C42380 and place a hook
.align 4
c2w_boot_hook_fixup_ios_reload:
    .thumb
    push {r4-r5, lr}

    ldr r0, =0x1018
    ldr r0, [r0] ; ios paddr
    ldr r1, =0x260000
    ldr r3, =0x24C42380 
c2w_boot_hook_search_5:
    cmp r0, r1
    bge c2w_boot_hook_search_5_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_5

    push {r0-r5}
    mov r4, r0
    ldr r0, =c2w_boot_hook_print_4
    mov r1, r4
    bl MCP_PRINTF_T
    pop {r0-r5}

    ; We found the OTP read, now to patch it and exit
    ldr r1, =c2w_boot_hook_fixup_ios_reload_hookcode_start
    ldr r2, =c2w_boot_hook_fixup_ios_reload_hookcode_end-c2w_boot_hook_fixup_ios_reload_hookcode_start
    bl MCP_MEMCPY_T

    b c2w_boot_hook_search_5_done

c2w_boot_hook_search_skip_replace_5:
    add r0, r0, #0x4
    b c2w_boot_hook_search_5

c2w_boot_hook_search_5_done:
	pop {r4-r5, pc}

; Replace SWI handler
.align 4
c2w_boot_hook_ios_semihosting:
    .thumb
    push {r4-r5, lr}

    ldr r0, =0x1018
    ldr r0, [r0] ; ios paddr
    ldr r1, =0x260000
    ldr r3, =0xE59FF018
    ;ldr r4, =0xFFFE7FFC
c2w_boot_hook_search_6:
    cmp r0, r1
    bge c2w_boot_hook_search_6_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_6
    ldr r2, [r0, #0x4]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_6

    ldr r1, =ios_ffff0000 ; ios kernel sect
    str r0, [r1]

    ;mov r1, #0x28
    ;add r0, r0, r1
    ;str r4, [r0]

    push {r0-r5}
    mov r4, r0
    ldr r0, =c2w_boot_hook_print_5
    mov r1, r4
    bl MCP_PRINTF_T
    pop {r0-r5}

    ; We found the OTP read, now to patch it and exit
    ;ldr r1, =c2w_boot_hook_ios_semihosting_hookcode_start
    ;ldr r2, =c2w_boot_hook_ios_semihosting_hookcode_end-c2w_boot_hook_ios_semihosting_hookcode_start
    ;bl MCP_MEMCPY_T

    b c2w_boot_hook_search_6_done

c2w_boot_hook_search_skip_replace_6:
    add r0, r0, #0x4
    b c2w_boot_hook_search_6

c2w_boot_hook_search_6_done:
    pop {r4-r5, pc}

; The only semihosting stuff that's actually used is write0, so we stash our semihosting code there.
.align 4
c2w_boot_hook_ios_semihosting_ledaddr:
    .thumb
    push {r4-r5, lr}

    ldr r0, =ios_ffff0000 ; ios kernel sect
    ldr r0, [r0]
    ldr r1, =0x260000
    ldr r3, =0xB5000601 ; push {lr}; lsls r1, r0, #24
c2w_boot_hook_search_8:
    cmp r0, r1
    bge c2w_boot_hook_search_8_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_8

    mov r5, r0
    ldr r0, =ios_ffff0000 ; ios kernel sect
    ldr r0, [r0]
    sub r4, r5, r0
    ldr r0, =0xFFFF0000
    add r4, r0 ; actual pointer of the led code

    ldr r1, =ios_ledout
    add r4, r4, #0x1
    str r4, [r1]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl MCP_PRINTF_T
    pop {r0-r5}

    b c2w_boot_hook_search_8_done

c2w_boot_hook_search_skip_replace_8:
    add r0, r0, #0x4
    b c2w_boot_hook_search_8

c2w_boot_hook_search_8_done:
    pop {r4-r5, pc}

; The only semihosting stuff that's actually used is write0, so we stash our semihosting code there.
.align 4
c2w_boot_hook_ios_semihosting_2:
    .thumb
    push {r4-r5, lr}

    ldr r0, =ios_ffff0000 ; ios kernel sect
    ldr r0, [r0]
    ldr r1, =0x260000
    ldr r3, =0x2005DFAB ; mov r0, #5; svc 0xab
c2w_boot_hook_search_7:
    cmp r0, r1
    bge c2w_boot_hook_search_7_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_7

    sub r0, #2
    mov r1, #0x3
    bic r0, r1 ; align 0x4
    mov r5, r0

    ldr r1, =c2w_boot_hook_ios_semihosting_hookcode_start
    ldr r2, =c2w_boot_hook_ios_semihosting_hookcode_end-c2w_boot_hook_ios_semihosting_hookcode_start
    bl MCP_MEMCPY_T

    ldr r0, =ios_ffff0000 ; ios kernel sect
    ldr r0, [r0]
    sub r4, r5, r0
    ldr r0, =0xFFFF0000
    add r4, r0 ; actual pointer of the semihosting code

    ldr r0, =ios_ffff0000 ; ios kernel sect
    ldr r0, [r0]
    ;ldr r5, [r0, #0x28]
    add r4, #(_vwii_pocket_serial_send_str_t-_vwii_pocket_gpio_debug_send+1)
    str r4, [r0, #0x28]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl MCP_PRINTF_T
    pop {r0-r5}

    ; store the LED output addr
    ldr r0, =ios_ledout
    ldr r0, [r0]
    ldr r4, [r5, #(_vwii_semihosting_ledoutaddr-_vwii_pocket_gpio_debug_send)]
    str r0, [r5, #(_vwii_semihosting_ledoutaddr-_vwii_pocket_gpio_debug_send)]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl MCP_PRINTF_T
    pop {r0-r5}

    mov r0, r5
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl MCP_PRINTF_T
    pop {r0-r5}

    b c2w_boot_hook_search_7_done

c2w_boot_hook_search_skip_replace_7:
    add r0, r0, #0x4
    b c2w_boot_hook_search_7

c2w_boot_hook_search_7_done:
    pop {r4-r5, pc}

.endif ; WIP_VWII_JUNK

;  

.if VWII_PPC_OC
.align 4
c2w_oc_hax_patch:
	.thumb
	push {r4-r5, lr}
	ldr r0, =0x01000000
	ldr r1, =0x01F80000
	ldr r3, =0xE3C22020
	ldr r4, =0xE3822020

	; Search for E3C22020 E3822020 and replace the second instruction with a nop
c2w_oc_hax_search:
	cmp r0, r1
	bge c2w_oc_hax_search_done
	ldr r2, [r0]
	cmp r2, r3
	bne c2w_oc_hax_search_skip_replace
	ldr r2, [r0, #0x4]
	cmp r2, r4
	bne c2w_oc_hax_search_skip_replace
	mov r4, #0x0
	str r4, [r0, #0x4]

	push {r0-r5}
	mov r4, r0
	ldr r0, =c2w_boot_hook_print_1
	mov r1, r4
	bl MCP_PRINTF_T
	pop {r0-r5}

c2w_oc_hax_search_skip_replace:
	add r0, r0, #0x4
	b c2w_oc_hax_search

c2w_oc_hax_search_done:
	pop {r4-r5, pc}
.endif ; VWII_PPC_OC

.align 4
c2w_boot_hook_t:
	.thumb
	mov r4, r0 ; orig instr
	push {lr}
	push {r0-r7}

	bl c2w_boot_hook_fixup_c2w_ptrs
	bl c2w_boot_hook_find_and_replace_otpread
	bl c2w_boot_hook_fixup_ios_ptrs
.if WIP_VWII_JUNK
    bl c2w_boot_hook_fixup_ios_reload
    bl c2w_boot_hook_ios_semihosting
    bl c2w_boot_hook_ios_semihosting_ledaddr
    bl c2w_boot_hook_ios_semihosting_2
.endif
.if VWII_PPC_OC
	bl c2w_oc_hax_patch
.endif ; VWII_PPC_OC

	pop {r0-r7}
	pop {r1} ; bug in armips??
	cmp r4, #0x0 ; orig instr
	bne c2w_boot_hook_fail
	bx r1

c2w_boot_hook_fail:
	ldr r0, =MCP_C2W_LAUNCH_FAIL+1
	bx r0


c2w_boot_hook_print_1:
.ascii "number 1 found at: %08x"
.byte 0xa 
.byte 0

c2w_boot_hook_print_2:
.ascii "OTP read fn found at: %08x"
.byte 0xa 
.byte 0

c2w_boot_hook_print_3:
.ascii "nubmer 3 found at: %08x"
.byte 0xa 
.byte 0

c2w_boot_hook_print_4:
.ascii "nubmer 4 found at: %08x"
.byte 0xa 
.byte 0

c2w_boot_hook_print_5:
.ascii "nubmer 5 found at: %08x"
.byte 0xa 
.byte 0

.pool
.endif ; OTP_IN_MEM

.align 4
boot_dirs_clear_t:
    .thumb
    bx pc
    nop
boot_dirs_clear:
    .arm
    push {r4, lr}
    adr r4, dirs_to_clear
clear_loop:
    ldr r0, [r4],#4
    cmp r0, #0
    beq dir_clear_ret
    ldr r2, =0x05016AF9
    blx r2      ; rmdir(char* path) as mcp
    b clear_loop
dir_clear_ret:
    pop {r4, pc}
    
dirs_to_clear:
.word 0x5063578 ; /vol/system_slc/tmp
.word 0x5061C58 ; /vol/storage_mlc01/usr/tmp
.word system_ramdisk_cache
.word 0
system_ramdisk_cache:
.ascii "/vol/system_ram/cache" 
.byte 0
.align 4

	.pool

.endarea
.Close

; MCP bss
.create "patched_sections/0x5074000.bin",0x5074000

.org MCP_BSS_START
.area (0x050C0000 - MCP_BSS_START)
ios_ffff0000:
.word 0x0
ios_ledout:
.word 0x0
; 0xA8C of space here to stack on 550!

.align 0x10
process_stack:
	.fill ((process_stack + 0x200) - .), 0x00
process_stacktop:

.endarea
.Close

;MCP-Recovery code (more valid free code pages)
.open "sections/0x5100000.bin","patched_sections/0x5100000.bin",0x05100000
.area (0x05380000 - 0x05100000)


.endarea

.endif
.Close
