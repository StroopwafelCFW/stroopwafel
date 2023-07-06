#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "config.h"
#include "bm.h"
#include "imports.h"
#include "ios/svc.h"
#include "ios/memory.h"
#include "latte/cache.h"
#include "latte/irq.h"
#include "latte/serial.h"
#include "wupserver/wupserver.h"
#include "dynamic.h"
#include "ios_dynamic.h"
#include "utils.h"
#include "addrs_55x.h"

#define NEW_TIMEOUT (0xFFFFFFFF)

u32 main_thread(void*);
extern void kern_entry();
extern void mcp_entry();
extern void svc_handler_hook();
extern void* old_svc_handler;
extern int otp_read_replace_hacky(int which_word, u32 *pOut, unsigned int len);
extern void otp_read_replace_hacky_end();
extern void ancast_crypt_check();
extern void launch_os_hook();
extern void syslog_hook();
extern void hai_shift_data_offsets_t();
extern void boot_dirs_clear_t();
extern void crypto_keychange_hook();
extern void crypto_disable_hook();
extern void c2w_seeprom_hook_t();
extern void c2w_boot_hook_t();
extern void c2w_otp_replacement_t();
extern void c2w_otp_replacement_t_end();

extern void wfs_crypto_hook();
extern void createDevThread_hook();
extern void scfm_try_slc_cache_migration();
extern void usbRead_patch();
extern void usbWrite_patch();
extern void usb_sector_spoof();
extern void opendir_hook();
extern void fsaopen_fullstr_dump_hook();
extern void slcRead1_patch();
extern void slcRead2_patch();
extern void slcWrite1_patch();
extern void slcWrite2_patch();
extern void sdcardRead_patch();
extern void sdcardWrite_patch();
extern void getMdDeviceById_hook();
extern void registerMdDevice_hook();

const char* fw_img_path = "/vol/sdcard";

static int is_55x = 0;

const u32 mcpIoMappings_patch[6*3] = 
{
    // mapping 1
    0x0D000000, // vaddr
    0x0D000000, // paddr
    0x001C0000, // size
    0x00000000, // ?
    0x00000003, // ?
    0x00000000, // ?
    
    // mapping 2
    0x0D800000, // vaddr
    0x0D800000, // paddr
    0x001C0000, // size
    0x00000000, // ?
    0x00000003, // ?
    0x00000000, // ?

    // mapping 3
    0x0C200000, // vaddr
    0x0C200000, // paddr
    0x00100000, // size
    0x00000000, // ?
    0x00000003, // ?
    0x00000000, // ?
};

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
    u32 bl_offs = (((u32)_dst - (u32)(_addr)) - 8) / 4; \
    u32 bl_insn = 0xEB000000 | (bl_offs & 0xFFFFFF); \
    _U32_PATCH(_addr, bl_insn, _copy_fn); \
}
#define _BL_T_TRAMPOLINE(_addr, _dst, _copy_fn) { \
    u32 bl_offs = (((u32)_dst - (u32)(_addr)) - 4) / 2; \
    u32 bl_insn = 0xF000F800 | (bl_offs & 0x7FF) | (((bl_offs >> 11) & 0x3FF) << 16); \
    _U32_PATCH(_addr, bl_insn, _copy_fn); \
}
#define _BRANCH_PATCH(_addr, _dst, _copy_fn) { \
    u32 bl_offs = (((u32)_dst - (u32)(_addr)) - 8) / 4; \
    u32 bl_insn = 0xEA000000 | (bl_offs & 0xFFFFFF); \
    _U32_PATCH(_addr, bl_insn, _copy_fn); \
}

#define ARRARG(...) {__VA_ARGS__}
#define _SEARCH_PATCH(_search, _start_addr, _next_macro, ...) {                                            \
    u8 search[] = ARRARG _search;                                                                          \
    uintptr_t _addr = (uintptr_t)boyer_moore_search((void*)_start_addr, 0x200000, search, sizeof(search)); \
    if (!_addr) {                                                                                          \
        debug_printf("Failed to find search pattern!\n");                                                  \
        for (int i = 0; i < sizeof(search); i++) {                                                         \
            debug_printf("%02x ", search[i]);                                                              \
        }                                                                                                  \
        debug_printf("\n");                                                                                \
        while(1);                                                                                          \
    }                                                                                                      \
    _next_macro(_addr, __VA_ARGS__);                                                                       \
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
#define BRANCH_PATCH_K(_addr, _dst) _BRANCH_PATCH(_addr, _dst, _patch_u32_k)

