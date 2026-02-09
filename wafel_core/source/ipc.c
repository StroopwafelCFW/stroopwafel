#include <string.h>
#include <unistd.h>
#include "wafel/ios/ipc_types.h"
#include "wafel/ios/svc.h"
#include "wafel/ios/mmu.h"
#include "wafel/ios/prsh.h"
#include "wafel/ipc_commands.h"
#include "wafel/latte/cache.h"
#include "utils.h"
#include "ipc.h"
#include <stdio.h>

static int ipcNodeKilled;
static u8 threadStack[0x1000] __attribute__((aligned(0x20)));

static int ipc_ioctl(ipcmessage *message) {

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
            return 0;
        }
        case IOCTL_GET_API_VERSION: {
            if (message->ioctl.buffer_io && message->ioctl.length_io >= sizeof(u32)) {
                *((u32*)message->ioctl.buffer_io) = WAFEL_API_VERSION;
                debug_printf("IPC: Get API Version: 0x%08X\n", WAFEL_API_VERSION);
                return sizeof(u32);
            } else {
                return IOS_ERROR_INVALID_ARG;
            }
            break;
        }
        case IOCTL_MAP_MEMORY: {
            if (message->ioctl.buffer_in && message->ioctl.length_in >= sizeof(ios_map_shared_info_t)) {
                int (*_iosMapSharedUserExecution)(void *descr) = (void *) 0x08124F88;
                int _res = _iosMapSharedUserExecution(message->ioctl.buffer_in);
                debug_printf("IPC: Map Memory: res %d\n", _res);
                return _res;
            } else {
                return IOS_ERROR_INVALID_ARG;
            }
            break;
        }
        case IOCTL_GET_MINUTE_PATH: {
            minute_path_t *out = (minute_path_t *)message->ioctl.buffer_io;
            if (out && message->ioctl.length_io >= sizeof(minute_path_t)) {
                u32 minute_on_slc = !prsh_get_entry("minute_on_slc", NULL, NULL);
                const char *filename = "fw.img";
                u32 *p_location = NULL;
                if (prsh_get_entry("minute_location", (void**)&p_location, NULL) == 0 && p_location) {
                    u32 location = *p_location;
                    minute_on_slc = location & 1;
                    if ((location >> 1) & 1) filename = "minute.img";
                }

                if (minute_on_slc) {
                    out->device = MIN_DEV_SLC;
                    snprintf(out->path, sizeof(out->path), "/sys/hax/%s", filename);
                } else {
                    out->device = MIN_DEV_SD;
                    snprintf(out->path, sizeof(out->path), "/%s", filename);
                }
                debug_printf("IPC: Get Minute Path: dev %d, path %s\n", out->device, out->path);
                return sizeof(minute_path_t);
            } else {
                return IOS_ERROR_INVALID_ARG;
            }
            break;
        }
        case IOCTL_GET_PLUGIN_PATH: {
            printf("IPC: IOCTL_GET_PLUGIN_PATH\n");
            minute_path_t *out = (minute_path_t *)message->ioctl.buffer_io;
            if (out && message->ioctl.length_io >= sizeof(minute_path_t)) {
                printf("IPC: IOCTL_GET_PLUGIN_PATH out buffer is valid\n");
                u32 boot = 0;
                if (prsh_get_entry("minute_boot", (void**)&boot, NULL)) {
                    out->device = MIN_DEV_UNKNOWN;
                    out->path[0] = '\0';
                    debug_printf("IPC: minute_boot PRSH entry not found\n");
                }
                printf("IPC: minute_boot value: %u\n", boot);
                if (boot == 1) {
                    out->device = MIN_DEV_SLC;
                    snprintf(out->path, sizeof(out->path), "/sys/hax/ios_plugins");
                } else if (boot >= 2 && boot <= 5) {
                    out->device = MIN_DEV_SD;
                    snprintf(out->path, sizeof(out->path), "/wiiu/ios_plugins");
                } else {
                    out->device = MIN_DEV_UNKNOWN;
                    out->path[0] = '\0';
                }
                debug_printf("IPC: Get Plugin Path: dev %d, path %s\n", out->device, out->path);
                return sizeof(minute_path_t);
            } else {
                return IOS_ERROR_INVALID_ARG;
            }
            break;
        }
        default:
            return IOS_ERROR_INVALID_ARG;
    }

    return 0;
}

static int ipc_ioctlv(ipcmessage *message) {

    switch (message->ioctlv.command) {
        case IOCTLV_WRITE_MEMORY: {
            if (message->ioctlv.num_in < 1 || message->ioctlv.num_io != 0) {
                return IOS_ERROR_INVALID_ARG;
            }

            if (message->ioctlv.vector[0].len % sizeof(u32) != 0) {
                return IOS_ERROR_INVALID_SIZE;
            }

            u32 *dest_addrs = (u32 *)message->ioctlv.vector[0].ptr;
            u32 num_writes = message->ioctlv.vector[0].len / sizeof(u32);

            if (message->ioctlv.num_in != 1 + num_writes) {
                return IOS_ERROR_INVALID_SIZE;
            }

            debug_printf("IPC: IOCTLV_WRITE_MEMORY, num_writes: %d\n", num_writes);
            //dc_flushrange(NULL, 0xFFFFFFFF);

            for (u32 i = 0; i < num_writes; i++) {
                u8 *dest = (u8*)dest_addrs[i];
                u8 *src = (u8*)message->ioctlv.vector[i + 1].ptr;
                u32 size = message->ioctlv.vector[i + 1].len;
                debug_printf("write %d: %p -> %p (%d)\n", i, src, dest, size);
                if (src && size > 0) {
                    // for(size_t k=0; k<size; k++)
                    //     dest[i] = src[i];
                    KERN_memcpy(dest, src, size);
                    flush_dcache(dest, size);
                    //dc_flushrange(dest, size);
                }
            }
            //debug_printf("dc_flushrange\n");
            //dc_flushrange(NULL, 0xFFFFFFFF);
            debug_printf("ic_invalidateall\n");
            ic_invalidateall();
            debug_printf("IPC: IOCTLV_WRITE_MEMORY FINISHED\n");
            return 0;
        }
        case IOCTLV_EXECUTE: {
            if (message->ioctlv.num_in < 1 || message->ioctlv.vector[0].len < sizeof(u32)) {
                return IOS_ERROR_INVALID_ARG;
            }

            u32 target_addr = *(u32 *)message->ioctlv.vector[0].ptr;
            void *config = (message->ioctlv.num_in > 1) ? message->ioctlv.vector[1].ptr : NULL;
            void *output = (message->ioctlv.num_io > 0) ? message->ioctlv.vector[message->ioctlv.num_in].ptr : NULL;

            debug_printf("IPC: IOCTLV_EXECUTE, target: 0x%08X, config: %p, output: %p\n", target_addr, config, output);

            if (target_addr) {
                void (*func)(void *, void *) = (void (*)(void *, void *))target_addr;
                func(config, output);
            }

            return 0;
        }
        default:
            return IOS_ERROR_INVALID_ARG;
    }

    return 0;
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
                    res = ipc_ioctlv(message);
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
        // set system mode in thread cpsr
        *(u32*)(0xffff4d78 + 0xc8 * threadId) |= 0x1f;
        iosStartThread(threadId);
}