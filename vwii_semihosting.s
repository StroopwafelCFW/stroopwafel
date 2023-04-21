
.align 4
.thumb
_vwii_pocket_gpio_debug_send:
    push {lr}
    ldr r1, =0xDEADCAFE
    ;bl bx_r1
    bl _vwii_pocket_delay_ticks
    pop {pc}
.align 4
_vwii_semihosting_ledoutaddr:
.pool
bx_r1:
    bx r1


.align 4
_vwii_pocket_serial_send_str_t:
    .thumb

    push {r4,lr}
    mov r4, r1
_vwii_pocket_serial_send_str_loop1:
    ldrb r0, [r4]
    cmp r0, #0x0
    beq _vwii_pocket_serial_send_str_loop1_end
    ;bl _vwii_pocket_serial_send
    bl _vwii_pocket_gpio_debug_send
    add r4, r4, #0x1
    b _vwii_pocket_serial_send_str_loop1

_vwii_pocket_serial_send_str_loop1_end:
    pop {r4}
    pop {r0}
    bx pc

.arm
    movs pc, r0

;.thumb

;    LDR     R2, =0xD8000FC
;    ldr     R3, =0xFF0000
;    LDR     R1, [R2]
;    BIC     R1, R3
;    STR     R1, [R2]
;    mov r1, #0x20
;    SUB     R2, R2, r1
;    LDR     R1, [R2]
;    ORR     R1, R3
;    STR     R1, [R2]
;    mov r1, #8
;    ADD     R2, R1
;    LDR     R1, [R2]
;    ORR     R1, R3
;    STR     R1, [R2]
;    lsl     R0, R0, #16
;    SUB     R2, R2, #4
;    LDR     R1, [R2]
;    BIC     R1, R3
;    ORR     R1, R0
;    STR     R1, [R2]
    

.thumb
_vwii_pocket_delay_ticks:
    bx lr
    ldr r2, =0x0d800010
    ldr r3, [r2] ; now
    add r0, r3, #4 ; then

    cmp r0, r3 ; then < now?
    bge _vwii_pocket_delay_ticks_loop2

_vwii_pocket_delay_ticks_loop1:
    ldr r1, [r2] ; cur_read
    cmp r1, r3 ; cur, now
    bge _vwii_pocket_delay_ticks_loop1

_vwii_pocket_delay_ticks_loop2:
    ldr r3, [r2] ; now
    cmp r3, r0 ; now < then
    blt _vwii_pocket_delay_ticks_loop2
    
    ;bx lr

.thumb
_vwii_pocket_serial_send:
    PUSH    {R4-R7,LR}
    MOV     R4, R0
    MOV     R6, #7
    mov r7, #0x80
_vwii_pocket_serial_send_loop1:
    MOV     R5, R4
    ASR r5, R6
    ;MOV     R0, #0
    ;BL      _vwii_pocket_gpio_debug_send
    
    mov r1, #0x1
    AND     R5, r1
    MOV     R0, R5
    BL      _vwii_pocket_gpio_debug_send
    mov r0, r5
    ORR     R0, r7
    BL      _vwii_pocket_gpio_debug_send
    MOV     R0, R5
    BL      _vwii_pocket_gpio_debug_send
    SUB    R6, R6, #1
    BCS     _vwii_pocket_serial_send_loop1

    MOV     R0, R5
    ORR     R0, r7
    BL      _vwii_pocket_gpio_debug_send

    MOV     R0, R5
    BL      _vwii_pocket_gpio_debug_send

    BL       _vwii_pocket_serial_force_terminate
    POP     {R4-R7,PC}

.thumb
_vwii_pocket_serial_force_terminate:
    push {r4,lr}
    mov r0, #0xF
    bl _vwii_pocket_gpio_debug_send

    mov r0, #0x8F
    bl _vwii_pocket_gpio_debug_send

    mov r0, #0xF
    bl _vwii_pocket_gpio_debug_send

    mov r0, #0x0
    bl _vwii_pocket_gpio_debug_send
    bl _vwii_pocket_delay_ticks

    pop {r4,pc}


.pool