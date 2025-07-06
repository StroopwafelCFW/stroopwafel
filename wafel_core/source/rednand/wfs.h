#pragma once

#include "sal.h"

typedef enum WFSCryptoKeyHandle {
  WFS_KEY_HANLDE_RAMDISK=0x11,
  WFS_KEY_HANDLE_MLC=0x11,
  WFS_KEY_HANDLE_USB=0x12,
  WFS_KEY_HANDLE_MLCORIG=0x28,
  WFS_KEY_HANDLE_NOCRYPTO=0xDEADBEEF
} WFSCryptoKeyHandle;


struct WFS_Device {
    uint32_t bock_size_shift;
    uint32_t block_count_related;
    uint32_t crypto_key_handle;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t block_count_hi;
    FSSALHandle handle;
    void * workbuf;
    uint32_t device_type; /* 1 MLC; 2 RAMDISK; 3 USB */
    u8 field9_0x24;
    u8 maybe_status_flags; /* Created by retype action */
} __attribute__((packed)) typedef WFS_Device;

_Static_assert(sizeof(WFS_Device) == 0x26, "WFS_Device size must be 0x26!");