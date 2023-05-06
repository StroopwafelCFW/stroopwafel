#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "ios/svc.h"
#include "ios/memory.h"
#include "latte/cache.h"
#include "latte/irq.h"
#include "wupserver/wupserver.h"
#include "dynamic.h"
#include "utils.h"

#define NEW_TIMEOUT (0xFFFFFFFF)

volatile int bss_var;
volatile int data_var = 0x12345678;
static int kern_done = 0;

u32 main_thread(void*);
extern void kern_entry();

#define U32_PATCH(_addr, _val) { \
    *(u32*)(_addr) = _val; \
    dc_flushrange((void*)_addr, sizeof(u32)); \
}


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

extern u32 otp_store[0x400/4];

int otp_read_replace(int which_word, void *pOut, unsigned int len)
{
    u32 cookie = irq_kill();
    memcpy(pOut, &otp_store[which_word], len);
    irq_restore(cookie);
    return 0;
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

    // KERNEL
    {
        // patch out restrictions on syscall 0x22 (otp_read)
        ASM_PATCH(0x08120374, "mov r12, #0x3");
        ASM_PATCH(0x081203C4, "nop");

        // patch OTP to read from memory
        ASM_PATCH(0x08120248, 
            "ldr r3, _otp_read_hook\n"
            "bx r3\n"
            "_otp_read_hook: .word otp_read_replace");

        // skip 49mb memclear for faster loading.
        ASM_PATCH(0x812A088, 
            "mov r1, r5\n"
            "mov r2, r6\n"
            "mov r3, r7\n"
            "bx r4\n");
    }

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

    // MCP
    {
        // fix 10 minute timeout that crashes MCP after 10 minutes of booting
        U32_PATCH(0x05022474, NEW_TIMEOUT);

        // disable loading cached title list from os-launch memory
        ASM_PATCH(0x0502E59A, 
            ".thumb\n"
            "mov r0, #0x0\n"
        );


        // patch OS launch sig check
        ASM_PATCH(0x0500A818,
            ".thumb\n"
            "mov r0, #0\n"
            "mov r0, #0\n"
        );

        // patch IOSC_VerifyPubkeySign to always succeed
        ASM_PATCH(0x05052C44,
            "mov r0, #0\n"
            "bx lr\n"
        );

        // nop "COS encountered unrecoverable error..." IOS panic
        ASM_PATCH(0x05056B84,
            "nop\n"
        );

        // allow MCP_GetSystemLog on retail
        ASM_PATCH(0x05025EE6,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_SetDefaultTitleId
        ASM_PATCH(0x05026BA8,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_GetDefaultTitleId
        ASM_PATCH(0x05026C54,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_GetTitleSize
        ASM_PATCH(0x502574E,
            ".thumb\n"
            "nop\n"
            "nop\n"
        );

        // patch cert verification
        ASM_PATCH(0x05052A90,
            "mov r0, #0\n"
            "bx lr\n"
        );

        // allow usage of u16 at ancast header + 0x1A0
        ASM_PATCH(0x0500A7CA,
            ".thumb\n"
            "nop\n"
        );

        // skip content hash verification
        ASM_PATCH(0x05014CAC,
            ".thumb\n"
            "mov r0, #0\n"
            "bx lr\n"
        );

        // Shorten the RAMdisk area by 1MiB
        U32_PATCH(0x0502365C, (*(u32*)0x0502365C) - 0x100000);
    }
    
    // ACP
    {
        // Enable all logging
        // ASM_PATCH(0xE00D6DE8, "TODO"); // b ACP_SYSLOG_OUTPUT

        // skip SHA1 hash checks (used for bootMovie, bootLogoTex)
        ASM_PATCH(0xE0030BC0, "mov r0, #0\nbx lr");

        // allow any region title to be launched (results may vary!)
        ASM_PATCH(0xE0030474, "mov r0, #0\nbx lr");
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

    // CRYPTO
    {
        //TODO
    }

    // Make sure bss and such doesn't get initted again.
    ASM_PATCH(kern_entry, "bx lr");

    ic_invalidateall();
    debug_printf("done\n");
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