#define MEMCPY_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, MEMCPY_K, __VA_ARGS__)
#define U32_PATCH_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, U32_PATCH_K, __VA_ARGS__)
#define ASM_PATCH_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, ASM_PATCH_K, __VA_ARGS__)
#define BL_TRAMPOLINE_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BL_TRAMPOLINE_K, __VA_ARGS__)
#define BL_T_TRAMPOLINE_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BL_T_TRAMPOLINE_K, __VA_ARGS__)
#define BRANCH_PATCH_SK(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BRANCH_PATCH_K, __VA_ARGS__)


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
#define MEMCPY_PATCH(_addr, _dst, _sz) _patch_copy(_addr, _dst, _sz)
#define U32_PATCH(_addr, _val) _U32_PATCH(_addr, _val, _patch_u32)
#define ASM_PATCH(_addr, _str) _ASM_PATCH(_addr, _str, _patch_copy)
#define BL_TRAMPOLINE(_addr, _dst) _BL_TRAMPOLINE(_addr, _dst, _patch_u32)
#define BL_T_TRAMPOLINE(_addr, _dst) _BL_T_TRAMPOLINE(_addr, _dst, _patch_u32)
#define BRANCH_PATCH(_addr, _dst) _BRANCH_PATCH(_addr, _dst, _patch_u32)

#define MEMCPY_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, MEMCPY, __VA_ARGS__)
#define U32_PATCH_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, U32_PATCH, __VA_ARGS__)
#define ASM_PATCH_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, ASM_PATCH, __VA_ARGS__)
#define BL_TRAMPOLINE_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BL_TRAMPOLINE, __VA_ARGS__)
#define BL_T_TRAMPOLINE_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BL_T_TRAMPOLINE, __VA_ARGS__)
#define BRANCH_PATCH_S(_search, _start_addr, ...) _SEARCH_PATCH(_search, _start_addr, BRANCH_PATCH, __VA_ARGS__)


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
    while (1);
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

// Replace all instances of 0xFFF07F00 (Wii SEEPROM addr) with 0xFFF07B00 in c2w.elf
void c2w_boot_hook_fixup_c2w_ptrs()
{
    for (uintptr_t a = 0x01000000; a < 0x01F80000; a += 4)
    {
        if (read32(a) == 0xFFF07F00) {
            write32(a, 0xFFF07B00);
            debug_printf("SEEPROM ptr at: 0x%08x\n", a);
            break;
        }
    }
}

void c2w_boot_hook_find_and_replace_otpread()
{
    uintptr_t ios_paddr = read32(0x1018);
    uintptr_t ios_end = ios_paddr + 0x260000;

    for (uintptr_t a = ios_paddr; a < ios_end; a += 4)
    {
        if (read32(a) == 0x0D8001EC) {
            for (uintptr_t b = a; b >= a-0x1000; b -= 2)
            {
                if (read16(b) == 0xB5F0) // PUSH {R4-R7,LR}
                {
                    memcpy((void*)b, (void*)c2w_otp_replacement_t, (u32)c2w_otp_replacement_t_end-(u32)c2w_otp_replacement_t);
                    debug_printf("OTP read at: 0x%08x\n", b);
                    break;
                }
            }
        }
    }
}

// Search for 0xFFFE7CC0 in IOS and change it to 0xFFFE7B00
void c2w_boot_hook_fixup_ios_ptrs()
{
    uintptr_t ios_paddr = read32(0x1018);
    uintptr_t ios_end = ios_paddr + 0x260000;

    for (uintptr_t a = ios_paddr; a < ios_end; a += 4)
    {
        if (read32(a) == 0xFFFE7CC0) {
            write32(a, 0xFFFE7B00);
            debug_printf("IOS SEEPROM ptr at: 0x%08x\n", a);
            break;
        }
    }
}

