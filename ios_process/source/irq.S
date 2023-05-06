.globl irq_kill
.globl irq_restore

irq_kill:
    mrs     r1, cpsr
    and     r0, r1, #(0x80|0x40)
    orr     r1, r1, #(0x80|0x40)
    msr     cpsr_c, r1
    bx      lr

irq_restore:
    mrs     r1, cpsr
    bic     r1, r1, #(0x80|0x40)
    orr     r1, r1, r0
    msr     cpsr_c, r1
    bx      lr