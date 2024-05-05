#include "config.h"
#include "addrs_55x.h"

.extern fw_img_path
.extern memset
.extern memcpy
.extern debug_printf
.extern font_bin
.extern msleep
.extern snprintf
.extern iosCreateSemaphore
.extern iosWaitSemaphore
.extern iosSignalSemaphore
.extern iosDestroySemaphore

.extern otp_ptr
.globl otp_read_replace_hacky
.globl otp_read_replace_hacky_end
otp_read_replace_hacky:
     ldr r3, =otp_ptr
     ldr r3, [r3]
     add r0, r3, r0,lsl#2
     add r2, r1, r2

otp_read_replace_hacky_label1:
     cmp r1, r2
     bne otp_read_replace_hacky_label2
     mov r0, #0
     bx lr

otp_read_replace_hacky_label2:
     ldrb r3, [r0],#1
     strb r3, [r1],#1
     b otp_read_replace_hacky_label1
.pool
otp_read_replace_hacky_end:
.word 0x0

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
hai_shift_data_offsets_t:
    .thumb
    bx pc
    nop
hai_shift_data_offsets:
    .arm
    ldr r0, [r7] @ get imginfo pointer
    ldr r1, [r0] @ get number of items (TODO: are all 3 of these nums always the same?)
    add r0, #8 @ skip straight to entries list
    
#if 0
hai_shift_loop:
    ldr r2, [r0]    @ get offset in 512k blocks
    add r2, #2
    str r2, [r0],#8 @ add usb shrink offset (two 512k blocks)
    subs r1, #1
    bge hai_shift_loop
#endif
    
    
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
    b iosFlushDCache

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
    b iosFlushDCache

.pool

@ TODO

.align 4
MCP_IOS_SHUTDOWN:
    .arm
    push {r7, lr}
    ldr r7, =0x05056b7c
    blx r7
    pop {r7, pc}
.pool

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
        bl iosInvalidateDCache
        ldr r6, [r5]
        cmp r4, r6
        @ bang!
        bne _ppc_do_race

        @ check the rom-trace code for a panic
        mov r0, r7
        mov r1, #4
        bl iosInvalidateDCache
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
        bl iosInvalidateDCache

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
        bl iosFlushDCache

    @ unmap the kernel memory
        ldr r0, =0x08000000
        mov r1, #0
        bl iosLoadPpcKernel

    @ tell the PPC to jump to it!
        ldr r1, =0xFFE00100 @ kernel start (PPC address)
        mov r0, r7
        str r1, [r0]
        mov r1, #4
        bl iosFlushDCache

    @ we replace this and now its time to run it
        bl iosFlushIPCServer

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

.equ MCP_C2W_LAUNCH_FAIL, 0x5008824

.global c2w_otp_replacement_t
.global c2w_otp_replacement_t_end
.global c2w_seeprom_hook_t
.global c2w_boot_hook_t
.extern c2w_patches

c2w_patches_t:
    .thumb
    bx pc
    .align 4
    .arm
    b c2w_patches

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

#if WIP_VWII_JUNK

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

#endif // WIP_VWII_JUNK

.align 4
c2w_boot_hook_t:
    .thumb
    mov r4, r0 @ orig instr
    push {lr}
    push {r0-r7}

    bl c2w_patches_t

#if WIP_VWII_JUNK
    bl c2w_boot_hook_fixup_ios_reload
    bl c2w_boot_hook_ios_semihosting
    bl c2w_boot_hook_ios_semihosting_ledaddr
    bl c2w_boot_hook_ios_semihosting_2
#endif

    pop {r0-r7}
    pop {r1}
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

@
@ FS
@

.arm

.global wfs_crypto_hook
.global createDevThread_hook
.global scfm_try_slc_cache_migration
.global usbRead_patch
.global usbWrite_patch
.global usb_sector_spoof
.global opendir_hook
.global fsaopen_fullstr_dump_hook
.global slcRead1_patch
.global slcRead2_patch
.global slcWrite1_patch
.global slcWrite2_patch
.global sdcardRead_patch
.global sdcardWrite_patch
.global getMdDeviceById_hook
.global registerMdDevice_hook

@ our own custom code starts here

#if MLC_ACCELERATE

.align 4
scfm_try_slc_cache_migration:
    ldr r0, [r3] @ do replaced instruction and push result for later
    push {r0, r4-r6, lr}
    sub sp, #0x10
    ldr r4, =SCFM_FSA_HANDLE_PTR
    ldr r4, [r4]
    
    @ try to mount slcs
    mov r0, r4
    bl scfm_mount_slcs
    cmp r0, #0
    movne r1, #1 @ Stage 1 error: mounting SLCs
    bne scfm_migrate_error
    
    @ try to open scfm.img on slccmpt. if it does not exist, try to migrate it.
    mov r3, sp @ u32* file_handle
    ldr r2, =str_r @ mode
    ldr r1, =str_cmpt_scfm_img @ fname
    mov r0, r4  @ fsa handle
    bl REL_FS_FSAOPENFILE
    
    cmp r0, #0
    beq scfm_migrate_cmpt_exists
    
    @ throw an exception for any err that's not file not found
    ldr r1, =FSA_STATUS_NOT_FOUND
    cmp r0, r1
    movne r1, #2 @ Stage 2 error: FSAOpenFile of /vol/scfm_cmpt/scfm.img failed
    bne scfm_migrate_error
    
