.section ".text"
.arm
.align 4

.macro SVC_MACRO _idx,_name
.arm
.global \_name
.type \_name, %function
.weak \_name
\_name\():
	.word (0xE7F000F0 | \_idx << 8)
	bx lr
.endm

.extern semihosting_handler

.global _wafel_SyscallStart
_wafel_SyscallStart:

SVC_MACRO 0x00, iosCreateThread
SVC_MACRO 0x01, iosJoinThread
SVC_MACRO 0x02, iosCancelThread
SVC_MACRO 0x03, iosGetThreadId
SVC_MACRO 0x04, iosAccessIpcBufferPool
SVC_MACRO 0x05, iosGetProcessId
SVC_MACRO 0x07, iosStartThread
SVC_MACRO 0x08, iosStopThread
SVC_MACRO 0x09, iosYieldThread
SVC_MACRO 0x0A, iosGetThreadPriority
SVC_MACRO 0x0B, iosSetThreadPriority
SVC_MACRO 0x0C, iosCreateMessageQueue
SVC_MACRO 0x0D, iosDestroyMessageQueue
SVC_MACRO 0x0E, iosSendMessage
SVC_MACRO 0x0F, iosJamMessage
SVC_MACRO 0x10, iosReceiveMessage
SVC_MACRO 0x11, iosHandleEvent
SVC_MACRO 0x12, iosUnhandleEvent
SVC_MACRO 0x13, iosCreateTimer
SVC_MACRO 0x14, iosRestartTimer
SVC_MACRO 0x15, iosStopTimer
SVC_MACRO 0x16, iosDestroyTimer
SVC_MACRO 0x17, iosGetTimestamp
SVC_MACRO 0x18, iosTimeNow
SVC_MACRO 0x19, iosGetUpTimeStruct
SVC_MACRO 0x1A, iosGetUpTime64
SVC_MACRO 0x1B, iosSetRtcCounter
SVC_MACRO 0x1C, iosGetAbsTimeCalendar
SVC_MACRO 0x1D, iosGetAbsTime64
SVC_MACRO 0x1E, iosGetAbsTimeStruct
SVC_MACRO 0x1F, iosset_fault_behavior
SVC_MACRO 0x20, iosCheckDebugMode
SVC_MACRO 0x21, iosCheckJTAG
SVC_MACRO 0x22, iosReadOTP
SVC_MACRO 0x23, iosCreateHeap
SVC_MACRO 0x24, iosCreateLocalProcessHeap
SVC_MACRO 0x25, iosCreateCrossProcessHeap
SVC_MACRO 0x26, iosDestroyHeap
SVC_MACRO 0x27, iosAlloc
SVC_MACRO 0x28, iosAllocAligned
SVC_MACRO 0x29, iosFree
SVC_MACRO 0x2A, iosFreeAndClear
SVC_MACRO 0x2B, iosExpandHeap
SVC_MACRO 0x2C, iosRegisterResourceManager
SVC_MACRO 0x2D, iosDeviceAssociate
SVC_MACRO 0x2E, iosDeviceSetFlags
SVC_MACRO 0x2F, iosSetClient_capabilities
SVC_MACRO 0x30, iosClearClientCapabilities
SVC_MACRO 0x31, iosQueryClientCapabilities
SVC_MACRO 0x32, iosQueryFeatureId
SVC_MACRO 0x33, iosOpen
SVC_MACRO 0x34, iosClose
SVC_MACRO 0x35, iosRead
SVC_MACRO 0x36, iosWrite
SVC_MACRO 0x37, iosSeek
SVC_MACRO 0x38, iosIoctl
SVC_MACRO 0x39, iosIoctlv
SVC_MACRO 0x3A, iosOpenAsync
SVC_MACRO 0x3B, iosCloseAsync
SVC_MACRO 0x3C, iosReadAsync
SVC_MACRO 0x3D, iosWriteAsync
SVC_MACRO 0x3E, iosSeekAsync
SVC_MACRO 0x3F, iosIoctlAsync
SVC_MACRO 0x40, iosIoctlvAsync
SVC_MACRO 0x41, iosopen_as_async
SVC_MACRO 0x42, ioswrite_as_async
SVC_MACRO 0x43, iosIpcResume
SVC_MACRO 0x44, iosIpcSuspend
SVC_MACRO 0x45, iosIpcSvcMsg
SVC_MACRO 0x46, iosIpcResumeAsync
SVC_MACRO 0x47, iosIpcSuspendAsync
SVC_MACRO 0x48, iosIpcSvcmsgAsync
SVC_MACRO 0x49, iosResourceReply
SVC_MACRO 0x4A, iosSetProcUnkParams
SVC_MACRO 0x4B, iosGetProcUnkParams
SVC_MACRO 0x4C, iosAhbMemFlush
SVC_MACRO 0x4D, iosAhbFlushFrom
SVC_MACRO 0x4E, iosAhbFlushCheck
SVC_MACRO 0x4F, iosAhbFlushTo
SVC_MACRO 0x50, iosIrqEnable
SVC_MACRO 0x51, iosInvalidateDCache
SVC_MACRO 0x52, iosFlushDCache
SVC_MACRO 0x53, iosExecutePrivileged
SVC_MACRO 0x54, iosget_unk_flags1
SVC_MACRO 0x55, iosget_unk_flags2
SVC_MACRO 0x56, iosVirtToPhys
SVC_MACRO 0x57, iosCreateSemaphore
SVC_MACRO 0x58, iosWaitSemaphore
SVC_MACRO 0x59, iosSignalSemaphore
SVC_MACRO 0x5A, iosDestroySemaphore
SVC_MACRO 0x5B, iosFlushIPCServer
SVC_MACRO 0x5C, iosSetBspReady
SVC_MACRO 0x5D, iosCheckIosAddrRange
SVC_MACRO 0x5E, iosCheckPpcAddrRange
SVC_MACRO 0x5F, iosInitMem1Ppc
SVC_MACRO 0x60, iosGetIopCpuUtilization
SVC_MACRO 0x61, iosGetThreadStackInfo
SVC_MACRO 0x62, iosThreadProfileCommand
SVC_MACRO 0x63, iosGetThreadUtilization
SVC_MACRO 0x64, iosDumpThreadContext
SVC_MACRO 0x65, iosDumpThreadProfile
SVC_MACRO 0x66, iosGetMessageUtilization
SVC_MACRO 0x67, iosGetAggregateResourceUtilization
SVC_MACRO 0x68, iosGetPerProcessResourceUtilization
SVC_MACRO 0x69, iosGetTimerUtilization
SVC_MACRO 0x6A, iosGetSemaphoreUtilization
SVC_MACRO 0x6B, iosGetHeapProfile
SVC_MACRO 0x6C, iosSetIop2x
SVC_MACRO 0x6D, iosSetPpcBootParams
SVC_MACRO 0x6E, iosGetSyscfgRegisterValue
SVC_MACRO 0x6F, iosClearSyscfgRegisterValue
SVC_MACRO 0x70, iosSetSyscfgRegisterValue
SVC_MACRO 0x71, iosCheckSyscfgFlag
SVC_MACRO 0x72, iosShutdown
SVC_MACRO 0x73, iosPanic
SVC_MACRO 0x74, iosReset
SVC_MACRO 0x75, iosSetPanicBehavior
SVC_MACRO 0x76, iosSetSyslogBuffer
SVC_MACRO 0x77, iosLoadPpcKernel
SVC_MACRO 0x78, iosLoadPpcApp
SVC_MACRO 0x79, iosSetSecurityLevel
SVC_MACRO 0x7A, iosGetSecurityLevel
SVC_MACRO 0x7B, iosGetOpenResourceHandles
SVC_MACRO 0x7C, iosSetMainTitleSdkVersion
SVC_MACRO 0x7D, iosGetMainTitleSdkVersion
SVC_MACRO 0x7E, iosGetDynamicHeapAccess
SVC_MACRO 0x7F, iosStartGpuConfigValidation
SVC_MACRO 0x80, iosFinishGpuConfigValidation
SVC_MACRO 0x81, iosreturn_null
SVC_MACRO 0x82, iosGetResourceViolations
SVC_MACRO 0x83, iosDeviceGetClientHandles
SVC_MACRO 0x84, iosDeviceDisableRegistration
SVC_MACRO 0x85, iosGetPendingResourceRequests
SVC_MACRO 0x86, iosLaunchOS
SVC_MACRO 0x87, iosEXIReset

.arm
.global svc_sys_write
.global svc_sys_write_arm
.type svc_sys_write_arm, %function
svc_sys_write_arm:
	adr r12, svc_sys_write+1
	bx r12

.pool

.thumb_func
.thumb
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