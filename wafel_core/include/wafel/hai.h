#ifndef HAI_H
#define HAI_H

#include "dynamic.h"

LINKABLE void hai_redirect_mlc2sd(void);

LINKABLE void hai_companion_add_offset(uint32_t *buffer, uint32_t offset);

LINKABLE void hai_apply_getdev_patch(void);

LINKABLE int hai_getdev(void);

LINKABLE void hai_apply_force_mlc_patch(void);

LINKABLE void hai_setdev(int devid);

#endif