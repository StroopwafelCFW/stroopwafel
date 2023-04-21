
.align 4
.arm
_vwii_pocket_patches_jumps:
    b otp_hook
otp_hook:
    .arm

    ; orig instructs (this should hopefully cover everything?)
    MOVS            R4, #0xC4
    MOVS            R3, #0x80
    LSLS            R4, R4, #6
    LSLS            R3, R3, #0x13
    LDR             R2, =0x93400000
    STR             R3, [R4,#0x18]
    STR             R3, [R4,#0x1C]

    push {r0-r5,lr}

    ; r6 = ptr

    ; Don't patch Hai IOSs, they don't actually reload.
    ldr r4, =0xFFFF0000
    cmp r6, r4
    bxeq lr

    mov r4, r6
    mov r0, r6
    
    ;bl _pocket_patches_replace_2
    ;mov r0, r4
    ;bl _pocket_patches_replace_4

    pop {r0-r5, pc}

.align 4
compact_memcpy_t:
    .thumb
    bx pc

.align 4
compact_memcpy:
    .arm
    ldrb            r3, [r0],#1
    strb            r3, [r1],#1
    subs            r2, r2, #1
    bpl             compact_memcpy
    bx              lr

.pool

; Copied-in OTP read replacement function
.align 4
c2w_otp_replacement_t:
    .thumb
    push {r4-r5,lr}
    ; r0 = word idx
    ; r1 = dst
    ; r2 = len
    add r4, r1, r2
    ldr r5, =0xFFF07C80
    lsl r0, r0, #0x2
    add r0, r5, r0

c2w_otp_replacement_loop:
    cmp r1, r4
    beq c2w_otp_replacement_loop_done
    ldrb r2, [r0]
    strb r2, [r1]
    add r0, r0, #0x1
    add r1, r1, #0x1
    b c2w_otp_replacement_loop

c2w_otp_replacement_loop_done:
    pop {r4-r5, pc}

.pool
c2w_otp_replacement_t_end:


.pool