scfm_try_do_migration:
    @ file not found on cmpt - try to migrate!
    mov r0, r4
    bl scfm_do_migration
    cmp r0, #0
    movne r1, #9    @ stage 9: panic for migration fail, see syslog
    bne scfm_migrate_error
    
scfm_migrate_patch_for_slccmpt:
    @ Migration succeeded! patch up paths and file names
    
    @ patch /dev/slc01 to /dev/slccmpt01
    ldr r0, =0x10831510
    ldr r1, =str_dev_slccmpt
    bl scfm_migration_strcpy
    
    @ have scfmInit look for slccmpt instead of slc by tweaking char*'s
    @ (slccmpt appears 0x8 before the existing char*, so cheat by just subtracting)
    ldr r0, =0x107E8630
    ldr r1, [r0]
    sub r1, #8
    str r1, [r0]
    
    @ Try deleting slc scfm.img, as it's not needed at this point
    ldr r1, =str_mnt_scfm_img
    mov r0, r4
    bl REL_FS_FSAREMOVEFILE
    
    ldr r0, =str_migrate_success
    bl REL_FS_SYSLOG_OUTPUT
    
    @ scfm on slccmpt is ready to go!
    b scfm_migrate_return
    
scfm_migrate_cmpt_exists:
    @ file exists, verify it and migrate if it's bad
    @ (TODO: more checking? size check is sufficient for restarting partial migrations...)
    
    @ try to read the highest offset u32 to make sure file size is ok
    ldr r5, [sp] @ file handle in r5
    ldr r0, =0x08003FFC
    str r0, [sp]   @ offset
    str r5, [sp, #4] @ file (src)
    mov r0, #0x20 @ nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  @ fsa handle
    add r1, sp, #0xC  @ buf
    mov r2, #1  @ size
    mov r3, #4  @ count
    bl REL_FS_FSAREADFILEOFFSET
    mov r6, r0
    
    mov r1, r5
    mov r0, r4
    bl REL_FS_FSACLOSEFILE
    cmp r6, #4
    beq scfm_migrate_patch_for_slccmpt
    
    @ bad file read size (probably partial migration.) try deleting and migrating again.
    ldr r1, =str_cmpt_scfm_img
    mov r0, r4
    bl REL_FS_FSAREMOVEFILE
    cmp r0, #0
    beq scfm_try_do_migration
    movne r1, #20       @ Stage 20: slccmpt scfm.img exists, but is invalid and cannot be deleted

@ return here to silently default back to slc scfm.img
scfm_migrate_error:
    bl scfm_migration_throw_error
    
scfm_migrate_return: 
    mov r0, r4
    bl scfm_unmount_slcs
    add sp, #0x10
    pop {r0, r4-r6, pc}
    
@ main function doing the actual migration
scfm_do_migration:
    push {r4-r9,lr}
    sub sp, #0x10
    mov r4, r0 @ fsa handle in r4
    mov r5, #-1  @ invalid file handles and null file data ptr by default
    mov r6, #-1
    mov r7, #0
    
    @ Open scfm.img on slc for read
    mov r3, sp @ u32* file_handle
    ldr r2, =str_r @ mode
    ldr r1, =str_mnt_scfm_img @ fname
    mov r0, r4  @ fsa handle
    bl REL_FS_FSAOPENFILE
    cmp r0, #0
    movne r1, #4        @ Stage 4: couldn't open scfm.img on slc for transfer
    bne scfm_do_migration_fail_ret
    
    @ Open scfm.img on slccmpt for write
    add r3, sp, #4 @ u32* file_handle
    ldr r2, =str_wplus @ mode
    ldr r1, =str_cmpt_scfm_img @ fname
    mov r0, r4  @ fsa handle
    bl REL_FS_FSAOPENFILE
    cmp r0, #0
    movne r1, #5        @ Stage 5: couldn't open scfm.img on slccmpt for transfer
    bne scfm_do_migration_fail_ret
    
    ldr r5, [sp]    @ source handle in r5
    ldr r6, [sp, #4]@ dst handle in r6
    
.equ SCFM_MIGRATION_BLK_SIZE, (1*1024*1024)

    @ acquire a buffer for our block copy
    mov r0, #1  @ shared heap
    mov r1, #SCFM_MIGRATION_BLK_SIZE
    mov r2, #0x40
    bl REL_FS_IOS_MEMALIGN
    cmp r0, #0
    moveq r1, #6        @ Stage 6: failed to alloc 4MB for scfm.img copy
    beq scfm_do_migration_fail_ret
    
    mov r7, r0  @ allocated buffer in r7
    
    @ TODO: read file size instead of assuming
    ldr r8, =0x8004000  @ size left in r8
    mov r10, #0         @ current offset in r10
    
scfm_migrate_copy_loop:
    @ first: get read size for block
    mov r9, r8  @ current block size in r9
    cmp r9, #SCFM_MIGRATION_BLK_SIZE
    movhi r9, #SCFM_MIGRATION_BLK_SIZE @ use smaller size if over 4mb left
    
    @ read the block
    str r10, [sp]   @ offset
    str r5, [sp, #4] @ file (src)
    mov r0, #0x20 @ nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  @ fsa handle
    mov r1, r7  @ buf
    mov r2, #1  @ size
    mov r3, r9  @ count
    bl REL_FS_FSAREADFILEOFFSET
    cmp r0, r9
    movne r1, #7        @ Stage 7: failed to read source scfm.img
    bne scfm_do_migration_fail_ret
    
    @ write the block
    str r10, [sp]   @ offset
    str r6, [sp, #4] @ file (src)
    mov r0, #0x20 @ nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  @ fsa handle
    mov r1, r7  @ buf
    mov r2, #1  @ size
    mov r3, r9  @ count
    bl REL_FS_FSAWRITEFILEOFFSET
    cmp r0, r9
    movne r1, #8        @ Stage 8: failed to write dst scfm.img
    bne scfm_do_migration_fail_ret
    
    sub r8, r9  @ left -= cur_size
    add r10, r9 @ offset += cur_size
    
    @ print bytes left
    mov r1, r8
    ldr r0, =str_migrate_progress_format
    bl REL_FS_SYSLOG_OUTPUT
    
    cmp r8, #0
    bhi scfm_migrate_copy_loop
    
    mov r0, #0
    b scfm_do_migration_ret

scfm_do_migration_fail_ret:
    bl scfm_migration_throw_error
    mov r0, #-1
    
scfm_do_migration_ret:
    push {r0} @ back up result
    
    @ Clean up all the stuff we (potentially) used
    mov r1, r5
    mov r0, r4
    bl REL_FS_FSACLOSEFILE
    mov r1, r6
    mov r0, r4
    bl REL_FS_FSACLOSEFILE
    
    cmp r7, #0
    beq scfm_do_migration_after_free
    mov r1, r7
    mov r0, #1
    bl REL_FS_IOS_FREE
    
    pop {r0} @ get result
scfm_do_migration_after_free:
    add sp, #0x10
    pop {r4-r9,pc}

@ r0 fsa handle
scfm_mount_slcs:
    push {r4, lr}
    sub sp, #8
    mov r4, r0 @ fsa handle in r4
    
    @ mount /dev/slc at /vol/scfm_mnt
    mov r0, r4
    ldr r1, =str_dev_slc
    ldr r2, =str_vol_scfm_mnt
    mov r3, #0
    str r3, [sp]
    str r3, [sp, #4]
    bl REL_FS_FSAMOUNT
    cmp r0, #0
    movne r0, #-1        @ -1: failed to mount /dev/slc
    bne scfm_slc_mount_ret
    
    @ mount /dev/slccmpt at /vol/scfm_cmpt
    mov r0, r4
    ldr r1, =str_dev_slccmpt
    ldr r2, =str_vol_scfm_cmpt
    mov r3, #0
    str r3, [sp]
    str r3, [sp, #4]
    bl REL_FS_FSAMOUNT
    cmp r0, #0
    movne r0, #-2        @ -2: failed to mount /dev/slccmpt
    
scfm_slc_mount_ret:
    add sp, #8
    pop {r4, pc}

scfm_unmount_slcs:
    push {r4, lr}
    mov r4, r0
    
    mov r2, #0
    ldr r1, =str_vol_scfm_cmpt
    mov r0, r4
    bl REL_FS_FSAUNMOUNT
    
    mov r2, #0
    ldr r1, =str_vol_scfm_mnt
    mov r0, r4
    bl REL_FS_FSAUNMOUNT
    pop {r4, pc}
    
@ r0 error, r1 stage (where we errored), code is stage-error
scfm_migration_throw_error:
    mov r2, r0
    ldr r0, =str_migration_fail_format
    b REL_FS_SYSLOG_OUTPUT
    
scfm_migration_strcpy:
    mov r2, r0
scfm_migration_strcpy_loop:
    ldrb r3, [r1],#1
    strb r3, [r2],#1
    cmp r3, #0
    bne scfm_migration_strcpy_loop
    bx lr

str_migration_fail_format: 
.byte 0xA
.ascii "SCFM-SALT: migration failed, %02d-%08X"
.byte 0xA,0
str_migrate_progress_format: 
    .ascii "SCFM-SALT: Migrating scfm.img, %d bytes remaining..."
.byte 0xA, 0
str_migrate_success: .ascii "SCFM-SALT: Using scfm.img on /dev/slccmpt."
.byte 0xA,0
str_dev_slccmpt: .ascii "/dev/slccmpt01"
.byte 0
str_dev_slc: .ascii "/dev/slc01"
.byte 0
str_vol_scfm_cmpt: .ascii "/vol/scfm_cmpt"
.byte 0
str_cmpt_scfm_img: .ascii "/vol/scfm_cmpt/scfm.img"
.byte 0
str_vol_scfm_mnt: .ascii "/vol/scfm_mnt"
.byte 0
str_mnt_scfm_img: .ascii "/vol/scfm_mnt/scfm.img"
.byte 0
str_r: .ascii "r"
.byte 0
str_wplus: .ascii "w+"
.byte 0
.align 4
#endif

.align 4
seekfile_hook:
    push {r0-r3, lr}
    ldr r0, [r0,#4]
    ldr r0, [r0, #0xC] @ get pid
    cmp r0, #FSAPRINT_MAX_PID
    bhi seekfile_return
    
    cmp r0, #FSAPRINT_MIN_PID
    bls seekfile_return
    
    mov r1, r3  @ seek offset
    mov r3, r2  @ handle
    mov r2, r0  @ pid
    adr r0, str_seekfile_format
    bl REL_FS_SYSLOG_OUTPUT
    
seekfile_return:
    pop {r0-r3, lr}
    subs r6, r0, #0
    bx lr
    
str_seekfile_format: 
.ascii "SALT: fileseek to 0x%X pid %d handle 0x%X"
.byte 0xA
.byte 0
.align 4

fread_hook:
    push {r0-r3, lr}
    adr r1, str_read
    b fread_write_print
    
fwrite_hook:
    push {r0-r3, lr}
    adr r1, str_write
    b fread_write_print
    
fread_write_print:
    ldr r0, [r0,#4]
    ldr r0, [r0, #0xC] @ get pid
    cmp r0, #FSAPRINT_MAX_PID
    bhi fread_write_return
    
    cmp r0, #FSAPRINT_MIN_PID
    bls fread_write_return
    
    sub sp, #4
    str r0, [sp]
    
    adr r0, str_read_write_print_format
    ldr r2, [sp, #(4*(9+5+1))]    @ base's sp+0, cnt
    mul r2, r3                  @ cnt * size = r/w size
    ldr r3, [sp, #(4*(9+5+1+2))]  @ base's sp+0x8, handle
    bl REL_FS_SYSLOG_OUTPUT
    add sp, #4
fread_write_return:
    pop {r0-r3, lr}
    subs r10, r0, #0    @ replaced
    bx lr
    
str_read: .ascii "read"
.byte 0
str_write: .ascii "write"
.byte 0
str_read_write_print_format: 
.ascii "SALT: file %s size 0x%X handle 0x%X pid %d"
.byte 0xA,0
.align 4
    



wfs_crypto_hook:
    ldrb r3, [lr,#0x19] @ replaced
#if USB_SHRINKSHIFT | NO_CRYPTO
    push {r4-r7}
#if !NO_CRYPTO
    // Disable crypto only for selected USB
    ldr r4, =NOCRYPTO_WFS_SECTOR_COUNT1
    ldr r6, =NOCRYPTO_WFS_SECTOR_COUNT2
    ldr r7, =NOCRYPTO_WFS_SECTOR_COUNT3
    cmp r5, r4  @ sector count in r5
    cmpne r5, r6
    cmpne r5, r7
    bne wfs_crypto_hook_ret
#endif // !NO_CRYPTO
    
    @ set the key to 0xDEADBEEF to skip crypto
    ldr r5, =0xDEADBEEF
    ldr r4, [lr, #0x24]
    str r5, [r4]
    
wfs_crypto_hook_ret:
    pop {r4-r7}
#endif // USB_SHRINKSHIFT | NOCRYPTO
    b REL_WFS_CRYPTO_HOOK_RET_ADDR
.align 4
    
fsaopen_fullstr_dump_hook:
    push {r4, lr}
    bl REL_FS_GET_FULL_DIR_STR   @ fsa_get_full_dir_str
    mov r4, r0
    mov r1, r5
    mov r2, r6
    adr r0, fullstr_str
    bl REL_FS_SYSLOG_OUTPUT
    mov r0, r4
    pop {r4, pc}

fullstr_str: .ascii "SALT: FSAOpen %s mode:'%s'"
.byte 0xA,0
.align 4

opendir_hook:
    push {r0-r3, lr}
    mov r1, r2
    adr r0, fsaopendir_str
    bl REL_FS_SYSLOG_OUTPUT
    pop {r0-r3, lr}
    
    subs r6, r0, #0
    b (REL_FS_OPENDIR_BASE+4)

fsaopendir_str: 
.ascii "SALT: FSAOpenDir %s"
.byte 0xA,0
.align 4
    
sdcard_init:
    @ this should run *after* /dev/mmc thread is created
    push {lr}

    @ first we create our synchronization stuff
    mov r0, #1
    mov r1, #1
    bl iosCreateSemaphore
    ldr r1, =sdcard_access_mutex
    str r0, [r1]

    @ then we sleep until /dev/mmc is done initializing sdcard (TODO : better synchronization here)
    mov r0, #1000
    bl msleep

    @ finally we set some flags to indicate sdcard is ready for use
    ldr r0, =FS_MMC_SDCARD_STRUCT
    ldr r1, [r0, #0x24]
    orr r1, #0x20
    str r1, [r0, #0x24]
    ldr r1, [r0, #0x28]
    bic r1, #0x4
    str r1, [r0, #0x28]

    pop {pc}

mlc_init:
    @ this should run *after* /dev/mmc thread is created (and after sdcard_init)
    @ this should also only be run when you want to dump mlc; this will cause the physical mlc device to be inaccessible to regular FS code
    push {lr}

    @ finally we set some flags to indicate sdcard is ready for use
    ldr r0, =FS_MMC_MLC_STRUCT
    ldr r1, [r0, #0x24]
    orr r1, #0x20
    str r1, [r0, #0x24]
    ldr r1, [r0, #0x28]
    bic r1, #0x4
    str r1, [r0, #0x28]

    pop {pc}

.equ sdcard_readwrite_stackframe_size, (4*4+12*4)

@ r0 : bool read (0 = read, not 0 = write), r1 : data_ptr, r2 : cnt, r3 : block_size, sparg0 : offset_blocks, sparg1 : out_callback_arg2, sparg2 : device_id
sdcard_readwrite:
    
    push {r4,r5,r6,lr}
    sub sp, #12*4

    @ pointer for command paramstruct
    add r4, sp, #0xC
    @ pointer for mutex (sp + 0x8 will be callback's arg2)
    add r5, sp, #0x4
    @ offset_blocks
    ldr r6, [sp, #sdcard_readwrite_stackframe_size]

    @ first of all, grab sdcard mutex
    push {r0,r1,r2,r3}
    ldr r0, =sdcard_access_mutex
    ldr r0, [r0]
    mov r1, #0
    bl iosWaitSemaphore
    @ also create a mutex for synchronization with end of operation...
    mov r0, #1
    mov r1, #1
    bl iosCreateSemaphore
    str r0, [r5]
    @ ...and acquire it
    mov r1, #0
    bl iosWaitSemaphore
    pop {r0,r1,r2,r3}

    @ block_size needs to be equal to sector_size (0x200)
    sdcard_readwrite_block_size_adjust:
        cmp r3, #0x200
        movgt r3, r3, lsr #1 @ block_size >>= 1;
        movgt r2, r2, lsl #1 @ cnt <<= 1;
        movgt r6, r6, lsl #1 @ offset_blocks <<= 1;
        bgt sdcard_readwrite_block_size_adjust

    @ build rw command paramstruct 
    str r2, [r4, #0x00] @ cnt
    str r3, [r4, #0x04] @ block_size
    cmp r0, #0
    movne r0, #0x3 @ read operation
    str r0, [r4, #0x08] @ command type
    str r1, [r4, #0x0C] @ data_ptr
    mov r0, #0
    str r0, [r4, #0x10] @ offset_high
    str r6, [r4, #0x14] @ offset_low
    str r0, [r4, #0x18] @ callback
    str r0, [r4, #0x1C] @ callback_arg
    mvn r0, #0
    str r0, [r4, #0x20] @ -1

    @ setup parameters
    ldr r0, [sp, #sdcard_readwrite_stackframe_size+0x8] @ device_identifier : sdcard (real one is 0x43, but patch makes 0xDA valid)
    mov r1, r4 @ paramstruct
    mov r2, r6 @ offset_blocks
    adr r3, sdcard_readwrite_callback @ callback
    str r5, [sp] @ callback_arg (mutex ptr)

    @ call readwrite function
    bl REL_FS_SDIO_DOREADWRITECOMMAND
    mov r4, r0
    cmp r0, #0
    bne sdcard_readwrite_skip_wait

    @ wait for callback to give the go-ahead
    ldr r0, [r5]
    mov r1, #0
    bl iosWaitSemaphore
    ldr r0, [r5, #0x4]
    ldr r1, [sp, #sdcard_readwrite_stackframe_size+0x4]
    cmp r1, #0
    strne r0, [r1]
    sdcard_readwrite_skip_wait:

    @ finally, release sdcard mutexes
    ldr r0, [r5]
    bl iosDestroySemaphore
    ldr r0, =sdcard_access_mutex
    ldr r0, [r0]
    bl iosSignalSemaphore

    @ return
    mov r0, r4
    add sp, #12*4
    pop {r4,r5,r6,pc}

    @ release mutex to let everyone know we're done
sdcard_readwrite_callback:
    str r1, [r0, #4]
    ldr r0, [r0]
    b iosSignalSemaphore

createDevThread_hook:
    push {r0}
    @ check if we were initializing /dev/mmc
    ldr r0, [r4, #0x8]
    cmp r0, #0xD
    bne createDevThread_hook_skip1

    bl sdcard_init

    bl clear_screen

    mov r0, #20
    mov r1, #20
    adr r2, createDevThread_hook_welcome
    bl _printf

    createDevThread_hook_skip1:

    @ check if we were initializing /dev/fla
    @ldr r0, [r4, #0x8]
    @cmp r0, #0x7
    @bne createDevThread_hook_skip2

    @createDevThread_hook_skip2:
    pop {r0,r4-r8,pc}
    createDevThread_hook_welcome:
    .ascii "welcome to redNAND: SALT edition"
.byte 0
    .align 0x4
    

getMdDeviceById_hook:
    mov r4, r0
    cmp r0, #0xDA @ magic id (sdcard)
    ldreq r0, =FS_MMC_SDCARD_STRUCT
    popeq {r4,r5,pc}
    @cmp r0, #0xAB @ magic id (mlc)
    @ldreq r0, =FS_MMC_MLC_STRUCT
    @popeq {r4,r5,pc}
    bx lr @ return if different

registerMdDevice_hook:
    push {r4,lr}

    cmp r0, #0
    beq registerMdDevice_hook_skip

    ldr r3, [r0, #0x8] @ mmc device struct ptr
    ldr r4, =FS_MMC_SDCARD_STRUCT
    cmp r3, r4
    bne registerMdDevice_hook_skip

    @ sdcard ! fix stuff up so that registration can happen ok

    push {r0-r3}
    
    @ first lock that mutex
    ldr r0, =sdcard_access_mutex
    ldr r0, [r0]
    mov r1, #0
    bl iosWaitSemaphore
    
    @ then, clear the flag we set earlier (that FS_REGISTERMDPHYSICALDEVICE will set back anyway)
    ldr r0, =FS_MMC_SDCARD_STRUCT
    ldr r1, [r0, #0x24]
    bic r1, #0x20
    str r1, [r0, #0x24]

    pop {r0-r3}

    @ register it !
    bl REL_FS_REGISTERMDPHYSICALDEVICE
    mov r4, r0

    @ finally, release the mutex
    ldr r0, =sdcard_access_mutex
    ldr r0, [r0]
    bl iosSignalSemaphore

    mov r0, r4
    pop {r4,pc}

    registerMdDevice_hook_skip:
    @ not sdcard
    bl REL_FS_REGISTERMDPHYSICALDEVICE
    pop {r4,pc}

@ read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
@ readWriteCallback_patch(bool read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
readWriteCallback_patch:
.equ readWriteCallback_patch_stackframe_size, (7*4)
    push {r0,r1,r2,r3,r4,r5,lr}
    mov r5, #0xDA
    str r5, [sp, #0x8] @ device id (sdcard)
    add r5, sp, #0xC @ out_callback_arg2 dst
    add r2, r1
    str r2, [sp] @ offset
    str r5, [sp, #4] @ out_callback_arg2
    ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0x4] @ data_ptr
    mov r2, r3 @ cnt
    ldr r3, [sp, #readWriteCallback_patch_stackframe_size] @ block_size
    bl sdcard_readwrite
    mov r4, r0
    cmp r0, #0
    bne readWriteCallback_patch_skip_callback

    ldr r12, [sp, #readWriteCallback_patch_stackframe_size+0x8] @ callback
    ldr r0, [r5]
    ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0xC] @ callback_parameter
    cmp r12, #0
    blxne r12

    readWriteCallback_patch_skip_callback:
    mov r0, r4
    add sp, #4
    pop {r1,r2,r3,r4,r5,pc}

@ @ @ @ @ @ @ @ @ @
@    USB TWEAKS   @
@ @ @ @ @ @ @ @ @ @

@ spoof mass storage sector count, if it matches
usb_sector_spoof:
    ldr r11, [r7, #0x20]
#if USB_SHRINKSHIFT
    add r9, r11, 0x20
    ldr r2, [r9]
    ldr r1, =(USB_SHRINK_ORIG_SECTOR_COUNT1-1)
    ldr r3, =(USB_SHRINK_SECTOR_COUNT1-1) @ these numbers are technically 'last sector'/'highest lba'
    cmp r2, r1
    ldrne r1, =(USB_SHRINK_ORIG_SECTOR_COUNT2-1)
    ldrne r3, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r2, r1
    ldrne r1, =(USB_SHRINK_ORIG_SECTOR_COUNT3-1)
    ldrne r3, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r2, r1
    streq r3, [r9]
#endif
    b (REL_FS_USB_SECTOR_SPOOF+4)

@ add an offset to mass storage sector reads/writes
usbRead_patch:
    push {r4-r11, lr}
    
#if PRINT_USB_RW
    push {r0-r3}
    ldr r0, [sp,#((9+4)*4)]
    mul r3, r0
    adr r0, usb_str
    adr r1, read_str
    bl REL_FS_SYSLOG_OUTPUT
    pop {r0-r3}
#endif
    
#if USB_SHRINKSHIFT
    @ get max lba from *(*(ctx+0x20)+0x20)
    ldr r5, [r0, #0x20]
    ldr r6, [r5, #0x20]
    ldr r4, =(USB_SHRINK_SECTOR_COUNT1-1)   @ see if it matches any of the sector counts
    cmp r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r4, r6
    moveq r7, #0x800     @ if so, add our offset of 0x800 sectors (1MB)
    addeq  r2, r7
#endif
    b (REL_FS_USB_READ+4)

usbWrite_patch:
    push {r4-r11, lr}
    
#if PRINT_USB_RW
    push {r0-r3}
    ldr r0, [sp,#((9+4)*4)]
    mul r3, r0
    adr r0, usb_str
    adr r1, write_str
    bl REL_FS_SYSLOG_OUTPUT
    pop {r0-r3}
#endif
    
#if USB_SHRINKSHIFT
    @ get max lba from *(*(ctx+0x20)+0x20)
    ldr r5, [r0, #0x20]
    ldr r6, [r5, #0x20]
    ldr r4, =(USB_SHRINK_SECTOR_COUNT1-1)   @ see if it matches any of the sector counts
    cmp r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r4, r6
    moveq r7, #0x800     @ if so, add our offset of 0x800 sectors (1MB)
    addeq  r2, r7
#endif
    b (REL_FS_USB_WRITE+4)
usb_str:.ascii "SALT: USB %s offset 0x%X size 0x%X"
.byte 0xA, 0
read_str:.ascii "read"
.byte 0
write_str:.ascii "write"
.byte 0
.align 4


@ @ @ @ @ @ @ @ @ @ @
@ SDIO REDIRECTION  @
@ @ @ @ @ @ @ @ @ @ @

sdcardWrite_patch:
    mov r12, #0 @ write
    b sdcardReadWrite_patch

sdcardRead_patch:
    mov r12, #1 @ read
sdcardReadWrite_patch:
    ldr r4, [r0, #0x14]
    cmp r4, #5 @ DEVICETYPE_SDCARD
    ldreq r4, redmlc_size_sectors
    cmpeq r4, #0
    subeq sp,sp,#0x14 @overwritten instruction
    bxeq lr
    pop {r4,lr}
    mov r0, r12
    mov r1, #0
    b readWriteCallback_patch

.global redmlc_size_sectors
redmlc_size_sectors:
    .word 0

@ @ @ @ @ @ @ @ @ @
@ SLC REDIRECTION @
@ @ @ @ @ @ @ @ @ @

.global redslc_off_sectors
.global redslc_size_sectors
.global redslccmpt_off_sectors
.global redslccmpt_size_sectors

slcReadWrite_patch:
    push {r2}
    ldr r2, [r0, #4]
    mov r0, r1
    cmp r2, #0
    ldrne r1, redslc_off_sectors
    ldreq r1, redslccmpt_off_sectors
    
    lsr r1, #2   @ 4 sd sectors -> 1 slc sector
    pop {r2}
    b readWriteCallback_patch

slcRead1_patch:
    push {r0, r1}
    ldr r1, [r0, #4]
    cmp r1, #0
    ldrne r1, redslc_size_sectors
    ldreq r1, redslccmpt_size_sectors
    cmp r1, #0
    pop {r0, r1}
    bne slcRead1_patch_cont
    
    @ slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b REL_FS_SLC_READ1+4
    
slcRead1_patch_cont:
    mov r1, #1 @ read
    b slcReadWrite_patch

slcWrite1_patch:
    push {r0, r1}
    ldr r1, [r0, #4]
    cmp r1, #0
    ldrne r1, redslc_size_sectors
    ldreq r1, redslccmpt_size_sectors
    cmp r1, #0
    pop {r0, r1}
    bne slcWrite1_patch_cont
    
    @ slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b REL_FS_SLC_WRITE1+4

slcWrite1_patch_cont:
    mov r1, #0 @ write
    b slcReadWrite_patch

slcRead2_patch:
    push {r0, r1}
    ldr r1, [r0, #4]
    cmp r1, #0
    ldrne r1, redslc_size_sectors
    ldreq r1, redslccmpt_size_sectors
    cmp r1, #0
    pop {r0, r1}
    bne slcRead2_patch_cont
    
    @ slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b REL_FS_SLC_READ2+4

slcRead2_patch_cont:
.equ slcRead2_patch_stackframe_size, (0x10+7*4)
    push {r0-r5,lr}
    sub sp, #0x10
    ldr r4, [sp, #slcRead2_patch_stackframe_size+0x00]
    str r4, [sp, #0x0] @ block_size
    ldr r4, [sp, #slcRead2_patch_stackframe_size+0x08]
    str r4, [sp, #0x4] @ data_outptr
    ldr r4, [sp, #slcRead2_patch_stackframe_size+0x10]
    str r4, [sp, #0x8] @ callback
    ldr r4, [sp, #slcRead2_patch_stackframe_size+0x14]
    str r4, [sp, #0xC] @ callback_parameter
    bl slcRead1_patch_cont
    add sp, #0x14
    pop {r1-r5,pc}

    
slcWrite2_patch:
    push {r0, r1}
    ldr r1, [r0, #4]
    cmp r1, #0
    ldrne r1, redslc_size_sectors
    ldreq r1, redslccmpt_size_sectors
    cmp r1, #0
    pop {r0, r1}
    bne slcWrite2_patch_cont
    
    @ slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b REL_FS_SLC_WRITE2+4

slcWrite2_patch_cont:
.equ slcWrite2_patch_stackframe_size, (0x10+6*4)
    push {r0-r4,lr}
    sub sp, #0x10
    ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x00]
    str r4, [sp, #0x0] @ block_size
    ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x08]
    str r4, [sp, #0x4] @ data_outptr
    ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x10]
    str r4, [sp, #0x8] @ callback
    ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x14]
    str r4, [sp, #0xC] @ callback_parameter
    bl slcWrite1_patch_cont
    add sp, #0x14
    pop {r1-r4,pc}
 
redslc_off_sectors:
    .word 0
redslc_size_sectors:
    .word 0
redslccmpt_off_sectors:
    .word 0
redslccmpt_size_sectors:
    .word 0

@ @ @ @ @ @ @ @ @ @
@   DEBUG STUFF   @
@ @ @ @ @ @ @ @ @ @

mlcRead1_dbg:
.equ mlcRead1_dbg_stackframe, (4*6)
    mov r12, r0
    push {r0-r3,r12,lr}
    adr r0, mlcRead1_dbg_format
    ldr r1, [sp, #mlcRead1_dbg_stackframe+9*4]
    bl REL_FS_SYSLOG_OUTPUT
    pop {r0-r3,lr,pc} @ replaces mov lr, r0
    mlcRead1_dbg_format:
        .ascii "mlcRead1 : %08X %08X %08X"
.byte 0xA,0
        .align 0x4
    
mlcWrite1_dbg:
.equ mlcWrite1_dbg_stackframe, (4*6)
    mov r12, r0
    push {r0-r3,r12,lr}
    adr r0, mlcWrite1_dbg_format
    ldr r1, [sp, #mlcWrite1_dbg_stackframe+9*4]
    bl REL_FS_SYSLOG_OUTPUT
    pop {r0-r3,lr,pc} @ replaces mov lr, r0
    mlcWrite1_dbg_format:
        .ascii "mlcWrite1 : %08X %08X %08X"
.byte 0xA,0
        .align 0x4

@ r0 : data ptr
@ r1 : size
@ r2 : offset_blocks
@ r3 : read
rw_data_offset:
    push {r1,r2,r3,r4,lr}
    mov r4, r3
    mov r3, r1, lsr #9 @ size /= 0x200
    cmp r3, #0
    moveq r3, #1
    mov r1, r0 @ data_ptr
    str r2, [sp] @ offset
    mov r0, #0
    str r0, [sp, #0x4] @ out_callback_arg2
    mov r0, #0xDA
    str r0, [sp, #0x8] @ device id
    cmp r3, #1
    moveq r0, #1 @ read
    movne r0, #0 @ write
    mov r2, r3 @ num_sectors
    mov r3, #0x200 @ block_size
    bl sdcard_readwrite
    add sp, #0xC
    pop {r4,pc}
.pool

.equ FRAMEBUFFER_ADDRESS, (0x14000000+0x38C0000)
.equ FRAMEBUFFER_STRIDE,  (0xE00)
.equ FRAMEBUFFER_SIZE, (FRAMEBUFFER_STRIDE*504)
.equ CHARACTER_MULT, (2)
.equ CHARACTER_SIZE, (8*CHARACTER_MULT)

clear_screen:
    push {lr}
    ldr r0, =FRAMEBUFFER_ADDRESS @ data_ptr
    ldr r1, =0x00
    ldr r2, =FRAMEBUFFER_SIZE
    bl memset
    pop {pc}

@ r0 : x, r1 : y, r2 : format, ...
@ NOT threadsafe so dont even try you idiot
_printf:
    ldr r12, =_printf_xylr
    str r0, [r12]
    str r1, [r12, #4]
    str lr, [r12, #8]
    ldr r0, =_printf_string
    mov r1, #_printf_string_end-_printf_string
    bl snprintf
    ldr r12, =_printf_xylr
    ldr r1, [r12]
    ldr r2, [r12, #4]
    ldr lr, [r12, #8]
    push {lr}
    ldr r0, =_printf_string
    bl drawString
    pop {pc}


@ r0 : str, r1 : x, r2 : y
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
        add r5, #(CHARACTER_SIZE)
        b drawString_loop
    drawString_end:
    pop {r4-r6,pc}

@ r0 : char, r1 : x, r2 : y
drawCharacter:
    subs r0, #32
    @ bxlt lr
    push {r4-r7,lr}
    ldr r4, =FRAMEBUFFER_ADDRESS @ r4 : framebuffer address
    add r4, r1, lsl #2 @ add x * 4
    mov r3, #FRAMEBUFFER_STRIDE
    mla r4, r2, r3, r4
    ldr r5, =font_bin @ r5 : character data
    add r5, r0, lsl #3 @ font is 1bpp, 8x8 => 8 bytes represents one character
    mov r1, #0xFFFFFFFF @ color
    mov r2, #0x0 @ empty color
    mov r6, #8 @ i
    drawCharacter_loop1:
        mov r3, #CHARACTER_MULT
        drawCharacter_loop3:
            mov r7, #8 @ j
            ldrb r0, [r5]
            drawCharacter_loop2:
                tst r0, #1
                @ as many STRs as CHARACTER_MULT (would be nice to do this in preproc...)
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

.section ".bss"

sdcard_access_mutex:
    .word 0x00000000
mlc_out_callback_arg2:
    .word 0x00000000
_printf_xylr:
    .word 0x00000000
    .word 0x00000000
    .word 0x00000000
_printf_string:
    .skip 0x100
_printf_string_end:

.global trampoline_buffer
.global trampoline_buffer_end
.align 0x4
trampoline_buffer:
.skip 2048
trampoline_buffer_end:


.skip 32
.align 0x4
syslog_buffer:
    .skip 0x40000

.skip 32
.align 0x4
sdcard_read_buffer:
    .skip 0x100000