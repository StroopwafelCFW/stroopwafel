// this is all ripped from IOS, because no one can figure out just WTF is going on here

#include "cache.h"

#include "irq.h"
#include "latte.h"

void _dc_inval_entries(void *start, int count);
void _dc_flush_entries(const void *start, int count);
void _dc_flush(void);
void _ic_inval(void);
void _drain_write_buffer(void);

#define LINESIZE 0x20
#define CACHESIZE 0x4000

#define CR_MMU      (1 << 0)
#define CR_DCACHE   (1 << 2)
#define CR_ICACHE   (1 << 12)

void _ahb_flush_to(enum rb_client dev) {
    u32 mask;
    switch(dev)
    {
        case RB_IOD: mask = 0x8000; break;
        case RB_IOI: mask = 0x4000; break;
        case RB_AIM: mask = 0x0001; break;
        case RB_FLA: mask = 0x0002; break;
        case RB_AES: mask = 0x0004; break;
        case RB_SHA: mask = 0x0008; break;
        case RB_EHCI: mask = 0x0010; break;
        case RB_OHCI0: mask = 0x0020; break;
        case RB_OHCI1: mask = 0x0040; break;
        case RB_SD0: mask = 0x0080; break;
        case RB_SD1: mask = 0x0100; break;
        default:
            return;
    }

    if(read32(0xd8b0008) & mask) {
        return;
    }

    switch(dev) {
        case RB_FLA:
        case RB_AES:
        case RB_SHA:
        case RB_EHCI:
        case RB_OHCI0:
        case RB_OHCI1:
        case RB_SD0:
        case RB_SD1:
            while((read32(LT_BOOT0) & 0xF) == 9) {
                set32(LT_COMPAT_MEMCTRL_WORKAROUND, 0x10000);
            }
            clear32(LT_COMPAT_MEMCTRL_WORKAROUND, 0x10000);
            set32(LT_COMPAT_MEMCTRL_WORKAROUND, 0x2000000);
            mask32(LT_AHB_UNK124, 0x7C0, 0x280);
            set32(LT_AHB_UNK134, 0x400);
            while((read32(LT_BOOT0) & 0xF) != 9);
            set32(LT_AHB_UNK100, 0x400);
            set32(LT_AHB_UNK104, 0x400);
            set32(LT_AHB_UNK108, 0x400);
            set32(LT_AHB_UNK10C, 0x400);
            set32(LT_AHB_UNK110, 0x400);
            set32(LT_AHB_UNK114, 0x400);
            set32(LT_AHB_UNK118, 0x400);
            set32(LT_AHB_UNK11C, 0x400);
            set32(LT_AHB_UNK120, 0x400);
            clear32(0xd8b0008, mask);
            set32(0xd8b0008, mask);
            clear32(LT_AHB_UNK134, 0x400);
            clear32(LT_AHB_UNK100, 0x400);
            clear32(LT_AHB_UNK104, 0x400);
            clear32(LT_AHB_UNK108, 0x400);
            clear32(LT_AHB_UNK10C, 0x400);
            clear32(LT_AHB_UNK110, 0x400);
            clear32(LT_AHB_UNK114, 0x400);
            clear32(LT_AHB_UNK118, 0x400);
            clear32(LT_AHB_UNK11C, 0x400);
            clear32(LT_AHB_UNK120, 0x400);
            clear32(LT_COMPAT_MEMCTRL_WORKAROUND, 0x2000000);
            mask32(LT_AHB_UNK124, 0x7C0, 0xC0);
            break;
        case RB_IOD:
        case RB_IOI:
            set32(0xd8b0008, mask);
            break;
        default:
            break;
    }
}

