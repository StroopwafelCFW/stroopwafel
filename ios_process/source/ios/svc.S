.section ".text"
.arm
.align 4

.arm
.global svcAlloc
.type svcAlloc, %function
svcAlloc:
	.word 0xE7F027F0
	bx lr

.arm
.global svcAllocAlign
.type svcAllocAlign, %function
svcAllocAlign:
	.word 0xE7F028F0
	bx lr

.arm
.global svcFree
.type svcFree, %function
svcFree:
	.word 0xE7F029F0
	bx lr

.arm
.global svcOpen
.type svcOpen, %function
svcOpen:
	.word 0xE7F033F0
	bx lr

.arm
.global svcClose
.type svcClose, %function
svcClose:
	.word 0xE7F034F0
	bx lr

.arm
.global svcIoctl
.type svcIoctl, %function
svcIoctl:
	.word 0xE7F038F0
	bx lr

.arm
.global svcIoctlv
.type svcIoctlv, %function
svcIoctlv:
	.word 0xE7F039F0
	bx lr

.arm
.global svcInvalidateDCache
.type svcInvalidateDCache, %function
svcInvalidateDCache:
	.word 0xE7F051F0
	bx lr

.arm
.global svcFlushDCache
.type svcFlushDCache, %function
svcFlushDCache:
	.word 0xE7F052F0
	bx lr

.arm
.global svcBackdoor
.type svcBackdoor, %function
svcBackdoor:
	.word 0xE7F081F0
	bx lr
    

.arm
.global svcCreateThread
.type svcCreateThread, %function
svcCreateThread:
	.word 0xE7F000F0
	bx lr

.arm
.global svcJoinThread
.type svcJoinThread, %function
svcJoinThread:
	.word 0xE7F001F0
	bx lr

.arm
.global svcCancelThread
.type svcCancelThread, %function
svcCancelThread:
	.word 0xE7F002F0
	bx lr

.arm
.global svcGetThreadId
.type svcGetThreadId, %function
svcGetThreadId:
	.word 0xE7F003F0
	bx lr

.arm
.global svcStartThread
.type svcStartThread, %function
svcStartThread:
	.word 0xE7F007F0
	bx lr

.arm
.global svcSuspendThread
.type svcSuspendThread, %function
svcSuspendThread:
	.word 0xE7F008F0
	bx lr

.arm
.global svcYieldThread
.type svcYieldThread, %function
svcYieldThread:
	.word 0xE7F009F0
	bx lr

.thumb_func
.thumb
.global svc_sys_write
.type svc_sys_write, %function
svc_sys_write:
	mov r2, lr
	mov r1, r0
	mov r0, #4
	svc 0xAB
	bx r2

.arm
.global crash_and_burn
.type crash_and_burn, %function
crash_and_burn:
	.word 0xFFFFFFFF
