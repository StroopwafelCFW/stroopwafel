#include <wafel/utils.h>
#include <wafel/ios/svc.h>
#include <wafel/trampoline.h>
#include "config.h"


void sys_write_r1(trampoline_state *state){
    // buffer is in r1
    svc_sys_write((char*)state->r[1]);
}


static void mcp_patches(void){
    trampoline_hook_before(0x05055334, sys_write_r1);

    // MCP: some selective logging function : enable all the logging !
    BRANCH_PATCH_K(0x05055438, MCP_SYSLOG_OUTPUT_ADDR);
}

static void fs_patches(void){
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
    // ASM_PATCH_K(0x107043E4,
    //     "mov r3, #-1\n"
    //     "mov r2, #-1\n"
    // );
    
#if PRINT_FSAREADWRITE
    BL_TRAMPOLINE_K(0x1070AA1C, FS_ALTBASE_ADDR(fread_hook));
    BL_TRAMPOLINE_K(0x1070A7A8, FS_ALTBASE_ADDR(fwrite_hook));
#endif // PRINT_FSAREADWRITE

#if PRINT_FSASEEKFILE
    BL_TRAMPOLINE_K(0x1070A46C, FS_ALTBASE_ADDR(seekfile_hook));
#endif // PRINT_FSASEEKFILE
}

static void pad_patches(void){
#if PRINT_PAD
    // PAD: some selective logging function : pad module
    BRANCH_PATCH_K(0x11f7efec, PAD_SYSLOG_OUTPUT_ADDR);
#endif
}

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here in wafel_debug_exts plugin kern %p\n", kern_main);

    mcp_patches();
    fs_patches();
    pad_patches();

}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}