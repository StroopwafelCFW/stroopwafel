#include <string.h>
#include <unistd.h>
#include "wafel/ios/ipc_types.h"
#include "wafel/ios/svc.h"
#include "wafel/ipc_commands.h"
#include "utils.h"
#include "ipc.h"

static int ipcNodeKilled;
static u8 threadStack[0x1000] __attribute__((aligned(0x20)));

static int ipc_ioctl(ipcmessage *message) {
    int res = 0;

    switch (message->ioctl.command) {
        case IOCTL_SET_FW_PATH: {
            char *path_buffer = (char *) message->ioctl.buffer_in;
            if (path_buffer) {
                // Find the last '/' to split path and filename
                char *last_slash = strrchr(path_buffer, '/');
                if (last_slash) {
                    int path_len = last_slash - path_buffer;
                    strncpy(fw_img_path_buffer, path_buffer, path_len);
                    fw_img_path_buffer[path_len] = '\0';

                    strncpy(fw_img_filename_buffer, last_slash + 1, sizeof(fw_img_filename_buffer) - 1);
                    fw_img_filename_buffer[sizeof(fw_img_filename_buffer) - 1] = '\0';
                    
                    debug_printf("IPC: Set fw_img_path_buffer to '%s' and fw_img_filename_buffer to '%s'\n", fw_img_path_buffer, fw_img_filename_buffer);
                } else {
                    // No slash, treat the whole string as filename
                    strncpy(fw_img_filename_buffer, path_buffer, sizeof(fw_img_filename_buffer) - 1);
                    fw_img_filename_buffer[sizeof(fw_img_filename_buffer) - 1] = '\0';
                    fw_img_path_buffer[0] = '\0'; // No path
                    debug_printf("IPC: Set fw_img_filename_buffer to '%s' (no path)\n", fw_img_filename_buffer);
                }
            }
            res = 0;
            break;
        }
        default:
            res = IOS_ERROR_INVALID_ARG;
            break;
    }

    return res;
}


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
                    res = ipc_ioctl(message);
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