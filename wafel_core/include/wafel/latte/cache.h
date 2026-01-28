// this is all ripped from IOS, because no one can figure out just WTF is going on here

#ifndef _CACHE_H_
#define _CACHE_H_

#include "../types.h"
#include "../utils.h"

enum rb_client {
    RB_IOD = 0,
    RB_IOI = 1,
    RB_AIM = 2,
    RB_FLA = 3,
    RB_AES = 4,
    RB_SHA = 5,
    RB_EHCI = 6,
    RB_OHCI0 = 7,
    RB_OHCI1 = 8,
    RB_SD0 = 9,
    RB_SD1 = 10,
    RB_SD2 = 11,
    RB_SD3 = 12,
    RB_EHC1 = 13,
    RB_OHCI10 = 14,
    RB_EHC2 = 15,
    RB_OHCI20 = 16,
    RB_SATA = 17,
    RB_AESS = 18,
    RB_SHAS = 19
};

enum wb_client {
    WB_IOD = 0,
    WB_AIM = 1,
    WB_FLA = 2,
    WB_AES = 3,
    WB_SHA = 4,
    WB_EHCI = 5,
    WB_OHCI0 = 6,
    WB_OHCI1 = 7,
    WB_SD0 = 8,
    WB_SD1 = 9,
    WB_SD2 = 10,
    WB_SD3 = 11,
    WB_EHC1 = 12,
    WB_OHCI10 = 13,
    WB_EHC2 = 14,
    WB_OHCI20 = 15,
    WB_SATA = 16,
    WB_AESS = 17,
    WB_SHAS = 18,
    WB_DMAA = 19,
    WB_DMAB = 20,
    WB_DMAC = 21,
    WB_ALL = 22
};

#define CR_MMU      (1 << 0)
#define CR_DCACHE   (1 << 2)
#define CR_ICACHE   (1 << 12)

LINKABLE void dc_flushrange(const void *start, u32 size);
LINKABLE void dc_invalidaterange(void *start, u32 size);
LINKABLE void dc_flushall(void);
LINKABLE void ic_invalidateall(void);
LINKABLE void ahb_flush_from(enum wb_client dev);
LINKABLE void ahb_flush_to(enum rb_client dev);

LINKABLE  void _ahb_flush_to(enum rb_client dev);

#endif // _CACHE_H