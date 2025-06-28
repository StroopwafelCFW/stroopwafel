#ifndef SVC_H
#define SVC_H

#include "../types.h"
#include "../dynamic.h"
#include "ipc_types.h"

typedef IOSVec iovec_s;

typedef enum {
    MEM_PERM_R  = 1 << 0,
    MEM_PERM_W  = 1 << 1,
    MEM_PERM_RW = (MEM_PERM_R | MEM_PERM_W),
} MemPermFlags;

typedef int ios_retval;

LINKABLE void _wafel_SyscallStart();
#define wafel_SyscallStart ((uintptr_t)_wafel_SyscallStart)

LINKABLE uintptr_t wafel_SyscallTable[0x88];

// threading
LINKABLE int iosCreateThread(u32 (*proc)(void* arg), void* arg, u32* stack_top, u32 stacksize, int priority, u32 flags);
LINKABLE int iosJoinThread(int thread_id, u32* retval);
LINKABLE int iosCancelThread(int thread_id, u32 retval);
LINKABLE int iosGetThreadId();

LINKABLE int iosStartThread(int thread_id);
LINKABLE int iosSuspendThread(int thread_id);
LINKABLE int iosYieldThread();

LINKABLE int iosCreateSemaphore(int type, int initialCount);
LINKABLE int iosWaitSemaphore(int semaphore, int timeout);
LINKABLE void iosSignalSemaphore(int semaphore);
LINKABLE int iosDestroySemaphore(int semaphore);

LINKABLE int iosIpcResume(int fd, u32 system_mode, u32 power_flags);
LINKABLE int iosIpcSuspend(int fd);

LINKABLE int iosCreateMessageQueue(u32 *ptr, u32 n_msgs);
LINKABLE int iosDestroyMessageQueue(int queueid);
LINKABLE int iosSendMessage(int queueid, u32 message, u32 flags);
LINKABLE int iosJamMessage(int queueid, u32 message, u32 flags);
LINKABLE int iosReceiveMessage(int queueid, ipcmessage **message, u32 flags);

LINKABLE int iosCreateTimer(int time_us, int repeat_time_us, int queueid, u32 message);
LINKABLE int iosRestartTimer(int timerid, int time_us, int repeat_time_us);
LINKABLE int iosStopTimer(int timerid);
LINKABLE int iosDestroyTimer(int timerid);

LINKABLE int iosPanic(const char* msg, u32 msg_size);

LINKABLE void* iosAlloc(u32 heapid, u32 size);
LINKABLE void* iosAllocAligned(u32 heapid, u32 size, u32 align);
LINKABLE void iosFree(u32 heapid, void* ptr);
LINKABLE int32_t iosCheckIosAddrRange(void *ptr, uint32_t size, MemPermFlags perm);
LINKABLE uint32_t iosVirtToPhys(void *ptr);

LINKABLE int iosOpen(char* name, int mode);
LINKABLE int iosClose(int fd);
LINKABLE int iosIoctl(int fd, u32 request, void* input_buffer, u32 input_buffer_len, void* output_buffer, u32 output_buffer_len);
LINKABLE int iosIoctlv(int fd, u32 request, u32 vector_count_in, u32 vector_count_out, iovec_s* vector);
LINKABLE int iosInvalidateDCache(void* address, u32 size);
LINKABLE int iosFlushDCache(void* address, u32 size);
LINKABLE int iosBackdoor(u32, u32, u32, void* func);

LINKABLE int iosRegisterResourceManager(const char *device, int queueid);
LINKABLE int iosResourceReply(ipcmessage *ipc_message, u32 result);

LINKABLE int iosreturn_null();

LINKABLE int svc_sys_write(char* str);
LINKABLE void crash_and_burn();

#endif
