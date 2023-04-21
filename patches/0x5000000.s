.arm.big

.open "sections/0x5000000.bin","patched_sections/0x5000000.bin",0x05000000

SECTION_BASE equ 0x05000000
SECTION_SIZE equ 0x000598f0
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)
MCP_BSS_START equ (0x5074000 + 0x48574)

.include "config.s"

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

NEW_TIMEOUT equ (0xFFFFFFFF) ; over an hour

; fix 10 minute timeout that crashes MCP after 10 minutes of booting
.org 0x05022474
	.word NEW_TIMEOUT

; disable loading cached title list from os-launch memory
.org 0x0502E59A
	.thumb
	mov r0, #0x0

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

; hook main thread to start our thread ASAP
.org 0x05056718
	.arm
	bl mcpMainThread_hook

; patch OS launch sig check
.org 0x0500A818
	.thumb
	mov r0, #0
	mov r0, #0

; patch IOSC_VerifyPubkeySign to always succeed
.org 0x05052C44
	.arm
	mov r0, #0
	bx lr

.org 0x050282AE
	.thumb
	bl launch_os_hook

; patch pointer to fw.img loader path
.org 0x050284D8
	.word fw_img_path

; nop "COS encountered unrecoverable error..." IOS panic
.org 0x05056B84
	.arm
	nop

; allow MCP_GetSystemLog on retail
.org 0x05025EE6
	.thumb
	mov r0, #0
	nop
; same, MCP_SetDefaultTitleId
.org 0x05026BA8
	.thumb
	mov r0, #0
	nop
; same, MCP_GetDefaultTitleId
.org 0x05026C54
	.thumb
	mov r0, #0
	nop
; same, MCP_GetTitleSize
    .thumb
.org 0x502574E
    nop
    nop

; patch cert verification
.org 0x05052A90
	.arm
	mov r0, #0
	bx lr

; allow usage of u16 at ancast header + 0x1A0
.org 0x0500A7CA
	.thumb
	nop

; hook to allow decrypted ancast images
.org 0x0500A678
	.thumb
	bl ancast_crypt_check

; hooks to allow custom PPC kernel.img
;.org 0x050340A0
;	bl ppc_hook_pre
;.org 0x05034118
;	bl ppc_hook_post

; skip content hash verification
.org 0x05014CAC
.thumb
    mov r0, #0
    bx lr

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

; r0 = location, r1 = entry
ppc_jump_stub:
	.arm
	; lis r3, entry@h
	lsr r2, r1, #16
	orr r2, r2, #0x3C400000
	orr r2, r2, #0x200000
	str r2, [r0]

	; ori r3, r3, entry@l
	lsl r1, r1, #16
	lsr r1, r1, #16
	orr r1, r1, #0x60000000
	orr r1, r1, #0x630000
	add r2, r0, #4
	str r1, [r2]

	; mtsrr0 r3
	ldr r2, =0x7C7A03A6
	add r1, r0, #8
	str r2, [r1]

	; li r3, 0
	ldr r2, =0x38600000
	add r1, r0, #0xC
	str r2, [r1]

	; mtsrr1 r3
	ldr r2, =0x7C7B03A6
	add r1, r0, #0x10
	str r2, [r1]

	; rfi
	ldr r2, =0x4C000064
	add r3, r0, #0x14
	str r2, [r3]

	mov r1, #0x18 ; size
	b MCP_SVC_FLUSHDCACHE

; r0 = location, r1 = entry
ppc_wait_stub:
	push {lr}

	; lis r3, entry@h
	lsr r2, r1, #16
	orr r2, r2, #0x3C400000
	orr r2, r2, #0x200000
	str r2, [r0]

	; ori r3, r3, entry@l
	lsl r1, r1, #16
	lsr r1, r1, #16
	orr r1, r1, #0x60000000
	orr r1, r1, #0x630000
	add r2, r0, #4
	str r1, [r2]

	; li r4, 0
	mov r1, #0x38800000
	add r3, r0, #8
	str r1, [r3]

	; stw r4, 0(r3)
	ldr r1, =0x90830000
	add r3, r0, #0xC
	str r1, [r3]

	; dcbf r0, r3
	ldr r1, =0x7C0018AC
	add r3, r0, #0x10
	str r1, [r3]

	; sync
	sub r1, r1, #0x1400
	add r3, r0, #0x14
	str r1, [r3]