// Search for E3C22020 E3822020 and replace the second instruction with a nop
void c2w_oc_hax_patch()
{
    for (uintptr_t a = 0x01000000; a < 0x01F80000; a += 4)
    {
        if (read32(a) == 0xE3C22020 && read32(a+4) == 0xE3822020) {
            write32(a+4, 0);
            debug_printf("Overclock ptr at: 0x%08x\n", a+4);
            break;
        }
    }
}

void c2w_patches()
{
    c2w_boot_hook_fixup_c2w_ptrs();
    c2w_boot_hook_find_and_replace_otpread();
    c2w_boot_hook_fixup_ios_ptrs();

#if VWII_OVERCLOCK
    c2w_oc_hax_patch();
#endif
}

u32 (*FS_REGISTER_FS_DRIVER)(u8* opaque) = (void*)0x10732D70;
const char* (*FS_DEVTYPE_TO_NAME)(int a) = (void*)0x107111A8;

u32 fat_register_hook(u8* opaque)
{
    opaque[0xc] = 0x6; // sdcard
    opaque[0xd] = 0x11; // usb
    opaque[0xe] = 0x0; // terminating 0

    return FS_REGISTER_FS_DRIVER(opaque);
}

const char* fsa_dev_register_hook(int a)
{
    register u8* hax __asm__("r4");
    if (a == 0x11)
    {
        u8* opaque = (u8*)read32((u32)(hax+0x5C));
        opaque += 8;
        if (!memcmp(opaque, "fat", 4)) {
            return "ext";
        }
    }
    return FS_DEVTYPE_TO_NAME(a);
}

void init_phdrs()
{
    // Init memory mappings
    Elf32_Phdr* phdr_base = ios_elf_add_phdr(wafel_plugin_base_addr);

    size_t code_trampoline_sz = 0x100000; // TODO?

    phdr_base->p_type = 1;   // LOAD
    phdr_base->p_offset = 0; // doesn't matter
    phdr_base->p_vaddr = wafel_plugin_base_addr;
    phdr_base->p_paddr = wafel_plugin_base_addr; // ramdisk is consistent so we can do this.
    phdr_base->p_filesz = CARVEOUT_SZ;
    phdr_base->p_memsz = CARVEOUT_SZ;
    phdr_base->p_flags = IOS_PHDR_FLAGS(IOS_RWX, IOS_MCP); // RWX, MCP
    phdr_base->p_align = 1;
    phdr_base[-1].p_memsz -= CARVEOUT_SZ;

    Elf32_Phdr* phdr_mcp = ios_elf_add_phdr(wafel_plugin_base_addr);

    // Add mirror at 0x05800000 for MCP trampolines
    phdr_mcp->p_type = 1;   // LOAD
    phdr_mcp->p_offset = 0; // doesn't matter
    phdr_mcp->p_vaddr = MCP_ALT_BASE;
    phdr_mcp->p_paddr = wafel_plugin_base_addr; // ramdisk is consistent so we can do this.
    phdr_mcp->p_filesz = 0;
    phdr_mcp->p_memsz = code_trampoline_sz;
    phdr_mcp->p_flags = IOS_PHDR_FLAGS(IOS_RWX, IOS_MCP);
    phdr_mcp->p_align = 1;

    Elf32_Phdr* phdr_fs = ios_elf_add_phdr(wafel_plugin_base_addr);

    // Add mirror at 0x10600000 for FS trampolines
    phdr_fs->p_type = 1;   // LOAD
    phdr_fs->p_offset = 0; // doesn't matter
    phdr_fs->p_vaddr = FS_ALT_BASE;
    phdr_fs->p_paddr = wafel_plugin_base_addr; // ramdisk is consistent so we can do this.
    phdr_fs->p_filesz = 0;
    phdr_fs->p_memsz = code_trampoline_sz;
    phdr_fs->p_flags = IOS_PHDR_FLAGS(IOS_RWX, IOS_MCP); // RWX, MCP
    phdr_fs->p_align = 1;

    ios_elf_print_map();
}

