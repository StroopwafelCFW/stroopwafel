#pragma once

#include <wafel/types.h>

struct FSSALDeviceParams {
    uint32_t flags;
    uint32_t device_type;
    uint32_t unk1;
    uint32_t allowd_ops;
    uint32_t media_state;
    uint64_t max_lba_size;
    uint32_t unk2;
    uint32_t unk3;
    uint64_t block_count;
    uint32_t block_size;
    uint32_t unk4;
    uint32_t alignment_smth;
    uint32_t unk5;
    uint32_t unk6;
    uint32_t unk7;
    uint32_t unk8;
    char unk9[128];
    char unk10[128];
    char unk11[128];
} __attribute__((packed)) typedef FSSALDeviceParams;

_Static_assert(sizeof(FSSALDeviceParams) == 0x1c8, "FSSALAttachDeviceArg size must be 0x1c8!");


typedef int read_func(void *device_handle, u32 lba_hi, u32 lba_lo, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ct);
typedef int write_func(void *device_handle, u32 lba_hi, u32 lba_lo, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ct);
typedef int sync_func(int server_handle, u32 lba_hi, u32 lba_lo, u32 num_blocks, void * cb, void * cb_ctx);

struct FSSALAttachDeviceArg {
    void *server_handle;
    FSSALDeviceParams params;
    read_func *op_read;
    void *op_read2;
    write_func *op_write;
    void *op_write2;
    uint32_t unk1;
    uint32_t unk2;
    sync_func *opsync;	
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
    uint32_t unk6;
    uint32_t unk7;

} __attribute__((packed)) ALIGNED(4) typedef FSSALAttachDeviceArg;

#define test sizeof(FSSALAttachDeviceArg)

_Static_assert(sizeof(FSSALAttachDeviceArg) == 0x1fc, "FSSALAttachDeviceArg size must be 0x1fc!");