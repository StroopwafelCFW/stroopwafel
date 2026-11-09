#include <wafel/utils.h>
#include <wafel/ios/svc.h>
#include <wafel/trampoline.h>
#include <wafel/patch.h>
#include <wafel/latte/serial.h>

#define MCP_SYSLOG_OUTPUT_ADDR (0x0503DCF8)
#define PAD_SYSLOG_OUTPUT_ADDR (0x11F7EF98)
#define FS_SYSLOG_OUTPUT 0x107F0C84

#define PRINT_PAD 0

// Hooks semihosting operations with GPIO output
#define HOOK_SEMIHOSTING 1


// just needs to exist to enable debug_printf
int enable_debug_printf;

// Hooked on `bl ringbuf_write` in MCP's ringbuf_write_overwrite (0x05055334):
// r1 = data, r2 = length. The data is length-delimited, NOT NUL-terminated
// (e.g. /dev/syslog writes from other modules / Cafe OS pass the client
// buffer as is), so it must not be passed to svc_sys_write directly: that
// printed bytes past the end until some random 0 byte (stale text from older
// messages, garbage) and stopped early on an embedded 0.
// Copy it in NUL-terminated chunks instead. Small stack buffer since this
// runs on the stack of whatever MCP thread logs.
void sys_write_r1(trampoline_state *state){
    const char *data = (const char*)state->r[1];
    u32 len = state->r[2];
    char buf[128];

    while (len) {
        u32 n = 0;
        while (len && n < sizeof(buf) - 1) {
            char c = *data++;
            len--;
            if (c)              // drop embedded NULs instead of truncating
                buf[n++] = c;
        }
        if (n) {
            buf[n] = 0;
            svc_sys_write(buf);
        }
    }
}

extern void* old_svc_handler;
extern void svc_handler_hook();

static void semihosting_write0(const char* str)
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

    // MCP: some selective logging function : enable all the logging !
    BRANCH_PATCH_K(0x05055438, MCP_SYSLOG_OUTPUT_ADDR);

    // FS: some selective logging function : enable all the logging!
    BRANCH_PATCH_K(0x107F5720, FS_SYSLOG_OUTPUT);

#if PRINT_PAD
    // PAD: some selective logging function : pad module
    BRANCH_PATCH_K(0x11f7efec, PAD_SYSLOG_OUTPUT_ADDR);
#endif

#if HOOK_SEMIHOSTING
        // Hook SVC handler for semihosting
        old_svc_handler = (void*)(*(u32*)0xFFFF0028);
        *(u32*)0xFFFF0028 = (u32)svc_handler_hook;
#endif

}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{

}