void init_linking()
{
    wafel_register_plugin(wafel_plugin_base_addr);
    debug_printf("kern_main symbol at: %08x\n", wafel_get_symbol_addr(wafel_plugin_base_addr, "kern_main"));
    debug_printf("our module mem size is: %08x\n", wafel_plugin_max_addr(wafel_plugin_base_addr)-wafel_plugin_base_addr);
    
    uintptr_t next_plugin = wafel_plugin_base_addr;
    while (next_plugin)
    {
        next_plugin = wafel_plugin_next(next_plugin);
        
        if (next_plugin) {
            debug_printf("registering plugin: %08x\n", next_plugin);
            wafel_register_plugin(next_plugin);
        }
    }

    next_plugin = wafel_plugin_base_addr;
    while (next_plugin)
    {
        next_plugin = wafel_plugin_next(next_plugin);
        
        if (next_plugin) {
            debug_printf("linking plugin: %08x\n", next_plugin);
            wafel_link_plugin(next_plugin);
        }
    }
}

void init_plugins()
{
    uintptr_t next_plugin = wafel_plugin_base_addr;
    while (next_plugin)
    {
        next_plugin = wafel_plugin_next(next_plugin);
        
        if (next_plugin) {
            debug_printf("initializing plugin: %08x\n", next_plugin);
            uintptr_t km = wafel_get_symbol_addr(next_plugin, "kern_entry");
            if (!km) continue;

            void (*plug_kern_entry)() = (void*)km;
            plug_kern_entry();
        }
    }
}

void patch_general()
{
    // KERNEL
    {
#if OTP_IN_MEM
        u8 otp_pattern[8] = {0xE5, 0x85, 0x31, 0xEC, 0xE0, 0x84, 0x00, 0x07};
        uintptr_t otp_read_addr = (uintptr_t)boyer_moore_search((void*)0x08100000, 0x100000, otp_pattern, sizeof(otp_pattern));

        debug_printf("wafel_core: found OTP read pattern 1 at %08x...\n", otp_read_addr);
        u32* search_2 = (u32*)otp_read_addr;
        for (int i = 0; i < 0x100; i++) {
            if (*search_2 == 0xE92D47F0) // push {r4-r10, lr}
            {
                break;
            }

            otp_read_addr -= 4;
            search_2--;
        }
        debug_printf("wafel_core: found OTP read at %08x.\n", otp_read_addr);

        // patch OTP to read from memory
#if 0
        ASM_PATCH_K(otp_read_addr, 
            "ldr r3, _otp_read_hook\n"
            "bx r3\n"
            "_otp_read_hook: .word otp_read_replace");
#endif

        // Hacky: copy an ASM stub instead, because our permissions might not be
        // kosher w/o more patches generalized
        memcpy((void*)otp_read_addr, otp_read_replace_hacky, (uintptr_t)otp_read_replace_hacky_end - (uintptr_t)otp_read_replace_hacky);
#endif // OTP_IN_MEM

        // Early MMU mapping
        // (For some reason they map 0x28000000 twice, so it's really easy for us)
        ASM_PATCH_SK(
            (0xE1, 0xA0, 0x10, 0x00, 0xE3, 0xA0, 0x23, 0x2A),
            0x08100000,
            "mov r2, #0x400000\n"
            "sub r1, r0, r2\n"
            "mov r0, r1\n"
        );
    }

    // MCP
    {
        // MCP main thread hook
        //BL_TRAMPOLINE_K(ios_elf_get_module_entrypoint(IOS_MCP), MCP_ALTBASE_ADDR(mcp_entry));
    }
    

    // search 
    // then search back 
}

