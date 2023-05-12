
.extern fw_img_path
.extern memset
.extern memcpy
.extern debug_printf

@ jump here on add r7, sp, #0x24
.global ancast_crypt_check
.align 4
ancast_crypt_check:
    .thumb
    bx pc   @ change to arm
    nop
    .arm
    ldr r7, =0x010001A0  @ device type offset
    ldrh r7, [r7]       @ get device type
    tst r7, #1          @ set bit 0 at the u16 at 0x1A0 for no-crypt mode
    bne ancast_no_crypt

    add r7, sp, #0x24   @ replaced instructions
    str r7, [sp, #0x18]
    bx lr

ancast_no_crypt:
    pop {r4-r7}
    pop {r1}
    add sp, #0x10
    mov r0, #0
    bx r1

.pool


.align 4
.global syslog_hook
syslog_hook:
    .arm
    mov r5, r2
    mov r6, r0
    mov r4, r1
    push {r4-r6, lr}
    mov r0, r4
    bl svc_sys_write_arm
    pop {r4-r6, lr}
    mov r2, r5
    mov r0, r6
    mov r1, r4
    bx lr



.align 4
hai_shift_data_offsets_t:
    .thumb
    bx pc
    nop
hai_shift_data_offsets:
    .arm
    ldr r0, [r7] @ get imginfo pointer
    ldr r1, [r0] @ get number of items (TODO: are all 3 of these nums always the same?)
    add r0, #8 @ skip straight to entries list
    
.if 1
hai_shift_loop:
    ldr r2, [r0]    @ get offset in 512k blocks
    add r2, #2
    str r2, [r0],#8 @ add usb shrink offset (two 512k blocks)
    subs r1, #1
    bge hai_shift_loop
.endif
    
    
    ldr r3, [r7, #8] @replaced
    ldr r0, [r5]
    bx lr



.align 4
.global boot_dirs_clear_t
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
    blx r2      @ rmdir(char* path) as mcp
    b clear_loop
dir_clear_ret:
    pop {r4, pc}
    
dirs_to_clear:
.word 0x5063578 @ /vol/system_slc/tmp
.word 0x5061C58 @ /vol/storage_mlc01/usr/tmp
.word system_ramdisk_cache
.word 0
system_ramdisk_cache:
.ascii "/vol/system_ram/cache" 
.byte 0
.align 4


@ r0 = location, r1 = entry
ppc_jump_stub:
    .arm
    @ lis r3, entry@h
    lsr r2, r1, #16
    orr r2, r2, #0x3C400000
    orr r2, r2, #0x200000
    str r2, [r0]

    @ ori r3, r3, entry@l
    lsl r1, r1, #16
    lsr r1, r1, #16
    orr r1, r1, #0x60000000
    orr r1, r1, #0x630000
    add r2, r0, #4
    str r1, [r2]

    @ mtsrr0 r3
    ldr r2, =0x7C7A03A6
    add r1, r0, #8
    str r2, [r1]

    @ li r3, 0
    ldr r2, =0x38600000
    add r1, r0, #0xC
    str r2, [r1]

    @ mtsrr1 r3
    ldr r2, =0x7C7B03A6
    add r1, r0, #0x10
    str r2, [r1]

    @ rfi
    ldr r2, =0x4C000064
    add r3, r0, #0x14
    str r2, [r3]

    mov r1, #0x18 @ size
    b svcFlushDCache

@ r0 = location, r1 = entry
ppc_wait_stub:
    push {lr}

    @ lis r3, entry@h
    lsr r2, r1, #16
    orr r2, r2, #0x3C400000
    orr r2, r2, #0x200000
    str r2, [r0]

    @ ori r3, r3, entry@l
    lsl r1, r1, #16
    lsr r1, r1, #16
    orr r1, r1, #0x60000000
    orr r1, r1, #0x630000
    add r2, r0, #4
    str r1, [r2]

    @ li r4, 0
    mov r1, #0x38800000
    add r3, r0, #8
    str r1, [r3]

    @ stw r4, 0(r3)
    ldr r1, =0x90830000
    add r3, r0, #0xC
    str r1, [r3]

    @ dcbf r0, r3
    ldr r1, =0x7C0018AC
    add r3, r0, #0x10
    str r1, [r3]

    @ sync
    sub r1, r1, #0x1400
    add r3, r0, #0x14
    str r1, [r3]

@ _wait:
    @ dcbi r0, r3
    ldr r3, =0x7C001BAC
    add lr, r0, #0x18
    str r3, [lr]

    @ sync
    add r3, r0, #0x1C
    str r1, [r3]

    @ lwz r4, 0(r3)
    ldr r1, =0x80830000
    add r3, r0, #0x20
    str r1, [r3]

    @ cmpwi cr7, r4, 0
    ldr r1, =0x2F840000
    add r3, r0, #0x24
    str r1, [r3]

    @ beq cr7, _wait
    ldr r1, =0x419EFFF0
    add r3, r0, #0x28
    str r1, [r3]

    @ mtsrr0 r4
    ldr r1, =0x7C9A03A6
    add r3, r0, #0x2C
    str r1, [r3]

    @ li r4, 0
    mov r1, #0x38800000
    add r3, r0, #0x30
    str r1, [r3]

    @ mtsrr1 r4
    ldr r2, =0x7C9B03A6
    add r1, r0, #0x34
    str r2, [r1]

    @ rfi
    ldr r2, =0x4C000064
    add r3, r0, #0x38
    str r2, [r3]

    mov r1, #0x3C   @ size
    pop {lr}
    b svcFlushDCache

.pool

@ TODO

.align 4
MCP_FSA_OPEN:
    .arm
    push {r7, lr}
    ldr r7, =0x05035294
    blx r7
    pop {r7, pc}
.pool

.align 4
MCP_FSA_MOUNT:
    .arm
    push {r7, lr}
    ldr r7, =0x050397B0
    blx r7
    pop {r7, pc}
.pool

.align 4
MCP_READFILE:
    .thumb
    ldr r4, =0x050170FC+1
    bx r4
.pool

.align 4
MCP_SYSLOG_OUTPUT:
    .arm
    push {r7, lr}
    ldr r7, =0x0503DCF8
    blx r7
    pop {r7, pc}
.pool

@ this runs after BSP holds the PPC in reset,
@ the signed binary is already loaded in to memory
@ this hook replaces syscall 0x5F ("init_mem1_ppc") but it sucks so who cares
@ we also use that memory so we would have to disable it anyway
ppc_hook_pre:
    .thumb
    bx pc @ change to ARM
    nop
    .arm
    push {lr}
    @ TODO: check the redir-file exists, abort if not

    @ install the wait-stub, which waits for further code
        ldr r0, =0x00001000 @ location (safe region that would've been used for vectors ("init_mem1_ppc"))
        ldr r1, =0x016FFFFC @ entry (this is actually the rom-trace code but we repurpose it)
        bl ppc_wait_stub

    pop {pc}

@ this runs immediately after BSP boots the PowerPC
@ this replaces syscall 0x5B ("flush_ipc_server"), run it later
ppc_hook_post:
    .thumb
    bx pc @ change to ARM
    nop
    .arm
    @ store registers we want to use
        push {r4-r8, lr}
        sub sp, #12

    @ retrieve the first instruction (thanks cache)
        ldr r5, =0x08000100 @ kernel start (IOP address)
        ldr r4, [r5] @ encrypted first instruction
        ldr r7, =0x016FFFFC @ rom-trace code (also serves as entry pointer)
        mov r8, #0 @ timeout counter

    @ wait until it changes
    _ppc_wait_race:
        mov r0, r5
        mov r1, #4
        bl svcInvalidateDCache
        ldr r6, [r5]
        cmp r4, r6
        @ bang!
        bne _ppc_do_race

        @ check the rom-trace code for a panic
        mov r0, r7
        mov r1, #4
        bl svcInvalidateDCache
        ldr r6, [r7]
        lsr r6, r6, #24
        cmp r6, #0
        bne _ppc_panic

        @ also count so we know wtf happened if this fails
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
    @ race it!
    @ install the jump-stub, which jumps to the wait-stub loaded earlier
        mov r0, r5 @ location
        ldr r1, =0x00001000 @ entry
        bl ppc_jump_stub

    @ wait for the PowerPC to clear the entry pointer
    _ppc_wait_entry:
        mov r0, r7
        mov r1, #4
        bl svcInvalidateDCache

        mov r0, r7
        mov r1, #0
        ldr r0, [r0]
        cmp r0, r1
        bne _ppc_wait_entry

    @ at this point, we have control over the PowerPC and it's waiting for more code

    @ mount SD
        mov r0, #0
        bl MCP_FSA_OPEN

        ldr r1, =launch_os_hook_devicepath
        ldr r2, =launch_os_hook_mountpath
        mov r3, #0
        str r3, [sp]
        str r3, [sp, #4]
        bl MCP_FSA_MOUNT

    @ map the kernel memory
        ldr r0, =0x08000000
        ldr r1, =0x120000
        bl iosLoadPpcKernel

    @ read the custom kernel.img from SD
        ldr r0, =fw_img_path
        ldr r1, =MCP_STR_KERNEL_IMG
        ldr r2, =0x08000000
        ldr r3, =0x120000
        add r4, sp, #8
        str r4, [sp]
        mov r4, #1
        str r4, [sp, #4]
        bl MCP_READFILE

    @ flush it to MEM0
        ldr r0, =0x08000000
        ldr r1, =0x120000
        bl svcFlushDCache

    @ unmap the kernel memory
        ldr r0, =0x08000000
        mov r1, #0
        bl iosLoadPpcKernel

    @ tell the PPC to jump to it!
        ldr r1, =0xFFE00100 @ kernel start (PPC address)
        mov r0, r7
        str r1, [r0]
        mov r1, #4
        bl svcFlushDCache

    @ we replace this and now its time to run it
        bl svcFlushIPCServer

    @ return to the original code
        add sp, #12
        pop {r4-r8, pc}

.pool

    .align 4
    .thumb
    .global launch_os_hook
    launch_os_hook:
        bx pc
        .align 0x4
        .arm
        push {r0-r12,lr}
        sub sp, #8

        bl MCP_SYSLOG_OUTPUT

        mov r0, #0
        bl MCP_FSA_OPEN

        adr r1, launch_os_hook_devicepath
        adr r2, launch_os_hook_mountpath
        mov r3, #0
        str r3, [sp]
        str r3, [sp, #4]
        bl MCP_FSA_MOUNT

        add sp, #8
        pop {r0-r12,pc}

.pool
launch_os_hook_devicepath:
    .ascii "/dev/sdcard01"
    .byte 0x00

.align 4
launch_os_hook_mountpath:
    .ascii "/vol/sdcard"
    .byte 0x00

.align 4
MCP_STR_KERNEL_IMG:
    .ascii "kernel.img"
    .byte 0x00
    .align 4


@
@ CRYPTO
@

.global crypto_keychange_hook
.global crypto_disable_hook

@ TODO real config.s
.equ USB_SEED_0, 0xDEADBEEF
.equ USB_SEED_1, 0xCAFEBABE
.equ USB_SEED_2, 0x42042069
.equ USB_SEED_3, 0x0BADBEEF

@ These hooks cause key 0xDEADBEEF to do no crypto, copying if needed.
@ crypto_keychange_hook sets the key to a valid handle all processes have access to, and
@ sets a flag at crypt_info + 0xC (unused) so the later hook knows to not use crypto.
@ crypto_disable_hook checks the flag we set and sets crypto mode in crypt_data+4 to 0 if needed
.align 4
crypto_keychange_hook:
    .arm
    ldr     r0, [r7, #8] @ get key in use
    ldr     r1, =0xDEADBEEF
    mov     r2, #0x6    @ wii sd key, all pids can use this
    cmp     r0, r1
    streq   r2, [r7,#8] @ update 
    streq   r1, [r7,#12]@ set hax flag
    
    mov     r0, r5      @ replaced
    bx      lr

.align 4
crypto_disable_hook:
    .arm
    ldr     r0, [r7, #12]
    ldr     r1, =0xDEADBEEF
    cmp     r0, r1
    moveq   r0, #0
    streq   r0, [r3]
    
    mov     r1, r6     @replaced
    bx lr


.align 4
usb_seedswap:
    .arm
    push {r4,r5}
    mov r0, r2
    adr r1, usb_seed
    ldmia r1, {r2-r5}
    stmia r0, {r2-r5}
    pop {r4,r5}
    
    mov r0, #0
    bx lr
    
usb_seed:
.word USB_SEED_0
.word USB_SEED_1
.word USB_SEED_2
.word USB_SEED_3
.pool


@
@ C2W
@

.equ VWII_PPC_OC, 0
.equ WIP_VWII_JUNK, 0
.equ MCP_C2W_LAUNCH_FAIL, 0x5008824

.global c2w_seeprom_hook_t
.global c2w_boot_hook_t

memset_t:
    .thumb
    bx pc
    nop
memset_thunk:
    .arm
    b memset

memcpy_t:
    .thumb
    bx pc
    nop
memcpy_thunk:
    .arm
    b memcpy

debug_printf_t:
    .thumb
    bx pc
    nop
debug_printf_thunk:
    .arm
    b debug_printf

c2w_read_otp_t:
    .thumb
    bx pc
    nop
c2w_read_otp:
    .arm
    .word 0xE7F022F0 @ UND 0x220
    bx lr

.align 4
c2w_seeprom_hook_t:
    .thumb

    @ Do the original memset, but with 0x400 of the data
    ldr r4, =0x500
    mov r2, r4
    push {lr}
    push {r0-r5}
    bl memset_t
    pop {r0-r5}

    @ Read in OTP
    push {r0-r5}
    mov r1, r0
    mov r0, #0x0
    mov r2, #0x80
    add r1, r1, r2
    add r1, r1, r2
    add r1, r1, r2 @ +0x180
    bl c2w_read_otp_t
    pop {r0-r5}

    @ Read in patch pocket
    push {r0-r5}
    ldr r1, =vwii_pocket_patches_start
    ldr r2, =vwii_pocket_end
    sub r2, r2, r1
    bl memcpy_t
    pop {r0-r5}

    push {r0-r5}
    mov r4, r0
    ldr r0, =c2w_boot_hook_print_3
    mov r1, r4
    ldr r1, [r1, #0x4]
    bl debug_printf_t
    pop {r0-r5}

    @ Shift EEPROM up 0x400
    ldr r0, =0x400
    add r5, r5, r0
    pop {pc}
.pool

.align 4
vwii_pocket_patches_start:
#include "vwii_pocket_patches.inc"
vwii_pocket_end:

.align 4
c2w_boot_hook_fixup_c2w_ptrs:
    .thumb
    push {r4-r5, lr}
    ldr r0, =0x01000000
    ldr r1, =0x01F80000
    ldr r3, =0xFFF07F00
    ldr r4, =0xFFF07B00

    @ Replace all instances of 0xFFF07F00 (Wii SEEPROM addr) with 0xFFF07B00 in c2w.elf
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
    bl debug_printf_t
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
    ldr r0, [r0] @ ios paddr
    ldr r1, =0x260000
    ldr r3, =0x0D8001EC

    add r1, r0, r1

    @ Replace OTP read routine w/ memcpy
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

    ldr r3, =0xB5F0 @ PUSH {R4-R7,LR}

    @sub r1, r0, #0x200
c2w_boot_hook_search_3:

    ldrh r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_3

    @ We found the OTP read, now to patch it and exit
    mov r4, r0
    ldr r1, =c2w_otp_replacement_t
    ldr r2, =c2w_otp_replacement_t_end
    sub r2, r2, r1
    bl memcpy_t

    ldr r0, =c2w_boot_hook_print_2
    mov r1, r4
    bl debug_printf_t

    pop {r0-r5, pc}

c2w_boot_hook_search_skip_replace_3:
    sub r0, r0, #0x2
    b c2w_boot_hook_search_3

.align 4
c2w_boot_hook_fixup_ios_ptrs:
    .thumb
    push {r4-r5, lr}

    @ Search for 0xFFFE7CC0 in IOS and change it to 0xFFFE7B00
    ldr r0, =0x1018
    ldr r0, [r0] @ ios paddr
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
    @push {r0-r5, lr}
    @mov r4, r0
    @ldr r0, =c2w_boot_hook_print_3
    @mov r1, r4
    @bl debug_printf_t
    @pop {r0-r5, r14}

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
lsl r4, r4, #0x8 @ 0xFFFE7B00
blx r4
.pool
c2w_boot_hook_fixup_ios_reload_hookcode_end:

c2w_boot_hook_ios_semihosting_hookcode_start:
.include "vwii_semihosting.s"
c2w_boot_hook_ios_semihosting_hookcode_end:


@ Look for 0x24C42380 and place a hook
.align 4
c2w_boot_hook_fixup_ios_reload:
    .thumb
    push {r4-r5, lr}

    ldr r0, =0x1018
    ldr r0, [r0] @ ios paddr
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
    bl debug_printf_t
    pop {r0-r5}

    @ We found the OTP read, now to patch it and exit
    ldr r1, =c2w_boot_hook_fixup_ios_reload_hookcode_start
    ldr r2, =c2w_boot_hook_fixup_ios_reload_hookcode_end-c2w_boot_hook_fixup_ios_reload_hookcode_start
    bl memcpy_t

    b c2w_boot_hook_search_5_done

c2w_boot_hook_search_skip_replace_5:
    add r0, r0, #0x4
    b c2w_boot_hook_search_5

c2w_boot_hook_search_5_done:
    pop {r4-r5, pc}

@ Replace SWI handler
.align 4
c2w_boot_hook_ios_semihosting:
    .thumb
    push {r4-r5, lr}

    ldr r0, =0x1018
    ldr r0, [r0] @ ios paddr
    ldr r1, =0x260000
    ldr r3, =0xE59FF018
    @ldr r4, =0xFFFE7FFC
c2w_boot_hook_search_6:
    cmp r0, r1
    bge c2w_boot_hook_search_6_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_6
    ldr r2, [r0, #0x4]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_6

    ldr r1, =ios_ffff0000 @ ios kernel sect
    str r0, [r1]

    @mov r1, #0x28
    @add r0, r0, r1
    @str r4, [r0]

    push {r0-r5}
    mov r4, r0
    ldr r0, =c2w_boot_hook_print_5
    mov r1, r4
    bl debug_printf_t
    pop {r0-r5}

    @ We found the OTP read, now to patch it and exit
    @ldr r1, =c2w_boot_hook_ios_semihosting_hookcode_start
    @ldr r2, =c2w_boot_hook_ios_semihosting_hookcode_end-c2w_boot_hook_ios_semihosting_hookcode_start
    @bl memcpy_t

    b c2w_boot_hook_search_6_done

c2w_boot_hook_search_skip_replace_6:
    add r0, r0, #0x4
    b c2w_boot_hook_search_6

c2w_boot_hook_search_6_done:
    pop {r4-r5, pc}

@ The only semihosting stuff that's actually used is write0, so we stash our semihosting code there.
.align 4
c2w_boot_hook_ios_semihosting_ledaddr:
    .thumb
    push {r4-r5, lr}

    ldr r0, =ios_ffff0000 @ ios kernel sect
    ldr r0, [r0]
    ldr r1, =0x260000
    ldr r3, =0xB5000601 @ push {lr}@ lsls r1, r0, #24
c2w_boot_hook_search_8:
    cmp r0, r1
    bge c2w_boot_hook_search_8_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_8

    mov r5, r0
    ldr r0, =ios_ffff0000 @ ios kernel sect
    ldr r0, [r0]
    sub r4, r5, r0
    ldr r0, =0xFFFF0000
    add r4, r0 @ actual pointer of the led code

    ldr r1, =ios_ledout
    add r4, r4, #0x1
    str r4, [r1]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl debug_printf_t
    pop {r0-r5}

    b c2w_boot_hook_search_8_done

c2w_boot_hook_search_skip_replace_8:
    add r0, r0, #0x4
    b c2w_boot_hook_search_8

c2w_boot_hook_search_8_done:
    pop {r4-r5, pc}

@ The only semihosting stuff that's actually used is write0, so we stash our semihosting code there.
.align 4
c2w_boot_hook_ios_semihosting_2:
    .thumb
    push {r4-r5, lr}

    ldr r0, =ios_ffff0000 @ ios kernel sect
    ldr r0, [r0]
    ldr r1, =0x260000
    ldr r3, =0x2005DFAB @ mov r0, #5@ svc 0xab
c2w_boot_hook_search_7:
    cmp r0, r1
    bge c2w_boot_hook_search_7_done
    ldr r2, [r0]
    cmp r2, r3
    bne c2w_boot_hook_search_skip_replace_7

    sub r0, #2
    mov r1, #0x3
    bic r0, r1 @ align 0x4
    mov r5, r0

    ldr r1, =c2w_boot_hook_ios_semihosting_hookcode_start
    ldr r2, =c2w_boot_hook_ios_semihosting_hookcode_end-c2w_boot_hook_ios_semihosting_hookcode_start
    bl memcpy_t

    ldr r0, =ios_ffff0000 @ ios kernel sect
    ldr r0, [r0]
    sub r4, r5, r0
    ldr r0, =0xFFFF0000
    add r4, r0 @ actual pointer of the semihosting code

    ldr r0, =ios_ffff0000 @ ios kernel sect
    ldr r0, [r0]
    @ldr r5, [r0, #0x28]
    add r4, #(_vwii_pocket_serial_send_str_t-_vwii_pocket_gpio_debug_send+1)
    str r4, [r0, #0x28]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl debug_printf_t
    pop {r0-r5}

    @ store the LED output addr
    ldr r0, =ios_ledout
    ldr r0, [r0]
    ldr r4, [r5, #(_vwii_semihosting_ledoutaddr-_vwii_pocket_gpio_debug_send)]
    str r0, [r5, #(_vwii_semihosting_ledoutaddr-_vwii_pocket_gpio_debug_send)]

    mov r0, r4
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl debug_printf_t
    pop {r0-r5}

    mov r0, r5
    push {r0-r5}
    mov r1, r0
    ldr r0, =c2w_boot_hook_print_5
    bl debug_printf_t
    pop {r0-r5}

    b c2w_boot_hook_search_7_done

c2w_boot_hook_search_skip_replace_7:
    add r0, r0, #0x4
    b c2w_boot_hook_search_7

c2w_boot_hook_search_7_done:
    pop {r4-r5, pc}

.endif @ WIP_VWII_JUNK

@  

.if VWII_PPC_OC
.align 4
c2w_oc_hax_patch:
    .thumb
    push {r4-r5, lr}
    ldr r0, =0x01000000
    ldr r1, =0x01F80000
    ldr r3, =0xE3C22020
    ldr r4, =0xE3822020

    @ Search for E3C22020 E3822020 and replace the second instruction with a nop
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
    bl debug_printf_t
    pop {r0-r5}

c2w_oc_hax_search_skip_replace:
    add r0, r0, #0x4
    b c2w_oc_hax_search

c2w_oc_hax_search_done:
    pop {r4-r5, pc}
.endif @ VWII_PPC_OC

.align 4
c2w_boot_hook_t:
    .thumb
    mov r4, r0 @ orig instr
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
.endif @ VWII_PPC_OC

    pop {r0-r7}
    pop {r1} @ bug in armips??
    cmp r4, #0x0 @ orig instr
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

ios_ffff0000:
.word 0x0
ios_ledout:
.word 0x0

.pool