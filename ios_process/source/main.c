#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "ios/svc.h"
#include "dynamic.h"
#include "ios/memory.h"
#include "latte/cache.h"
#include "wupserver/wupserver.h"
#include "utils.h"

volatile int bss_var;
volatile int data_var = 0x12345678;
static int kern_done = 0;

u32 main_thread(void*);
extern void kern_entry();

#define ASM_PATCH(_addr, _str) { \
    __asm__ volatile (           \
    ".globl pre_" #_addr "\n"    \
    ".globl post_" #_addr "\n"   \
    "b post_" #_addr "\n"        \
    "pre_" #_addr ":\n"          \
    ".arm\n"                     \
    _str "\n"                    \
     ".arm\n"                    \
    "post_" #_addr ":\n");       \
    extern void pre_##_addr();   \
    extern void post_##_addr();  \
    memcpy((void*)_addr, pre_##_addr, (u32)post_##_addr - (u32)pre_##_addr); \
    dc_flushrange((void*)_addr, (u32)post_##_addr - (u32)pre_##_addr); \
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP, which launches before MCP)
// It must return.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here kern %p\n", kern_main);
    bss_var = 0x12345678;
    data_var = 0;

    // BSP
    {
        // Disk drive disable
        ASM_PATCH(0xE60085F8, "mov r3, #2");
        ASM_PATCH(0xE6008BEC, "mov r3, #2");

        // nop a function used for seeprom write enable, disable, nuking (will stay in write disable)
        ASM_PATCH(0xE600CF5C, 
            "mov r0, #0\n \
             bx lr\n"
        );

        // skip seeprom writes in eepromDrvWriteWord for safety
        ASM_PATCH(0xE600D010, 
            "mov r0, #0\n \
             bx lr\n"
        );
    }
    
    // TEST
    {
        // Tell TEST we're debug
        //ASM_PATCH(0xE4016A78, "mov r0, #0");

        // enable cafeos event log (CEL)
        //ASM_PATCH(0xE40079E4, "nop");

        // fixup shutdown
        //ASM_PATCH(0xE4007904, "mov r0, #0xA");

        // enable system profiler
        //ASM_PATCH(0xE4006648, "mov r0, #0xA");
    }
    
    // ACP
    {
        // Enable all logging
        // ASM_PATCH(0xE00D6DE8, "TODO"); // b ACP_SYSLOG_OUTPUT

        // skip SHA1 hash checks (used for bootMovie, bootLogoTex)
        ASM_PATCH(0xE0030BC0, "mov r0, #0\nbx lr"); // b ACP_SYSLOG_OUTPUT

        // allow any region title to be launched (results may vary!)
        ASM_PATCH(0xE0030474, "mov r0, #0\nbx lr"); // b ACP_SYSLOG_OUTPUT
    }

    // CRYPTO
    {
        //TODO
    }

    // MCP
    {
        //TODO
    }

    // Make sure bss and such doesn't get initted again.
    ASM_PATCH(kern_entry, "bx lr");

    ic_invalidateall();
    debug_printf("done %x\n", *(u32*)0xE600D010);
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