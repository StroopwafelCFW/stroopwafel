#ifndef REDNAND_H
#define REDNAND_H

#include "types.h"

void rednand_apply_mlc_patches(uint32_t redmlc_size_sectors);

void rednand_apply_slc_patches(void);

void rednand_apply_base_patches(void);

#endif