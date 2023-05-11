#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "ios/svc.h"
#include "ios/memory.h"
#include "latte/cache.h"
#include "latte/irq.h"
#include "latte/serial.h"
#include "wupserver/wupserver.h"
#include "dynamic.h"
#include "utils.h"

#define NEW_TIMEOUT (0xFFFFFFFF)

volatile int bss_var;
volatile int data_var = 0x12345678;
static int kern_done = 0;

u32 main_thread(void*);
extern void kern_entry();
extern void mcp_entry();
extern void svc_handler_hook();
extern void* old_svc_handler;
extern void ancast_crypt_check();
extern void launch_os_hook();
const char* fw_img_path = "/vol/sdcard";

#define _U32_PATCH(_addr, _val, _copy_fn) { \
    _copy_fn((uintptr_t)_addr, (u32)_val); \
}
#define _ASM_PATCH(_addr, _str, _copy_fn) { \
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
    _copy_fn((uintptr_t)_addr, (uintptr_t)pre_##_addr, (u32)post_##_addr - (u32)pre_##_addr); \
}
#define _BL_TRAMPOLINE(_addr, _dst, _copy_fn) { \
    u32 bl_offs = (((u32)_dst - (u32)_addr) - 8) / 4; \
    u32 bl_insn = 0xEB000000 | (bl_offs & 0xFFFFFF); \
    _U32_PATCH(_addr, bl_insn, _copy_fn); \
}
#define _BL_T_TRAMPOLINE(_addr, _dst, _copy_fn) { \
    u32 bl_offs = (((u32)_dst - (u32)_addr) - 4) / 2; \
    u32 bl_insn = 0xF000F800 | (bl_offs & 0x7FF) | (((bl_offs >> 11) & 0x3FF) << 16); \
    _U32_PATCH(_addr, bl_insn, _copy_fn); \
    debug_printf("%08x %08x %08x %08x\n", _addr, _dst, bl_offs, bl_insn); \
}

//
// Pre-MMU -- Lookups done manually
//
static void _patch_u32_k(uintptr_t addr, u32 val)
{
    memcpy((void*)ios_elf_vaddr_to_paddr(addr), &val, sizeof(u32));
}

static void _patch_copy_k(uintptr_t dst, uintptr_t src, size_t sz)
{
    memcpy((void*)ios_elf_vaddr_to_paddr(dst), (void*)src, sz);
}

#define U32_PATCH_K(_addr, _val) _U32_PATCH(_addr, _val, _patch_u32_k)
#define ASM_PATCH_K(_addr, _str) _ASM_PATCH(_addr, _str, _patch_copy_k)
#define BL_TRAMPOLINE_K(_addr, _dst) _BL_TRAMPOLINE(_addr, _dst, _patch_u32_k)
#define BL_T_TRAMPOLINE_K(_addr, _dst) _BL_T_TRAMPOLINE(_addr, _dst, _patch_u32_k)

//
// Post-MMU
//
static void _patch_u32(uintptr_t addr, u32 val)
{
    memcpy((void*)addr, &val, sizeof(u32));
    dc_flushrange((void*)addr, sizeof(u32));
}

static void _patch_copy(uintptr_t dst, uintptr_t src, size_t sz)
{
    memcpy((void*)dst, (void*)src, sz);
    dc_flushrange((void*)dst, sizeof(u32));
}
#define U32_PATCH(_addr, _val) _U32_PATCH(_addr, _val, _patch_u32)
#define ASM_PATCH(_addr, _str) _ASM_PATCH(_addr, _str, _patch_copy)
#define BL_TRAMPOLINE(_addr, _dst) _BL_TRAMPOLINE(_addr, _dst, _patch_u32)
#define BL_T_TRAMPOLINE(_addr, _dst) _BL_T_TRAMPOLINE(_addr, _dst, _patch_u32)

extern u32 otp_store[0x400/4];

int otp_read_replace(int which_word, void *pOut, unsigned int len)
{
    u32 cookie = irq_kill();
    memcpy(pOut, &otp_store[which_word], len);
    irq_restore(cookie);
    return 0;
}

