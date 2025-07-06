#pragma once

#include <wafel/types.h>


typedef enum SALDeviceType { /* from dimok, unknown accuracy https://github.com/dimok789/mocha/blob/43f1af08a345b333e800ac1e2e18e7aa153e9da9/ios_fs/source/devices.h#L55 */
    SAL_DEVICE_SI=1,
    SAL_DEVICE_ODD=2,
    SAL_DEVICE_SLCCMPT=3,
    SAL_DEVICE_SLC=4,
    SAL_DEVICE_MLC=5,
    SAL_DEVICE_SDCARD=6,
    SAL_DEVICE_SD=7,
    SAL_DEVICE_HFIO=8,
    SAL_DEVICE_RAMDISK=9,
    SAL_DEVICE_USB=17,
    SAL_DEVICE_MLCORIG=18
} SALDeviceType;

struct FSSALHandle {
    enum SALDeviceType type :8;
    u8 index;
    u16 generation;
} __attribute__((packed)) typedef FSSALHandle;

_Static_assert(sizeof(FSSALHandle) == 4, "FSSALHandle size must be 4!");

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

struct FSLinkedQueueEntry {
    struct FSLinkedQueueEntry * next;
} __attribute__((packed));

struct FSSALFilesystem {
    struct FSLinkedQueueEntry link;
    FSSALHandle handle;
    uint32_t add_cb;
    uint32_t field3_0xc;
    bool (* attach_device_cb)(FSSALHandle, bool);
    char allowed_devices[12];
} __attribute__((packed));

struct FSSALDevice {
    struct FSLinkedQueueEntry link;
    FSSALHandle handle;
    struct FSSALFilesystem * filesystem;
    void *server_handle;
    uint32_t device_type;
    uint32_t field5_0x14;
    uint32_t allowed_ops;
    uint32_t media_state;
    uint32_t max_lba_size_hi;
    uint32_t max_lba_size;
    uint32_t field10_0x28;
    uint32_t field11_0x2c;
    uint32_t block_count;
    uint32_t block_count_hi;
    uint32_t blockSize;
    uint32_t field15_0x3c;
    uint32_t alignment_smth;
    uint32_t field17_0x44;
    uint32_t field18_0x48;
    uint32_t field19_0x4c;
    uint32_t field20_0x50;
    char field21_0x54[128];
    char field22_0xd4[128];
    char field23_0x154[128];
    read_func *op_read;
    read_func *op_read_2;
    write_func *op_write;
    write_func *op_write_2;
    uint32_t field28_0x1e4;
    uint32_t field29_0x1e8;
    sync_func * op_sync;
    uint32_t field31_0x1f0;
    uint32_t field32_0x1f4;
    uint32_t field33_0x1f8;
    uint32_t field34_0x1fc;
    uint32_t field35_0x200;
} __attribute__((packed)) typedef FSSALDevice;

_Static_assert(sizeof(FSSALDevice) == 0x204, "FSSALDevice size must be 0x204!");

static FSSALHandle (*FSSAL_attach_device)(FSSALAttachDeviceArg*) = (void*)0x10733aa4;
static FSSALHandle (*FSSCFM_attach_device)(FSSALAttachDeviceArg*) = (void*)0x107d1f04;
static FSSALDevice* (*FSSAL_LookupDevice)(FSSALHandle) = (void*)0x10733990;