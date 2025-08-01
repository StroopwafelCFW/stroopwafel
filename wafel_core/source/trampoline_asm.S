.global trampoline_blre_proto
.global trampoline_blre_proto_chain
.global trampoline_blre_proto_target
.global trampoline_blre_proto_end

.align 2
.arm
trampoline_blre_proto:
    push {lr}
    ldr lr, trampoline_blre_proto_chain
    push {lr}
    ldr lr, trampoline_blre_proto_target
    blx lr
    pop {lr, pc}
trampoline_blre_proto_target:
    .word 0
trampoline_blre_proto_chain:
    .word 0
trampoline_blre_proto_end:


.global trampoline_blre_regs_proto
.global trampoline_blre_regs_proto_chain
.global trampoline_blre_regs_proto_target
.global trampoline_blre_regs_proto_end

.align 2
.arm
trampoline_blre_regs_proto:
    push {lr}
    ldr lr, trampoline_blre_regs_proto_chain
    push {r4-r12, lr}
    ldr lr, trampoline_blre_regs_proto_target
    blx lr
    add sp, #36 @9 regs x 4bytes
    pop {lr, pc}
trampoline_blre_regs_proto_target:
    .word 0
trampoline_blre_regs_proto_chain:
    .word 0
trampoline_blre_regs_proto_end:



trampoline_hookafter_proto:
    push {lr}
trampoline_hookafter_proto_orgins:
    nop @org instrction 
    push {r0-r11}
    ldr lr, trampoline_hookafter_proto_target
    blx lr
    pop {r0-r11,pc}
trampoline_hookafter_proto_target:
    .word 0
trampoline_hookafter_proto_end:


.global trampoline_hookbefore_proto
.global trampoline_hookbefore_proto_orgins
.global trampoline_hookbefore_proto_target
.global trampoline_hookbefore_proto_chain
.global trampoline_hookbefore_proto_end

@ if original instruction wasn't a branch, the old instruction gets placed at
@ trampoline_hookbefore_proto_orgins and the ldr pc will be overwritten
@ if original instruction was a branch, its address will be placed at 
@ trampoline_hookbefore_proto_chain, overwriting the bx lr, which won't be needed 
@ anymore since the ldr pc will branch
.align 2
.arm
trampoline_hookbefore_proto:
    push {r0-r12, lr}
    mov r0, sp
    ldr lr, trampoline_hookbefore_proto_target
    blx lr
    pop {r0-r12,lr}
trampoline_hookbefore_proto_orgins:
    ldr pc, trampoline_hookbefore_proto_chain @org instrction
trampoline_hookbefore_proto_chain:
    bx lr
trampoline_hookbefore_proto_target:
    .word 0
trampoline_hookbefore_proto_end:



.global trampoline_hookbefore_lr_proto
.global trampoline_hookbefore_lr_proto_orgins
.global trampoline_hookbefore_lr_proto_target
.global trampoline_hookbefore_lr_proto_ret
.global trampoline_hookbefore_lr_proto_end

.align 2
.arm
trampoline_hookbefore_lr_proto:
    push {r0-r12, lr}
    mov r0, sp
    ldr  lr, trampoline_hookbefore_lr_proto_target
    blx lr
    cmp r0, #0
    pop {r0-r12, lr}
    ldrne pc, trampoline_hookbefore_lr_proto_ret
trampoline_hookbefore_lr_proto_orgins:
    nop
    ldr pc, trampoline_hookbefore_lr_proto_ret
trampoline_hookbefore_lr_proto_target:
    .word 0
trampoline_hookbefore_lr_proto_ret:
    .word 0
trampoline_hookbefore_lr_proto_end:  

.global trampoline_t_blre_proto
.global trampoline_t_blre_proto_chain
.global trampoline_t_blre_proto_target
.global trampoline_t_blre_proto_end

.align 2
.thumb
trampoline_t_blre_proto:
    push {r0, r1, lr} @ r1 just to reserve space
    ldr r0, trampoline_t_blre_proto_target
    mov lr, r0
    ldr r0, trampoline_t_blre_proto_chain
    str r0, [sp, #4]  @where we pushed r1
    pop {r0}
    blx lr
    add sp, #4
    pop {pc}
.align 2
trampoline_t_blre_proto_target:
    .word 0
trampoline_t_blre_proto_chain:
    .word 0
trampoline_t_blre_proto_end:


.global trampoline_t_blre_regs_proto
.global trampoline_t_blre_regs_proto_chain
.global trampoline_t_blre_regs_proto_target
.global trampoline_t_blre_regs_proto_end

.align 2
.thumb
trampoline_t_blre_regs_proto:
    push {lr}
    push {r0, r4-r7, lr} @ lr just to reserve space
    ldr r0, trampoline_t_blre_regs_proto_target
    mov lr, r0
    ldr r0, trampoline_t_blre_regs_proto_chain
    str r0, [sp, #20]  @where we pushed lr
    pop {r0}
    blx lr
    add sp, #4
    pop {pc}
.align 2
trampoline_t_blre_regs_proto_target:
    .word 0
trampoline_t_blre_regs_proto_chain:
    .word 0
trampoline_t_blre_regs_proto_end:


.global trampoline_t_hookbefore_proto
.global trampoline_t_hookbefore_proto_orgins
.global trampoline_t_hookbefore_proto_target
.global trampoline_t_hookbefore_proto_end

.align 2
.thumb
trampoline_t_hookbefore_proto:
    push {r0-r7,lr}
    ldr r7, trampoline_t_hookbefore_proto_target
    mov r0, sp
    blx r7
    ldr r0, [sp, #32]
    mov lr, r0
    pop {r0-r7}
    add sp, #4
trampoline_t_hookbefore_proto_orgins:
    nop
    nop
    bx lr
    nop
trampoline_t_hookbefore_proto_target:
    .word 0
trampoline_t_hookbefore_proto_end:


.global trampoline_t_hookbefore_bl_proto
.global trampoline_t_hookbefore_bl_proto_target
.global trampoline_t_hookbefore_bl_proto_chain
.global trampoline_t_hookbefore_bl_proto_end

.align 2
.thumb
trampoline_t_hookbefore_bl_proto:
    push {r0-r7, lr}
    ldr r7, trampoline_t_hookbefore_bl_proto_target
    mov r0, sp
    blx r7
    ldr r0, [sp, #32]
    mov lr, r0
    ldr r1, trampoline_t_hookbefore_bl_proto_chain
    str r1, [sp, #32]
    pop {r0-r7, pc}
.align 2
trampoline_t_hookbefore_bl_proto_chain:
    .word 0
trampoline_t_hookbefore_bl_proto_target:
    .word 0
trampoline_t_hookbefore_bl_proto_end:
