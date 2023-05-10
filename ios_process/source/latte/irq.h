#ifndef _IRQ_H
#define _IRQ_H

#include "types.h"

static inline u32 get_cr(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c1, c0, 0" : "=r" (data) );
    return data;
}

static inline u32 get_ttbr(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c2, c0, 0" : "=r" (data) );
    return data;
}

static inline u32 get_dacr(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c3, c0, 0" : "=r" (data) );
    return data;
}

static inline void set_cr(u32 data)
{
    __asm__ volatile ( "mcr\tp15, 0, %0, c1, c0, 0" :: "r" (data) );
}

static inline void set_ttbr(u32 data)
{
    __asm__ volatile ( "mcr\tp15, 0, %0, c2, c0, 0" :: "r" (data) );
}

static inline void set_dacr(u32 data)
{
    __asm__ volatile ( "mcr\tp15, 0, %0, c3, c0, 0" :: "r" (data) );
}

static inline u32 get_dfsr(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c5, c0, 0" : "=r" (data) );
    return data;
}

static inline u32 get_ifsr(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c5, c0, 1" : "=r" (data) );
    return data;
}

static inline u32 get_far(void)
{
    u32 data;
    __asm__ volatile ( "mrc\tp15, 0, %0, c6, c0, 0" : "=r" (data) );
    return data;
}

u32 irq_kill(void);
void irq_restore(u32 cookie);

#endif // _IRQ_H