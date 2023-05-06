#ifndef SVC_H
#define SVC_H

#include "types.h"

typedef struct
{
	void* ptr;
	u32 len;
	u32 unk;
}iovec_s;

void* svcAlloc(u32 heapid, u32 size);
void* svcAllocAlign(u32 heapid, u32 size, u32 align);
void svcFree(u32 heapid, void* ptr);
int svcOpen(char* name, int mode);
int svcClose(int fd);
int svcIoctl(int fd, u32 request, void* input_buffer, u32 input_buffer_len, void* output_buffer, u32 output_buffer_len);
int svcIoctlv(int fd, u32 request, u32 vector_count_in, u32 vector_count_out, iovec_s* vector);
int svcInvalidateDCache(void* address, u32 size);
int svcFlushDCache(void* address, u32 size);
int svcBackdoor(u32, u32, u32, void* func);

// threading
int svcCreateThread(u32 (*proc)(void* arg), void* arg, u32* stack_top, u32 stacksize, int priority, u32 flags);
int svcJoinThread(int thread_id, u32* retval);
int svcCancelThread(int thread_id, u32 retval);
int svcGetThreadId();

int svcStartThread(int thread_id);
int svcSuspendThread(int thread_id);
int svcYieldThread();

int svc_sys_write(char* str);
void crash_and_burn();

#endif
