#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wafel/dynamic.h>
#include <wafel/utils.h>

#define TV_FB (0x17500000)
#define TV_WIDTH 1280
#define TV_HEIGHT 720
#define TV_STRIDE (TV_WIDTH * 4)

#define WHITE 0xFFFFFFFF
#define BLACK 0x00000000

extern const u8 msx_font[];

void draw_char(char c, int x, int y, u32 color)
{
    if (c < 32 || c >= 128) return;
    c -= 32;

    const u8* charData = &msx_font[8 * c];
    u32* fb = (u32*)(TV_FB + (x * 4) + (y * TV_STRIDE));

    for (int i = 0; i < 8; ++i)
    {
        u8 v = charData[i];
        for (int j = 0; j < 8; ++j)
        {
            if (v & (128 >> j)) fb[j] = color;
            else fb[j] = BLACK;
        }
        fb += (TV_STRIDE / 4);
    }
}

void draw_string(const char* str, int x, int y, u32 color)
{
    int dx = 0, dy = 0;
    for (int k = 0; str[k]; k++)
    {
        if (str[k] == '\n')
        {
            dx = 0;
            dy += 8;
            continue;
        }

        draw_char(str[k], x + dx, y + dy, color);
        dx += 8;
        if (dx + 8 > TV_WIDTH)
        {
            dx = 0;
            dy += 8;
        }
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
    debug_printf("wafel_tv_log: kern_main\n");

    draw_string("Hello World from Stroopwafel Plugin!", 100, 100, WHITE);

    debug_printf("wafel_tv_log: done\n");
}

// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{
	debug_printf("wafel_tv_log: mcp_main\n");
}