void patch_55x()
{
    // KERNEL
    {
        // take down the hardware memory protection (XN) driver just as it's finished initializing
        //BL_TRAMPOLINE_K(0x08122490, 0x08122374); // __iosMemMapDisableProtection

        //
        U32_PATCH_K(0x08140DE0, (u32)mcpIoMappings_patch); // ptr to iomapping structs
        U32_PATCH_K(0x08140DE4, 0x00000003); // number of iomappings
        U32_PATCH_K(0x08140DE8, IOS_MCP); // pid

        // patch domains
        ASM_PATCH_K(0x081253C4,
            "str r3, [r7,#0x10]\n"
            "str r3, [r7,#0x0C]\n"
            "str r3, [r7,#0x04]\n"
            "str r3, [r7,#0x14]\n"
            "str r3, [r7,#0x08]\n"
            "str r3, [r7,#0x34]\n"
        );

        // ARM MMU Access Permissions patches
        // set AP bits for 1MB pages to r/w for all
        ASM_PATCH_K(0x08124678, "mov r6, #3");
        // set AP bits for section descriptors to r/w for all
        ASM_PATCH_K(0x08125270, "orr r3, #0x650");
        // set AP bits for second-level table entries to be r/w for all
        ASM_PATCH_K(0x08124D88, "mov r12, #3");

        // Don't enable system protection
        ASM_PATCH_K(0x081223FC, "bx lr");

        // patch out restrictions on syscall 0x22 (otp_read)
        ASM_PATCH_K(0x08120374, "mov r12, #0x3");
        ASM_PATCH_K(0x081203C4, "nop");

        // Always pass pointer checks
        //ASM_PATCH_K(0x081249C0, "mov r0, #0x0\nbx lr\n");
        ASM_PATCH_K(0x08124AD8, "mov r0, #0x0\nbx lr\n");

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

#if HOOK_SEMIHOSTING
        // Hook SVC handler for semihosting
        old_svc_handler = (void*)(*(u32*)0xFFFF0028);
        *(u32*)0xFFFF0028 = (u32)svc_handler_hook;
#endif
    }

    // BSP
    {
#if DISABLE_DISK_DRIVE
        // Disk drive disable
        ASM_PATCH_K(0xE60085F8, "mov r3, #2");
        ASM_PATCH_K(0xE6008BEC, "mov r3, #2");
#endif

        // SEEPROM write disable is restricted to redNAND only:
        // - if a non-redNAND system upgrades boot1, the version in SEEPROM will become
        //   mismatched and the system will be boot0-bricked, I think
        // - if a redNAND system upgrades boot1, it will be written to the SD card,
        //   but the SEEPROM version should NOT be synced to the SD card version, because NAND
        //   has not changed
#if USE_REDNAND
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
#endif
    }

#if MCP_PATCHES
    // MCP
    {
        // TODO: move this to general patches after early MMU stuff is generalized
        // MCP main thread hook
        BL_TRAMPOLINE_K(ios_elf_get_module_entrypoint(IOS_MCP), MCP_ALTBASE_ADDR(mcp_entry));

        // some selective logging function : enable all the logging !
        BRANCH_PATCH_K(0x05055438, MCP_SYSLOG_OUTPUT_ADDR);

        // hook to allow decrypted ancast images
        BL_T_TRAMPOLINE_K(0x0500A678, MCP_ALTBASE_ADDR(ancast_crypt_check));

        // launch OS hook (fw.img -> sdcard)
        BL_T_TRAMPOLINE_K(0x050282AE, MCP_ALTBASE_ADDR(launch_os_hook));

        // Nop SHA1 checks on fw.img
        //U32_PATCH_K(0x0500A84C, 0);

        // patch pointer to fw.img loader path
        U32_PATCH_K(0x050284D8, fw_img_path);

#if SYSLOG_SEMIHOSTING_WRITE
        // syslog -> semihosting write
        BL_TRAMPOLINE_K(0x05055328, MCP_ALTBASE_ADDR(syslog_hook));
#endif

#if USB_SHRINKSHIFT
        BL_T_TRAMPOLINE_K(0x050078A0, MCP_ALTBASE_ADDR(hai_shift_data_offsets_t));
#endif

        // hooks to allow custom PPC kernel.img
        //BL_T_TRAMPOLINE_K(0x050340A0, MCP_ALTBASE_ADDR(ppc_hook_pre));
        //BL_T_TRAMPOLINE_K(0x05034118, MCP_ALTBASE_ADDR(ppc_hook_post));

        // hook in a few dirs to delete on mcp startup
        ASM_PATCH_K(0x05016C8C,
            ".thumb\n"
            "push {lr}\n"
        );
        BL_T_TRAMPOLINE_K(0x05016C8E, MCP_ALTBASE_ADDR(boot_dirs_clear_t));
        ASM_PATCH_K(0x05016C92,
            ".thumb\n"
            "mov r0, #0\n"
            "pop {pc}\n"
        );

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

        // remove SHA1/RSA Ancast checks
        //ASM_PATCH_K(0x0500A81C, ".thumb\nmov r4, #0x0\nmov r0, #0x0\nnop\n");
        //ASM_PATCH_K(0x0500A838, ".thumb\nmov r4, #0x0\nmov r0, #0x0\nnop\n");
        //ASM_PATCH_K(0x0500A84A, ".thumb\nmov r4, #0x0\nmov r0, #0x0\nnop\n");
        //ASM_PATCH_K(0x0500A896, ".thumb\nmov r4, #0x0\nmov r0, #0x0\nnop\n");

        // TODO: why are these needed now???
        // remove various Ancast header size checks
        ASM_PATCH_K(0x0500A7C8, ".thumb\nnop\nnop\n");
        ASM_PATCH_K(0x0500A7D0, ".thumb\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
        ASM_PATCH_K(0x0500A7F4, ".thumb\nnop\nnop\nnop\nnop\nnop\nmov r3, #0x2");

        // Force a crash to inspect junk
        //ASM_PATCH_K(0x0500A778, ".thumb\nldr r0, [r0]\nbx pc\n.word 0xFFFFFFFF\n");

        // nop "COS encountered unrecoverable error..." IOS panic
        /*ASM_PATCH_K(0x05034554,
            "nop\n"
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
        U32_PATCH_K(0x0502365C, (*(u32*)ios_elf_vaddr_to_paddr(0x0502365C)) - CARVEOUT_SZ);

#if OTP_IN_MEM
        // - Wii SEEPROM gets copied to 0xFFF07F00.
        // - Hai params get copied to 0xFFFE7D00.
        // - We want to place our OTP at 0xFFF07C80, and our payload pocket at 0xFFF07B00
        //   so we move the SEEPROM +0x400 in its buffer,
        //   And repoint 0xFFF07F00->0xFFF07B00.
        // - Since Hai params get copied after SEEPROM, bytes 0x200~0x3FF will get clobbered, but that's fine.
        //   The maximum buffer size for SEEPROM is 0x1000 (even though it could never fit at 0xFFF07F00).

        // Shift Wii SEEPROM up 0x300 and read in OTP
        BL_T_TRAMPOLINE_K(0x05008BB6, MCP_ALTBASE_ADDR(c2w_seeprom_hook_t));
        ASM_PATCH_K(0x05008C18,
            ".thumb\n"
            "mov r4, #0x50\n"
            "lsl r4, r4, #0x4\n"
        );

        BL_T_TRAMPOLINE_K(0x05008810, MCP_ALTBASE_ADDR(c2w_boot_hook_t));
        ASM_PATCH_K(0x05008814, ".thumb\nnop\n");
#endif // OTP_IN_MEM
    }
#endif // MCP_PATCHES
    
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
        // nop out memcmp hash checks
        ASM_PATCH_K(0x040017E0, "mov r0, #0");
        ASM_PATCH_K(0x040019C4, "mov r0, #0");
        ASM_PATCH_K(0x04001BB0, "mov r0, #0");
        ASM_PATCH_K(0x04001D40, "mov r0, #0");

        // hook IOSC_Decrypt after it sanity-checks input (mov r0, r5)
        BL_TRAMPOLINE_K(0x04002D5C, MCP_ALTBASE_ADDR(crypto_keychange_hook));

        // hook IOSC_Decrypt after crypto engine acquired, after r3 setup (mov r1, r6)
        BL_TRAMPOLINE_K(0x04002EE8, MCP_ALTBASE_ADDR(crypto_disable_hook));

        // hook IOSC_Encrypt after it sanity-checks input (mov r0, r5)
        BL_TRAMPOLINE_K(0x04003120, MCP_ALTBASE_ADDR(crypto_keychange_hook));

        // hook IOSC_Encrypt after crypto engine acquired, after r3 setup (mov r1, r6)
        BL_TRAMPOLINE_K(0x040032A0, MCP_ALTBASE_ADDR(crypto_disable_hook));

        // allow USB seeds which don't match otp
        ASM_PATCH_K(0x040045A8, "mov r2, #0\nmov r3, #0");

        // hook USB key generation's call to seeprom read to use a replacement seed
#if USB_SEED_SWAP
        BL_TRAMPOLINE_K(0x04004584, MCP_ALTBASE_ADDR(usb_seedswap));
#endif
    }

    // FS
    {

#if FAT32_USB
        BL_TRAMPOLINE_K(0x1078F5D8, FS_ALTBASE_ADDR(fat_register_hook));

        // extra check? lol
        BRANCH_PATCH_K(0x1078E074, 0x1078E084);

        BL_TRAMPOLINE_K(FS_GET_SPECIFIC_DEVNAME_HOOK_ADDR, FS_ALTBASE_ADDR(fsa_dev_register_hook));
#endif

        //  usb redirection and crypto disable
        BRANCH_PATCH_K(FS_USB_READ, FS_ALTBASE_ADDR(usbRead_patch));
        BRANCH_PATCH_K(FS_USB_WRITE, FS_ALTBASE_ADDR(usbWrite_patch));
        BRANCH_PATCH_K(FS_USB_SECTOR_SPOOF, FS_ALTBASE_ADDR(usb_sector_spoof));
        BRANCH_PATCH_K(WFS_CRYPTO_HOOK_ADDR, FS_ALTBASE_ADDR(wfs_crypto_hook));

#if USE_REDNAND
        // in createDevThread
        BRANCH_PATCH_K(0x10700294, FS_ALTBASE_ADDR(createDevThread_hook));
#endif

        // null out references to slcSomething1 and slcSomething2
        // (nulling them out is apparently ok; more importantly, i'm not sure what they do and would rather get a crash than unwanted slc-writing)
#if USE_REDNAND
        U32_PATCH_K(0x107B96B8, 0);
        U32_PATCH_K(0x107B96BC, 0);

        // slc redirection
        BRANCH_PATCH_K(FS_SLC_READ1, FS_ALTBASE_ADDR(slcRead1_patch));
        BRANCH_PATCH_K(FS_SLC_READ2, FS_ALTBASE_ADDR(slcRead2_patch));
        BRANCH_PATCH_K(FS_SLC_WRITE1, FS_ALTBASE_ADDR(slcWrite1_patch));
        BRANCH_PATCH_K(FS_SLC_WRITE2, FS_ALTBASE_ADDR(slcWrite2_patch));
        ASM_PATCH_K(0x107206F0, "mov r0, #0"); // nop out hmac memcmp

        // mlc redirection
        BRANCH_PATCH_K(FS_SDCARD_READ1, FS_ALTBASE_ADDR(sdcardRead_patch));
        BRANCH_PATCH_K(FS_SDCARD_WRITE1, FS_ALTBASE_ADDR(sdcardWrite_patch));
        // FS_GETMDDEVICEBYID
        BL_TRAMPOLINE_K(FS_GETMDDEVICEBYID + 0x8, FS_ALTBASE_ADDR(getMdDeviceById_hook));
        // call to FS_REGISTERMDPHYSICALDEVICE in mdMainThreadEntrypoint
        BL_TRAMPOLINE_K(0x107BD81C, FS_ALTBASE_ADDR(registerMdDevice_hook));
        // mdExit : patch out sdcard deinitialization
        ASM_PATCH_K(0x107BD374, "bx lr");
#endif // USE_REDNAND

        // mlcRead1 logging
#if PRINT_MLC_READ
        BL_TRAMPOLINE_K(FS_MLC_READ1 + 0x4, FS_ALTBASE_ADDR(mlcRead1_dbg));
#endif // PRINT_MLC_READ
#if PRINT_MLC_WRITE
        BL_TRAMPOLINE_K(FS_MLC_WRITE1 + 0x4, FS_ALTBASE_ADDR(mlcWrite1_dbg));
#endif // PRINT_MLC_WRITE

        // some selective logging function : enable all the logging!
        BRANCH_PATCH_K(0x107F5720, FS_SYSLOG_OUTPUT);

//open_base equ 0x1070AF0C
//opendir_base equ 
#if PRINT_FSAOPEN
        BL_TRAMPOLINE_K(0x1070AC94, FS_ALTBASE_ADDR(fsaopen_fullstr_dump_hook));

        // opendir_base
        BRANCH_PATCH_K(FS_OPENDIR_BASE, FS_ALTBASE_ADDR(opendir_hook));
#endif // PRINT_FSAOPEN
    
        // FSA general permissions patch
        ASM_PATCH_K(0x107043E4,
            "mov r3, #-1\n"
            "mov r2, #-1\n"
        );
    
#if PRINT_FSAREADWRITE
        BL_TRAMPOLINE_K(0x1070AA1C, FS_ALTBASE_ADDR(fread_hook));
        BL_TRAMPOLINE_K(0x1070A7A8, FS_ALTBASE_ADDR(fwrite_hook));
#endif // PRINT_FSAREADWRITE

#if PRINT_FSASEEKFILE
        BL_TRAMPOLINE_K(0x1070A46C, FS_ALTBASE_ADDR(seekfile_hook));
#endif // PRINT_FSASEEKFILE

#if MLC_ACCELERATE
        // hooks for supporting scfm.img on sys-slccmpt instead of on the sd card
        // e.g. doing sd->slc caching instead of sd->sd caching which dramatically slows down ALL i/o
        
        // disable scfmInit's slc format on name/partition type error
        ASM_PATCH_K(0x107E8178, 
            "mov r0, #0xFFFFFFFE\n"
        );
    
        //hook scfmInit right after fsa initialization, before main thread creation
        BL_TRAMPOLINE_K(0x107E7B88, FS_ALTBASE_ADDR(scfm_try_slc_cache_migration));
#endif
    }
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here kern %p, base %p\n", kern_main, wafel_plugin_base_addr);

    init_heap();
    init_phdrs();
    init_linking();

    // TODO verify the bytes that are overwritten?
    // and/or search for instructions where critical (OTP)
    // TODO don't hardcode ramdisk carveout size

    is_55x = (ios_elf_get_module_entrypoint(IOS_MCP) == 0x05056718 
              && read32(0x08120248) == 0xE92D47F0
              && read32(0x08120290) == 0xE58531EC);
    patch_general();
    if (is_55x) {
        patch_55x();
    }
    else {
        debug_printf("Firmware not 5.5.x! All you get is the OTP patch, sorry.\n");
    }

    // Make sure bss and such doesn't get initted again.
    //ASM_PATCH_K(kern_entry, "bx lr");

    ic_invalidateall();
    debug_printf("done\n");

    init_plugins();
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{
    // Make sure relocs worked fine and mappings are good
	debug_printf("we in here MCP %p\n", mcp_main);

    debug_printf("done\n");

    //const char* test_panic = "This is a test panic\n";
    //iosPanic(test_panic, strlen(test_panic)+1);

    // TODO bootstrap other thread?

    // Start up iosuhax service
    u8* main_stack = (u8*)malloc_global(0x1000);
    int main_threadhand = iosCreateThread(main_thread, NULL, (u32*)(main_stack+0x1000), 0x1000, 0x78, 1);
    if (main_threadhand >= 0) {
        iosStartThread(main_threadhand);
    }

    // Start up wupserver
    u8* wupserver_stack = (u8*)malloc_global(0x1000);
    int wupserver_threadhand = iosCreateThread(wupserver_main, NULL, (u32*)(wupserver_stack+0x1000), 0x1000, 0x78, 1);
    if (wupserver_threadhand >= 0) {
        iosStartThread(wupserver_threadhand);
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