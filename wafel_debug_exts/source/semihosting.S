.arm
.global svc_handler_hook
.type svc_handler_hook, %function
svc_handler_hook:
	push {r0-r12, lr}
	ldrh r0, [lr, #-2]
	ldr r1, =0xDFAB
	cmp r0, r1
	beq do_svc
	pop {r0-r12, lr}

	ldr pc, old_svc_handler
do_svc:
	ldr r0, [sp, #0x0]
	ldr r1, [sp, #0x4]
	bl semihosting_handler
	.word 0xE8FD9FFF @ pop {r0-r12, pc}^

.align 4
.global old_svc_handler
old_svc_handler:
	.word 0x0