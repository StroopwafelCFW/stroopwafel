#include <string.h>
#include <wafel/ios/svc.h>
#include <wafel/utils.h>
#include "sal.h"
#include "sal_partition.h"

#define SECTOR_SIZE 512

u32 partition_offset = 0xFFFFFFF;
u32 partition_size = 0;


static read_func *real_read;
static write_func *real_write;
static sync_func *real_sync;

#define ADD_OFFSET(high, low) do { \
    unsigned long long combined = ((unsigned long long)(high) << 32) | (low); \
    combined += partition_offset; \
    (high) = (unsigned int)(combined >> 32); \
    (low) = (unsigned int)(combined & 0xFFFFFFFF); \
} while (0)

int read_wrapper(void *device_handle, u32 lba_hi, u32 lba_lo, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    ADD_OFFSET(lba_hi, lba_lo);
    return real_read(device_handle, lba_hi, lba_lo, blkCount, blockSize, buf, cb, cb_ctx);
}

int write_wrapper(void *device_handle, u32 lba_hi, u32 lba_lo, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    ADD_OFFSET(lba_hi, lba_lo);
    int ret = real_write(device_handle, lba_hi, lba_lo, blkCount, blockSize, buf, cb, cb_ctx);
    //debug_printf("WFSWRITE: %u, %u\n", lba_lo, blkCount);
    return ret;
}

int sync_wrapper(int server_handle, u32 lba_hi, u32 lba_lo, u32 num_blocks, void * cb, void * cb_ctx){
    ADD_OFFSET(lba_hi, lba_lo);
    //debug_printf("%s: sync called lba: %d, num_blocks: %d\n", PLUGIN_NAME, lba_lo, num_blocks);
    return real_sync(server_handle, lba_hi, lba_lo, num_blocks, cb, cb_ctx);
}


static void readop2_crash(int *device_handle, u32 lba_hi, u32 lba, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    debug_printf("%s ERROR: readop2 was called!!!! handle: %p type: %u\n", PLUGIN_NAME, device_handle, device_handle[5]);
    crash_and_burn();
}

static void writeop2_crash(int *device_handle, u32 lba_hi, u32 lba, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    debug_printf("%s ERROR: readop2 was called!!!! handle: %p type: %u\n", PLUGIN_NAME, device_handle, device_handle[5]);
    crash_and_burn();
}

void patch_partition_attach_arg(FSSALAttachDeviceArg *attach_arg, int device_type){
    real_read = attach_arg->op_read;
    real_write = attach_arg->op_write;
    real_sync = attach_arg->opsync;
    attach_arg->op_read = read_wrapper;
    attach_arg->op_write = write_wrapper;
    attach_arg->op_read2 = readop2_crash;
    attach_arg->op_write2 = writeop2_crash;
    attach_arg->opsync = sync_wrapper;
    attach_arg->params.device_type = device_type;
    attach_arg->params.max_lba_size = partition_size -1;
    attach_arg->params.block_count = partition_size;
}