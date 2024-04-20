.global trampoline_proto
.global trampoline_proto_chain
.global trampoline_proto_target
.global trampoline_proto_end

.align 4
.arm
trampoline_proto:
    push {lr}
@trampoline_proto_chain:
@    adr lr, trampoline_proto_chain @old target
    ldr lr, trampoline_proto_chain
    push {lr}
    ldr lr, trampoline_proto_target
    blx lr
    pop {lr, pc}
trampoline_proto_target:
    .word 0
trampoline_proto_chain:
    .word 0
trampoline_proto_end:


.global trampoline_t_proto
.global trampoline_t_proto_chain
.global trampoline_t_proto_target
.global trampoline_t_proto_end

.align 4
.thumb
trampoline_t_proto:
    push {r0,r1,r2, lr}
    adr r1,  trampoline_t_proto_target
    ldm r1, {r0,r1}
    str r1, [sp, #8]  @where we pushed r2
    mov lr, r0
    pop {r0, r1}
    blx lr
    add sp, #4
    pop {pc} @ remove chain pointer, jump to old lr
.align 4
trampoline_t_proto_target:
    .word 0
trampoline_t_proto_chain:
    .word 0
trampoline_t_proto_end:
