#ifndef WAFEL_IPC_COMMANDS_H
#define WAFEL_IPC_COMMANDS_H

#include "wafel/types.h"

#define IOCTL_GET_API_VERSION 0x1
#define IOCTL_SET_FW_PATH 0x2
#define IOCTLV_WRITE_MEMORY 0x3
#define IOCTLV_EXECUTE 0x4
#define IOCTL_MAP_MEMORY 0x5
#define IOCTL_GET_MINUTE_PATH 0x6
#define IOCTL_GET_PLUGIN_PATH 0x7

#define WAFEL_API_VERSION 0x010000 // v1.0.0

typedef enum {
    MIN_DEV_UNKNOWN = 0,
    MIN_DEV_SLC = 1,
    MIN_DEV_SD = 2,
} minute_device_t;

typedef struct {
    u32 device;
    char path[256];
} minute_path_t;

#endif // WAFEL_IPC_COMMANDS_H
