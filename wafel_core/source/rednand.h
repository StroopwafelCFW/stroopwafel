#ifndef REDNAND_H
#define REDNAND_H

#include "types.h"
#include "rednand_config.h"

#define DEVTYPE_MLC 5
#define DEVTYPE_SD 6
#define DEVTYPE_USB 17

// hope that 0x11 stays constant for mlc
#define MLC_CRYPTO_HANDLE 0x11
// tells crypto to not do crypto (depends on stroopwafel patch)
#define NO_CRYPTO_HANDLE 0xDEADBEEF

extern u32 redmlc_off_sectors;
extern u32 redmlc_size_sectors;
extern u32 redslc_off_sectors;
extern u32 redslc_size_sectors;
extern u32 redslccmpt_off_sectors;
extern u32 redslccmpt_size_sectors;

#define rednand (redmlc_size_sectors || redslc_size_sectors || redslccmpt_size_sectors)

void rednand_init(rednand_config* config, size_t config_size);

void rednand_patch_hai(void);
#endif