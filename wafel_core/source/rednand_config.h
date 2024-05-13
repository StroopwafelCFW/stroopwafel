#include "types.h"

#ifndef REDNAND_CONFIG_H
#define REDNAND_CONFIG_H

typedef struct {
    u32 lba_start;
    u32 lba_length;
} PACKED rednand_partition;

typedef struct {
    rednand_partition slc;
    rednand_partition slccmpt;
    rednand_partition mlc;
    bool disable_scfm;
    bool scfm_on_slccmpt;
    bool initilized;
} PACKED rednand_config_v1;

typedef struct {
    rednand_partition slc;
    rednand_partition slccmpt;
    rednand_partition mlc;
    bool disable_scfm;
    bool scfm_on_slccmpt;
    bool initilized;
    //v2
    bool slc_nocrypto; //not implemented yet
    bool slccmpt_nocrypto; //not implemented yet
    bool mlc_nocrypto;
} PACKED rednand_config_v2;

typedef struct {
    rednand_partition slc;
    rednand_partition slccmpt;
    rednand_partition mlc;
    bool disable_scfm;
    bool scfm_on_slccmpt;
    bool initilized;
    //v2
    bool slc_nocrypto; //not implemented yet
    bool slccmpt_nocrypto; //not implemented yet
    bool mlc_nocrypto;
    //v3
    bool sys_mount_slc; //not implemented yet
    bool sys_mount_slccmpt; //not implemented yet
    bool sys_mount_mlc;
} PACKED rednand_config;

#endif
