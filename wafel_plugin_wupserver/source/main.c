#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wafel/dynamic.h>
#include <wafel/utils.h>
#include <wafel/ios/svc.h>
#include <wafel/ios/memory.h>
#include "wupserver.h"

// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Make sure relocs worked fine and mappings are good
    debug_printf("we in here plugin wupserver kern %p\n", kern_main);

    debug_printf("init_linking symbol at: %08x\n", wafel_find_symbol("init_linking"));

    debug_printf("done\n");
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{
    // Make sure relocs worked fine and mappings are good
	debug_printf("we in here plugin wupserver MCP %p\n", mcp_main);

    // Start up wupserver
    u8* wupserver_stack = (u8*)malloc_global(0x1000);
    int wupserver_threadhand = iosCreateThread(wupserver_main, NULL, (u32*)(wupserver_stack+0x1000), 0x1000, 0x78, 1);
    if (wupserver_threadhand >= 0) {
        iosStartThread(wupserver_threadhand);
    }

    debug_printf("done wupserver MCP\n");
}