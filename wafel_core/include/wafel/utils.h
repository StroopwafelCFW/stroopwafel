#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define ALIGN_FORWARD(x,align) \
    ((__typeof__(x))((((u32)(x)) + (align) - 1) & (~(align-1))))

#define ALIGN_BACKWARD(x,align) \
    ((__typeof__(x))(((u32)(x)) & (~(align-1))))

/*
 * These functions are guaranteed to copy by reading from src and writing to dst in <n>-bit units
 * If size is not aligned, the remaining bytes are not copied
 */
LINKABLE void memset32(void *dst, u32 value, u32 size);
LINKABLE void memcpy32(void *dst, void *src, u32 size);
LINKABLE void memset16(void *dst, u16 value, u32 size);
LINKABLE void memcpy16(void *dst, void *src, u32 size);
LINKABLE void memset8(void *dst, u8 value, u32 size);
LINKABLE void memcpy8(void *dst, void *src, u32 size);

static inline u32 read32(u32 addr)
{
    u32 data;
    __asm__ volatile ("ldr\t%0, [%1]" : "=l" (data) : "l" (addr));
    return data;
}

static inline void write32(u32 addr, u32 data)
{
    __asm__ volatile ("str\t%0, [%1]" : : "l" (data), "l" (addr));
}

static inline u32 set32(u32 addr, u32 set)
{
    u32 data;
    __asm__ volatile (
        "ldr\t%0, [%1]\n"
        "\torr\t%0, %2\n"
        "\tstr\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set)
    );
    return data;
}

static inline u32 clear32(u32 addr, u32 clear)
{
    u32 data;
    __asm__ volatile (
        "ldr\t%0, [%1]\n"
        "\tbic\t%0, %2\n"
        "\tstr\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (clear)
    );
    return data;
}


static inline u32 mask32(u32 addr, u32 clear, u32 set)
{
    u32 data;
    __asm__ volatile (
        "ldr\t%0, [%1]\n"
        "\tbic\t%0, %3\n"
        "\torr\t%0, %2\n"
        "\tstr\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set), "l" (clear)
    );
    return data;
}

static inline u16 read16(u32 addr)
{
    u32 data;
    __asm__ volatile ("ldrh\t%0, [%1]" : "=l" (data) : "l" (addr));
    return data;
}

static inline void write16(u32 addr, u16 data)
{
    __asm__ volatile ("strh\t%0, [%1]" : : "l" (data), "l" (addr));
}

static inline u16 set16(u32 addr, u16 set)
{
    u16 data;
    __asm__ volatile (
        "ldrh\t%0, [%1]\n"
        "\torr\t%0, %2\n"
        "\tstrh\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set)

    );
    return data;
}

static inline u16 clear16(u32 addr, u16 clear)
{
    u16 data;
    __asm__ volatile (
        "ldrh\t%0, [%1]\n"
        "\tbic\t%0, %2\n"
        "\tstrh\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (clear)
    );
    return data;
}


static inline u16 mask16(u32 addr, u16 clear, u16 set)
{
    u16 data;
    __asm__ volatile (
        "ldrh\t%0, [%1]\n"
        "\tbic\t%0, %3\n"
        "\torr\t%0, %2\n"
        "\tstrh\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set), "l" (clear)
    );
    return data;
}

static inline u8 read8(u32 addr)
{
    u32 data;
    __asm__ volatile ("ldrb\t%0, [%1]" : "=l" (data) : "l" (addr));
    return data;
}

static inline void write8(u32 addr, u8 data)
{
    __asm__ volatile ("strb\t%0, [%1]" : : "l" (data), "l" (addr));
}

static inline u8 set8(u32 addr, u8 set)
{
    u8 data;
    __asm__ volatile (
        "ldrb\t%0, [%1]\n"
        "\torr\t%0, %2\n"
        "\tstrb\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set)
    );
    return data;
}

static inline u8 clear8(u32 addr, u8 clear)
{
    u8 data;
    __asm__ volatile (
        "ldrb\t%0, [%1]\n"
        "\tbic\t%0, %2\n"
        "\tstrb\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (clear)
    );
    return data;
}

static inline u8 mask8(u32 addr, u8 clear, u8 set)
{
    u8 data;
    __asm__ volatile (
        "ldrb\t%0, [%1]\n"
        "\tbic\t%0, %3\n"
        "\torr\t%0, %2\n"
        "\tstrb\t%0, [%1]"
        : "=&l" (data)
        : "l" (addr), "l" (set), "l" (clear)
    );
    return data;
}

LINKABLE void udelay(u32 d);
LINKABLE void debug_printf(const char *format, ...);

#define PTR_OFFS(a, b) ((void*)(((intptr_t)a) + b))

#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })

#define min(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })

#define BIT(n) (1<<n)

#endif // UTILS_H