; _wait:
	; dcbi r0, r3
	ldr r3, =0x7C001BAC
	add lr, r0, #0x18
	str r3, [lr]

	; sync
	add r3, r0, #0x1C
	str r1, [r3]

	; lwz r4, 0(r3)
	ldr r1, =0x80830000
	add r3, r0, #0x20
	str r1, [r3]

	; cmpwi cr7, r4, 0
	ldr r1, =0x2F840000
	add r3, r0, #0x24
	str r1, [r3]

	; beq cr7, _wait
	ldr r1, =0x419EFFF0
	add r3, r0, #0x28
	str r1, [r3]

	; mtsrr0 r4
	ldr r1, =0x7C9A03A6
	add r3, r0, #0x2C
	str r1, [r3]

	; li r4, 0
	mov r1, #0x38800000
	add r3, r0, #0x30
	str r1, [r3]

	; mtsrr1 r4
	ldr r2, =0x7C9B03A6
	add r1, r0, #0x34
	str r2, [r1]

	; rfi
	ldr r2, =0x4C000064
	add r3, r0, #0x38
	str r2, [r3]

	mov r1, #0x3C   ; size
	pop {lr}
	b MCP_SVC_FLUSHDCACHE

; this runs after BSP holds the PPC in reset,
; the signed binary is already loaded in to memory
; this hook replaces syscall 0x5F ("init_mem1_ppc") but it sucks so who cares
; we also use that memory so we would have to disable it anyway
ppc_hook_pre:
	.thumb
	bx pc ; change to ARM
	nop
	.arm
	push {lr}
	; TODO: check the redir-file exists, abort if not

	; install the wait-stub, which waits for further code
		ldr r0, =0x00001000 ; location (safe region that would've been used for vectors ("init_mem1_ppc"))
		ldr r1, =0x016FFFFC ; entry (this is actually the rom-trace code but we repurpose it)
		bl ppc_wait_stub

	pop {pc}

; this runs immediately after BSP boots the PowerPC
; this replaces syscall 0x5B ("flush_ipc_server"), run it later
ppc_hook_post:
	.thumb
	bx pc ; change to ARM
	nop
	.arm
	; store registers we want to use
		push {r4-r8, lr}
		sub sp, #12

	; retrieve the first instruction (thanks cache)
		ldr r5, =0x08000100 ; kernel start (IOP address)
		ldr r4, [r5] ; encrypted first instruction
		ldr r7, =0x016FFFFC ; rom-trace code (also serves as entry pointer)
		mov r8, #0 ; timeout counter

	; wait until it changes
	_ppc_wait_race:
		mov r0, r5
		mov r1, #4
		bl MCP_SVC_INVALIDATEDCACHE
		ldr r6, [r5]
		cmp r4, r6
		; bang!
		bne _ppc_do_race

		; check the rom-trace code for a panic
		mov r0, r7
		mov r1, #4
		bl MCP_SVC_INVALIDATEDCACHE
		ldr r6, [r7]
		lsr r6, r6, #24
		cmp r6, #0
		bne _ppc_panic

		; also count so we know wtf happened if this fails
		add r8, #1
		cmp r8, #0x100000
		beq _ppc_timeout

		b _ppc_wait_race

	_ppc_panic:
		ldr r5, [r5]
		ldr r7, [r7]
		ldr r0, =0xF00F1007
		ldr r0, [r0]

	_ppc_timeout:
		ldr r5, [r5]
		ldr r7, [r7]
		ldr r0, =0xF00F1008
		ldr r0, [r0]

	_ppc_do_race:
	; race it!
	; install the jump-stub, which jumps to the wait-stub loaded earlier
		mov r0, r5 ; location
		ldr r1, =0x00001000 ; entry
		bl ppc_jump_stub

	; wait for the PowerPC to clear the entry pointer
	_ppc_wait_entry:
		mov r0, r7
		mov r1, #4
		bl MCP_SVC_INVALIDATEDCACHE

		mov r0, r7
		mov r1, #0
		ldr r0, [r0]
		cmp r0, r1
		bne _ppc_wait_entry

	; at this point, we have control over the PowerPC and it's waiting for more code

	; mount SD
		mov r0, #0
		bl MCP_FSA_OPEN

		ldr r1, =launch_os_hook_devicepath
		ldr r2, =launch_os_hook_mountpath
		mov r3, #0
		str r3, [sp]
		str r3, [sp, #4]
		bl MCP_FSA_MOUNT

	; map the kernel memory
		ldr r0, =0x08000000
		ldr r1, =0x120000
		bl MCP_SVC_MAPPPCLOAD

	; read the custom kernel.img from SD
		ldr r0, =fw_img_path
		ldr r1, =MCP_STR_KERNEL_IMG
		ldr r2, =0x08000000
		ldr r3, =0x120000
		add r4, sp, #8
		str r4, [sp]
		mov r4, #1
		str r4, [sp, #4]
		ldr r4, =MCP_READFILE_T+1
		blx r4

	; flush it to MEM0
		ldr r0, =0x08000000
		ldr r1, =0x120000
		bl MCP_SVC_FLUSHDCACHE

	; unmap the kernel memory
		ldr r0, =0x08000000
		mov r1, #0
		bl MCP_SVC_MAPPPCLOAD

	; tell the PPC to jump to it!
		ldr r1, =0xFFE00100 ; kernel start (PPC address)
		mov r0, r7
		str r1, [r0]
		mov r1, #4
		bl MCP_SVC_FLUSHDCACHE

	; we replace this and now its time to run it
		bl MCP_SVC_FLUSHIPCSERVER

	; return to the original code
		add sp, #12
		pop {r4-r8, pc}

; jump here on add r7, sp, #0x24
ancast_crypt_check:
	.thumb
	bx pc   ; change to arm
	nop
	.arm
	ldr r7, =0x10001A0  ; device type offset
	ldrh r7, [r7]	   ; get device type
	tst r7, #1		  ; set bit 0 at the u16 at 0x1A0 for no-crypt mode
	bne ancast_no_crypt

	add r7, sp, #0x24   ; replaced instructions
	str r7, [sp, #0x18]
	bx lr

ancast_no_crypt:
	pop {r4-r7, lr}
	add sp, #0x10
	mov r0, #0
	bx lr

crypt_check_ret:

	.arm
	mcpMainThread_hook:
		mov r11, r0
		push {r0-r11,lr}
		sub sp, #8

		mov r0, #0x78
		str r0, [sp] ; prio
		mov r0, #1
		str r0, [sp, #4] ; detached
		ldr r0, =wupserver_entrypoint ; thread entrypoint
		mov r1, #0 ; thread arg
		ldr r2, =wupserver_stacktop ; thread stacktop
		mov r3, #wupserver_stacktop - wupserver_stack ; thread stack size
		bl MCP_SVC_CREATETHREAD

		cmp r0, #0
		blge MCP_SVC_STARTTHREAD

		ldr r1, =0x050BD000 - 4
		str r0, [r1]

		add sp, #8
		pop {r0-r11,pc}

	.thumb
	launch_os_hook:
		bx pc
		.align 0x4
		.arm
		push {r0-r3,lr}
		sub sp, #8

		bl MCP_SYSLOG_OUTPUT_T

		mov r0, #0
		bl MCP_FSA_OPEN_T

		ldr r1, =launch_os_hook_devicepath
		ldr r2, =launch_os_hook_mountpath
		mov r3, #0
		str r3, [sp]
		str r3, [sp, #4]
		bl MCP_FSA_MOUNT_T

		add sp, #8
		pop {r0-r3,pc}
		launch_os_hook_devicepath:
			.ascii "/dev/sdcard01"
			.byte 0x00
		launch_os_hook_mountpath:
			.ascii "/vol/sdcard"
			.byte 0x00
			.align 0x4

	fw_img_path:
		.ascii "/vol/sdcard"
		.byte 0x00
		.align 0x4

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


.org 0x050BD000
	wupserver_stack:
		.fill ((wupserver_stack + 0x1000) - .), 0x00
	wupserver_stacktop:
	wupserver_bss:
		.fill ((wupserver_bss + 0x2000) - .), 0x00

.endarea
.Close

;MCP-Recovery code (more valid free code pages)
.open "sections/0x5100000.bin","patched_sections/0x5100000.bin",0x05100000
.area (0x05380000 - 0x05100000)

; append wupserver code
.org 0x5116000
	wupserver_entrypoint:
		.incbin "wupserver/wupserver.bin"
	.align 0x100

.endarea
.Close
