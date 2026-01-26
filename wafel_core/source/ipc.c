#include "wafel/ios/ipc_types.h"
#include "wafel/ios/svc.h"
#include "utils.h"
#include "threads.h"

static int ipcNodeKilled;
static u8 threadStack[0x1000] __attribute__((aligned(0x20)));

static u32 ipc_thread(void *) {
    int res;
    ipcmessage *message;
    u32 messageQueue[0x10];

    int queueId = iosCreateMessageQueue(messageQueue, sizeof(messageQueue) / 4);

    if (iosRegisterResourceManager("/dev/stroopwafel", queueId) == 0) {
        while (!ipcNodeKilled) {
            res = iosReceiveMessage(queueId, &message, 0);
            if (res < 0) {
                usleep(10000);
                continue;
            }

            switch (message->command) {
                case IOS_OPEN: {
                    debug_printf("IPC: /dev/stoopwafel Open\n");
                    res = 0;
                    break;
                }
                case IOS_CLOSE: {
                    res = 0;
                    break;
                }
                case IOS_IOCTL: {
                    debug_printf("IPC: /dev/stoopwafel IOCTL %d\n", message->ioctl.command);
                    res = 0; //ipc_ioctl(message);
                    break;
                }
                case IOS_IOCTLV: {
                    debug_printf("IPC: /dev/stoopwafel IOCTLV %d\n", message->ioctlv.command);
                    res = 0; //ipc_ioctlv(message);
                    break;
                }
                default: {
                    res = IOS_ERROR_UNKNOWN_VALUE;
                    break;
                }
            }

            iosResourceReply(message, res);
        }
    }

    iosDestroyMessageQueue(queueId);
    return 0;
}

void ipc_init() {
    ipcNodeKilled = 0;

    int threadId = iosCreateThread(ipc_thread, 0, (u32 *) (threadStack + sizeof(threadStack)), sizeof(threadStack), 0x78, 1);
    if (threadId >= 0)
        iosStartThread(threadId);
}