void abt_replace(u32* stack, int which)
{
    for (int i = 0; i < 10; i++)
    {
        debug_printf("r%u  = %08X \n", i, stack[i+1]);
    }

    for (int i = 10; i < 16; i++)
    {
        debug_printf("r%u = %08X \n", i, stack[i+1]);
    }

    debug_printf("%08X\n", *(u32*)(stack[16] + ((which == 1) ? -8 : 0)));

    // TODO: write RTC
    // TODO: write screen?
}

void iabt_replace(u32* stack)
{
    debug_printf("GURU MEDITATION ERROR (prefetch abort)\n");
    abt_replace(stack, 0);
}

void dabt_replace(u32* stack)
{
    debug_printf("GURU MEDITATION ERROR (data abort)\n");
    abt_replace(stack, 1);
}

void undef_abt_replace(u32* stack)
{
    debug_printf("GURU MEDITATION ERROR (undefined instruction)\n");
    abt_replace(stack, 2);
}

void ios_panic_replace(const char* str)
{
    debug_printf("%s\n", str);

    // TODO: write RTC
    // TODO: write screen?
    while (1);
}

void semihosting_write0(const char* str)
{
    serial_send_str(str);
}

void semihosting_handler(int which, void* arg0)
{
    if (which == 4) // write0
    {
        semihosting_write0((const char*)arg0);
    }
}

