#ifndef FSA_H
#define FSA_H

#include "types.h"

typedef struct
{
    u32 flags;      // 0x0
    u32 permissions;// 0x4
	u32 unk1[0x2];  // 0x8
	u32 size;       // 0x10 size in bytes
	u32 physsize;   // 0x14 physical size on disk in bytes
	u32 unk2[0x13];
}fileStat_s;

typedef struct
{
	fileStat_s dirStat;
	char name[0x100];
}directoryEntry_s;

typedef enum
{
    MEDIA_READY = 0,
    MEDIA_NOT_PRESENT = 1,
    MEDIA_INVALID = 2,
    MEDIA_DIRTY = 3,
    MEDIA_FATAL = 4
} media_states;

typedef struct
{
    u32 flags;
    u32 media_state;
    s32 device_index;
    u32 block_size;
    u32 logical_block_size;
    u32 read_align;
    u32 write_align;
    char dev_type[8]; // sdcard, slc, mlc, etc
    char fs_type[8]; // isfs, wfs, fat, etc
    char vol_label[0x80];
    char vol_id[0x80];
    char dev_node[0x10];
    char mount_path[0x80];
} fsa_volume_info;

#define FSA_MOUNTFLAGS_BINDMOUNT (1 << 0)
#define FSA_MOUNTFLAGS_GLOBAL (1 << 1)

LINKABLE int MCP_InstallGetInfo(int fd, char* path);
LINKABLE int MCP_Install(int fd, char* path);

LINKABLE int FSA_Open();

LINKABLE int FSA_Mount(int fd, char* device_path, char* volume_path, u32 flags, char* arg_string, int arg_string_len);
LINKABLE int FSA_Unmount(int fd, char* path, u32 flags);
LINKABLE int FSA_FlushVolume(int fd, char* volume);

LINKABLE int FSA_GetDeviceInfo(int fd, char* device_path, int type, u32* out_data);
LINKABLE int FSA_GetVolumeInfo(int fd, char* volume_path, int type, fsa_volume_info* out_data);

LINKABLE int FSA_MakeDir(int fd, char* path, u32 flags);
LINKABLE int FSA_OpenDir(int fd, char* path, int* outHandle);
LINKABLE int FSA_ReadDir(int fd, int handle, directoryEntry_s* out_data);
LINKABLE int FSA_CloseDir(int fd, int handle);

LINKABLE int FSA_MakeQuota(int fd, char* path, u32 mode, u64 size);

LINKABLE int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle);
LINKABLE int FSA_ReadFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
LINKABLE int FSA_WriteFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
LINKABLE int FSA_StatFile(int fd, int handle, fileStat_s* out_data);
LINKABLE int FSA_CloseFile(int fd, int fileHandle);

LINKABLE int FSA_ChangeMode(int fd, char* path, int mode);

LINKABLE int FSA_Format(int fd, char* device, char* fs_format, int flags, u32 what1, u32 what2);

LINKABLE int FSA_RawOpen(int fd, char* device_path, int* outHandle);
LINKABLE int FSA_RawRead(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
LINKABLE int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
LINKABLE int FSA_RawClose(int fd, int device_handle);

#endif
