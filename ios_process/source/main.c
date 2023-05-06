#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "ios/svc.h"
#include "dynamic.h"
#include "ios/memory.h"
#include "wupserver/wupserver.h"
#include "utils.h"
#include "cache.h"

volatile int bss_var;
volatile int data_var = 0x12345678;
static int kern_done = 0;

u32 main_thread(void*);

#define ASM_PATCH(_addr, _str) { \
    __asm__ volatile (           \
    ".globl pre_" #_addr "\n"    \
    ".globl post_" #_addr "\n"   \
    "b post_" #_addr "\n"        \
    "pre_" #_addr ":\n"          \
    _str "\n"                    \
    "post_" #_addr ":\n"         \
    "adr r0, pre_" #_addr "\n"   \
    "ldr r1, =0x" #_addr "\n"    \
    "adr r2, post_" #_addr "\n"  \
    "sub r2, r2, r0\n"           \
    "bl memcpy\n");              \
    extern void pre_##_addr();   \
    extern void post_##_addr();  \
    dc_flushrange((void*)0x##_addr, (u32)post_##_addr - (u32)pre_##_addr); \
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP, which launches before MCP)
// It must return.
void kern_main()
{
    // They call the function I hook twice for whatever reason.
    if (kern_done) return;

    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here kern %p\n", kern_main);
    bss_var = 0x12345678;
    data_var = 0;

    // Disk drive disable
    ASM_PATCH(E60085F8, "mov r3, #2");
    ASM_PATCH(E6008BEC, "mov r3, #2");

    // nop a function used for seeprom write enable, disable, nuking (will stay in write disable)
    ASM_PATCH(E600CF5C, 
        "mov r0, #0\n \
         bx lr\n"
    );

    // skip seeprom writes in eepromDrvWriteWord for safety
    ASM_PATCH(E600D010, 
        "mov r0, #0\n \
         bx lr\n"
    );

    ic_invalidateall();
    debug_printf("done %x\n", *(u32*)0xE600D010);
    kern_done = 1;
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{
    // Make sure relocs worked fine and mappings are good
	debug_printf("we in here MCP %p\n", mcp_main);
    bss_var = 0x87654321;
    data_var = 0x12345678;

    debug_printf("done\n");

    // TODO bootstrap other thread?

    // Start up iosuhax service
    u8* main_stack = (u8*)malloc_global(0x1000);
    int main_threadhand = svcCreateThread(main_thread, NULL, (u32*)(main_stack+0x1000), 0x1000, 0x78, 1);
    if (main_threadhand >= 0) {
        svcStartThread(main_threadhand);
    }

    // Start up wupserver
    u8* wupserver_stack = (u8*)malloc_global(0x1000);
    int wupserver_threadhand = svcCreateThread(wupserver_main, NULL, (u32*)(wupserver_stack+0x1000), 0x1000, 0x78, 1);
    if (wupserver_threadhand >= 0) {
        svcStartThread(wupserver_threadhand);
    }
}

// TODO: make a service
u32 main_thread(void* arg)
{
    while (1) {
        usleep(10*1000*1000);
        debug_printf("alive\n");
    }
}