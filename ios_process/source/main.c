#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "ios/svc.h"
#include "dynamic.h"
#include "ios/memory.h"
#include "wupserver/wupserver.h"

volatile int bss_var;
volatile int data_var = 0x12345678;

u32 main_thread(void*);

void debug_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    static char buffer[0x100];

    vsnprintf(buffer, 0xFF, format, args);
    svc_sys_write(buffer);

    va_end(args);
}

void _main()
{
    // Make sure relocs worked fine and mappings are good
	debug_printf("we in here %p\n", _main);
    bss_var = 0x12345678;
    data_var = 0;

    debug_printf("done\n");

    for (int i = 0; i < 0x10; i += 4)
    {
        u32 val = *(u32*)0xE108E860+i;
        debug_printf("%08x\n", val);
    }

    // TODO bootstrap other thread?

    u8* main_stack = (u8*)malloc_global(0x1000);
    int main_threadhand = svcCreateThread(main_thread, NULL, (u32*)(main_stack+0x1000), 0x1000, 0x78, 1);
    if (main_threadhand >= 0) {
        svcStartThread(main_threadhand);
    }

    u8* wupserver_stack = (u8*)malloc_global(0x1000);
    int wupserver_threadhand = svcCreateThread(wupserver_main, NULL, (u32*)(wupserver_stack+0x1000), 0x1000, 0x78, 1);
    if (wupserver_threadhand >= 0) {
        svcStartThread(wupserver_threadhand);
    }
}

u32 main_thread(void* arg)
{
    while (1) {
        usleep(5*1000*1000);
        debug_printf("alive\n");
    }
}