void init_heap()
{
    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;
    extern void __heap_start();
    extern void __heap_end();

    fake_heap_start = (char*)__heap_start;
    fake_heap_end   = (char*)__heap_end;
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here kern %p\n", kern_main);
    bss_var = 0x12345678;
    data_var = 0;

    init_heap();

    // TODO verify the bytes that are overwritten?
    // and/or search for instructions where critical (OTP)
    // TODO don't hardcode ramdisk carveout size

    // KERNEL
    {
        // patch out restrictions on syscall 0x22 (otp_read)
        ASM_PATCH_K(0x08120374, "mov r12, #0x3");
        ASM_PATCH_K(0x081203C4, "nop");

        // patch OTP to read from memory
        ASM_PATCH_K(0x08120248, 
            "ldr r3, _otp_read_hook\n"
            "bx r3\n"
            "_otp_read_hook: .word otp_read_replace");

        // skip 49mb memclear for faster loading.
        ASM_PATCH_K(0x812A088, 
            "mov r1, r5\n"
            "mov r2, r6\n"
            "mov r3, r7\n"
            "bx r4\n");

        // insn abort
        ASM_PATCH_K(0x0812A134, 
            "ldr r3, _iabt_hook\n"
            "bx r3\n"
            "_iabt_hook: .word iabt_replace");

        // data abort
        ASM_PATCH_K(0x0812A1AC, 
            "ldr r3, _dabt_hook\n"
            "bx r3\n"
            "_dabt_hook: .word dabt_replace");

        // undef insn abort
        ASM_PATCH_K(0x08129E50, 
            "ldr r3, _undef_hook\n"
            "bx r3\n"
            "_undef_hook: .word undef_abt_replace");

        // IOS panic hook
        ASM_PATCH_K(0x8129AA0, //0x8129AA0
            "ldr r3, _panic_hook\n"
            "bx r3\n"
            "_panic_hook: .word ios_panic_replace");

        // Hook SVC handler for semihosting
        old_svc_handler = (void*)(*(u32*)0xFFFF0028);
        *(u32*)0xFFFF0028 = (u32)svc_handler_hook;

        // Early MMU mapping
        // (For some reason they map 0x28000000 twice, so it's really easy for us)
        ASM_PATCH_K(0x08122768,
            "mov r2, #0x100000\n"
            "sub r1, r0, r2\n"
            "mov r0, r1\n"
        );
    }

    // BSP
    {
        // Disk drive disable
        ASM_PATCH_K(0xE60085F8, "mov r3, #2");
        ASM_PATCH_K(0xE6008BEC, "mov r3, #2");

        // nop a function used for seeprom write enable, disable, nuking (will stay in write disable)
        ASM_PATCH_K(0xE600CF5C, 
            "mov r0, #0\n \
             bx lr\n"
        );

        // skip seeprom writes in eepromDrvWriteWord for safety
        ASM_PATCH_K(0xE600D010, 
            "mov r0, #0\n \
             bx lr\n"
        );
    }

    // MCP
    {
        // MCP main thread hook
        BL_TRAMPOLINE_K(0x05056718, MCP_ALTBASE_ADDR(mcp_entry));

        // hook to allow decrypted ancast images
        BL_T_TRAMPOLINE_K(0x0500A678, MCP_ALTBASE_ADDR(ancast_crypt_check));

        // launch OS hook (fw.img -> sdcard)
        BL_T_TRAMPOLINE_K(0x050282AE, MCP_ALTBASE_ADDR(launch_os_hook));

        // patch pointer to fw.img loader path
        U32_PATCH_K(0x050284D8, fw_img_path);


        // hooks to allow custom PPC kernel.img
        //BL_T_TRAMPOLINE_K(0x050340A0, MCP_ALTBASE_ADDR(ppc_hook_pre));
        //BL_T_TRAMPOLINE_K(0x05034118, MCP_ALTBASE_ADDR(ppc_hook_post));

        // fix 10 minute timeout that crashes MCP after 10 minutes of booting
        U32_PATCH_K(0x05022474, NEW_TIMEOUT);

        // disable loading cached title list from os-launch memory
        ASM_PATCH_K(0x0502E59A, 
            ".thumb\n"
            "mov r0, #0x0\n"
        );


        // patch OS launch sig check
        ASM_PATCH_K(0x0500A818,
            ".thumb\n"
            "mov r0, #0\n"
            "mov r0, #0\n"
        );

        // patch IOSC_VerifyPubkeySign to always succeed
        ASM_PATCH_K(0x05052C44,
            "mov r0, #0\n"
            "bx lr\n"
        );

        // nop "COS encountered unrecoverable error..." IOS panic
        /*ASM_PATCH_K(0x05056B84,
            "nop\n"
        );*/

        // allow MCP_GetSystemLog on retail
        ASM_PATCH_K(0x05025EE6,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_SetDefaultTitleId
        ASM_PATCH_K(0x05026BA8,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_GetDefaultTitleId
        ASM_PATCH_K(0x05026C54,
            ".thumb\n"
            "mov r0, #0\n"
            "nop\n"
        );

        // same, MCP_GetTitleSize
        ASM_PATCH_K(0x502574E,
            ".thumb\n"
            "nop\n"
            "nop\n"
        );

        // patch cert verification
        ASM_PATCH_K(0x05052A90,
            "mov r0, #0\n"
            "bx lr\n"
        );

        // allow usage of u16 at ancast header + 0x1A0
        ASM_PATCH_K(0x0500A7CA,
            ".thumb\n"
            "nop\n"
        );

        // skip content hash verification
        ASM_PATCH_K(0x05014CAC,
            ".thumb\n"
            "mov r0, #0\n"
            "bx lr\n"
        );

        // Shorten the RAMdisk area by 1MiB
        U32_PATCH_K(0x0502365C, (*(u32*)ios_elf_vaddr_to_paddr(0x0502365C)) - 0x100000);
    }
    
    // ACP
    {
        // Enable all logging
        // ASM_PATCH_K(0xE00D6DE8, "TODO"); // b ACP_SYSLOG_OUTPUT

        // skip SHA1 hash checks (used for bootMovie, bootLogoTex)
        ASM_PATCH_K(0xE0030BC0, "mov r0, #0\nbx lr");

        // allow any region title to be launched (results may vary!)
        ASM_PATCH_K(0xE0030474, "mov r0, #0\nbx lr");
    }

    // TEST
    {
        // Tell TEST we're debug
        //ASM_PATCH_K(0xE4016A78, "mov r0, #0");

        // enable cafeos event log (CEL)
        //ASM_PATCH_K(0xE40079E4, "nop");

        // fixup shutdown
        //ASM_PATCH_K(0xE4007904, "mov r0, #0xA");

        // enable system profiler
        //ASM_PATCH_K(0xE4006648, "mov r0, #0xA");
    }

    // CRYPTO
    {
        //TODO
    }

    // Make sure bss and such doesn't get initted again.
    ASM_PATCH_K(kern_entry, "bx lr");

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

    //const char* test_panic = "This is a test panic\n";
    //iosPanic(test_panic, strlen(test_panic)+1);

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