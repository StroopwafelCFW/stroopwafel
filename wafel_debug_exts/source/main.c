#include <wafel/utils.h>
#include <wafel/ios/svc.h>
#include <wafel/trampoline.h>


void sys_write_r1(trampoline_state *state){
    // buffer is in r1
    svc_sys_write((char*)state->r[1]);
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

    trampoline_hook_before(0x05055334, sys_write_r1);
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}