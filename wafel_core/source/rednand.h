#ifndef REDNAND_H
#define REDNAND_H

#include "types.h"
#include "rednand_config.h"

extern u32 redmlc_off_sectors;
extern u32 redmlc_size_sectors;
extern u32 redslc_off_sectors;
extern u32 redslc_size_sectors;
extern u32 redslccmpt_off_sectors;
extern u32 redslccmpt_size_sectors;

#define rednand (redmlc_size_sectors || redslc_size_sectors || redslccmpt_size_sectors)


void rednand_apply_mlc_patches(uint32_t redmlc_size_sectors);

void rednand_apply_slc_patches(void);

void rednand_apply_base_patches(void);

void rednand_init(rednand_config* config);
#endif