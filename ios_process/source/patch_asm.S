
.extern fw_img_path

@ jump here on add r7, sp, #0x24
.global ancast_crypt_check
ancast_crypt_check:
    .thumb
    bx pc   @ change to arm
    nop
    .arm
    ldr r7, =0x10001A0  @ device type offset
    ldrh r7, [r7]       @ get device type
    tst r7, #1          @ set bit 0 at the u16 at 0x1A0 for no-crypt mode
    bne ancast_no_crypt

    add r7, sp, #0x24   @ replaced instructions
    str r7, [sp, #0x18]
    bx lr

ancast_no_crypt:
    pop {r4-r7, lr}
    add sp, #0x10
    mov r0, #0
    bx lr

.pool



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

    .thumb
    .global launch_os_hook
    launch_os_hook:
        bx pc
        .align 0x4
        .arm
        push {r0-r3,lr}
        sub sp, #8

        bl MCP_SYSLOG_OUTPUT

        mov r0, #0
        bl MCP_FSA_OPEN

        ldr r1, =launch_os_hook_devicepath
        ldr r2, =launch_os_hook_mountpath
        mov r3, #0
        str r3, [sp]
        str r3, [sp, #4]
        bl MCP_FSA_MOUNT

        add sp, #8
        pop {r0-r3,pc}

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