// invalidate device and then starlet
void ahb_flush_to(enum rb_client dev)
{
    u32 mask;
    switch(dev) {
        case RB_IOD: mask = 0x8000; break;
        case RB_IOI: mask = 0x4000; break;
        case RB_AIM: mask = 0x0001; break;
        case RB_FLA: mask = 0x0002; break;
        case RB_AES: mask = 0x0004; break;
        case RB_SHA: mask = 0x0008; break;
        case RB_EHCI: mask = 0x0010; break;
        case RB_OHCI0: mask = 0x0020; break;
        case RB_OHCI1: mask = 0x0040; break;
        case RB_SD0: mask = 0x0080; break;
        case RB_SD1: mask = 0x0100; break;
        case RB_SD2: mask = 0x10000; break;
        case RB_SD3: mask = 0x20000; break;
        case RB_EHC1: mask = 0x40000; break;
        case RB_OHCI10: mask = 0x80000; break;
        case RB_EHC2: mask = 0x100000; break;
        case RB_OHCI20: mask = 0x200000; break;
        case RB_SATA: mask = 0x400000; break;
        case RB_AESS: mask = 0x800000; break;
        case RB_SHAS: mask = 0x1000000; break;
        default:
            debug_printf("ahb_flush_to(%d): Invalid device\n", dev);
            return;
    }

    u32 cookie = irq_kill();

    write32(AHMN_RDBI_MASK, mask);

    _ahb_flush_to(dev);
    if(dev != RB_IOD) {
        _ahb_flush_to(RB_IOD);
    }

    irq_restore(cookie);
}

// flush device and also invalidate memory
void ahb_flush_from(enum wb_client dev)
{
    u32 cookie = irq_kill();
    u16 req = 0;
    bool done = false;
    int i;

    switch(dev)
    {
        case WB_IOD:
            req = 0b0001;
            break;
        case WB_AIM:
        case WB_EHCI:
        case WB_EHC1:
        case WB_EHC2:
        case WB_SATA:
        case WB_DMAB:
            req = 0b0100;
            break;
        case WB_FLA:
        case WB_OHCI0:
        case WB_OHCI1:
        case WB_SD0:
        case WB_SD1:
        case WB_SD2:
        case WB_SD3:
        case WB_OHCI10:
        case WB_OHCI20:
        case WB_DMAC:
            req = 0b1000;
            break;
        case WB_AES:
        case WB_SHA:
        case WB_AESS:
        case WB_SHAS:
        case WB_DMAA:
            req = 0b0010;
            break;
        case WB_ALL:
            req = 0b1111;
            break;
        default:
            debug_printf("ahb_flush(%d): Invalid device\n", dev);
            goto done;
    }

    write16(MEM_FLUSH_MASK, req);

    for(i = 0; i < 1000000; i++)
    {
        if(!(read16(MEM_FLUSH_MASK) & req))
        {
            done = true;
            break;
        }
        //udelay(1);
    }

    if(!done)
    {
        debug_printf("ahb_flush(%d): Flush (0x%x) did not ack!\n", dev, req);
    }
done:
    irq_restore(cookie);
}

void dc_flushrange(const void *start, u32 size)
{
    u32 cookie = irq_kill();
    if(size > 0x4000) 
    {
        _dc_flush();
    } 
    else
    {
        void *end = ALIGN_FORWARD(((u8*)start) + size, LINESIZE);
        start = ALIGN_BACKWARD(start, LINESIZE);
        _dc_flush_entries(start, (end - start) / LINESIZE);
    }
    _drain_write_buffer();
    ahb_flush_from(WB_AIM);
    irq_restore(cookie);
}

void dc_invalidaterange(void *start, u32 size)
{
    u32 cookie = irq_kill();
    void *end = ALIGN_FORWARD(((u8*)start) + size, LINESIZE);
    start = ALIGN_BACKWARD(start, LINESIZE);
    _dc_inval_entries(start, (end - start) / LINESIZE);
    ahb_flush_to(RB_IOD);
    irq_restore(cookie);
}

void dc_flushall(void)
{
    u32 cookie = irq_kill();
    _dc_flush();
    _drain_write_buffer();
    ahb_flush_from(WB_AIM);
    irq_restore(cookie);
}

void ic_invalidateall(void)
{
    u32 cookie = irq_kill();
    _ic_inval();
    ahb_flush_to(RB_IOD);
    irq_restore(cookie);
}