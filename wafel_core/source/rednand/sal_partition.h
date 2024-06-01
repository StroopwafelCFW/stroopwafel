#pragma once

#include <wafel/types.h>
#include "sal.h"

#define DEVTYPE_USB 17
#define DEVTYPE_SD 6

extern u32 partition_offset;
extern u32 partition_size;

void patch_partition_attach_arg(FSSALAttachDeviceArg *attach_arg, int device_type);