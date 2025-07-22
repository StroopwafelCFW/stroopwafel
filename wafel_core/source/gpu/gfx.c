/*
 *  minute - a port of the "mini" IOS replacement for the Wii U.
 *
 *  Copyright (C) 2016          SALT
 *  Copyright (C) 2016          Daz Jones <daz@dazzozo.com>
 *
 *  This code is licensed to you under the terms of the GNU GPL, version 2;
 *  see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 */

#include "gfx.h"
//#include "serial.h"
#include "gpu.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static bool printf_to_display = false;

void gfx_printf_to_display(bool on){
	printf_to_display = on;
}


extern const u8 msx_font[];

#define CHAR_SIZE_X (8)
#define CHAR_SIZE_Y (8)

struct {
	u32* ptr;
	int width;
	int height;
	size_t bpp;

	int current_y;
	int current_x;
} fbs[GFX_ALL] = {
	[GFX_TV] =
	{
		.ptr = (u32*)(0x14000000 + 0x3500000),
		.width = 1280,
		.height = 720,
		.bpp = 4,

		.current_y = 10,
		.current_x = 10,
	},
	[GFX_DRC] =
	{
		.ptr = (u32*)(0x14000000 + 0x38C0000),
		.width = 896,
		.height = 504,
		.bpp = 4,

		.current_y = 10,
		.current_x = 10,
	},
};

static int gfx_currently_headless = 0;

void gfx_init(void)
{
	if (!gpu_tv_primary_surface_addr()) {
		gfx_currently_headless = 1;
		return;
	}

	/*if (gpu_tv_primary_surface_addr())
		fbs[GFX_TV].ptr = (u32*)gpu_tv_primary_surface_addr();
	if (gpu_drc_primary_surface_addr())
		fbs[GFX_DRC].ptr = (u32*)gpu_drc_primary_surface_addr();*/

	gfx_clear(GFX_ALL, BLACK);
}

bool gfx_is_currently_headless(void)
{
	return gfx_currently_headless;
}

size_t gfx_get_stride(gfx_screen_t screen)
{
	if(screen == GFX_ALL) return 0;

	return fbs[screen].bpp * fbs[screen].width;
}

size_t gfx_get_size(gfx_screen_t screen)
{
	if(screen == GFX_ALL) return 0;

	return gfx_get_stride(screen) * fbs[screen].height;
}

void gfx_draw_plot(gfx_screen_t screen, int x, int y, u32 color)
{
	if(screen == GFX_ALL) {
		for(int i = 0; i < GFX_ALL; i++)
			gfx_draw_plot(i, x, y, color);
	} else {
	    u32* fb = &fbs[screen].ptr[x + y * (gfx_get_stride(screen) / sizeof(u32))];
	    *fb = color;
	}
}

void gfx_clear(gfx_screen_t screen, u32 color)
{
	//if (gfx_currently_headless) return;

	if(screen == GFX_ALL) {
		for(int i = 0; i < GFX_ALL; i++)
			gfx_clear(i, color);
	} else {
	    for(int i = 0; i < fbs[screen].width * fbs[screen].height; i++)
	        fbs[screen].ptr[i] = color;

	    fbs[screen].current_x = 10;
	    fbs[screen].current_y = 10;
	}
}

void gfx_draw_char(gfx_screen_t screen, char c, int x, int y, u32 color)
{
	if (gfx_currently_headless) return;

	if(screen == GFX_ALL) {
		for(int i = 0; i < GFX_ALL; i++)
			gfx_draw_char(i, c, x, y, color);
	} else {
		if(c < 32) return;
		c -= 32;

		const u8* charData = &msx_font[(CHAR_SIZE_X * CHAR_SIZE_Y * c) / 8];
		u32* fb = &fbs[screen].ptr[x + y * (gfx_get_stride(screen) / sizeof(u32))];

		for(int i = 0; i < CHAR_SIZE_Y; ++i)
		{
			u8 v = *(charData++);

			for(int j = 0; j < CHAR_SIZE_X; ++j)
			{
				if(v & (128 >> j)) *fb = color;
				else *fb = 0x00000000;
				fb++;
			}

			fb += (gfx_get_stride(screen) / sizeof(u32)) - CHAR_SIZE_X;
		}
	}
}

void gfx_draw_string(gfx_screen_t screen, char* str, int x, int y, u32 color)
{
	if (gfx_currently_headless) return;

	if(screen == GFX_ALL) {
		for(int i = 0; i < GFX_ALL; i++)
			gfx_draw_string(i, str, x, y, color);
	} else {
		if(!str) return;

		int dx = 0, dy = 0;
		for(int k = 0; str[k]; k++)
		{
			if(str[k] >= 32 && str[k] < 128)
				gfx_draw_char(screen, str[k], x + dx, y + dy, color);

			dx += 8;

			if(str[k] == '\n')
			{
				dx = 0;
				dy -= 8;
			}
		}
	}
}

// This sucks, should use a stdout devoptab.
int printf(const char* fmt, ...)
{
	static char str[0x800];
	va_list va;

	va_start(va, fmt);
	vsnprintf(str, sizeof(str), fmt, va);
	va_end(va);

	// char* str_iter = str;
	// while (*str_iter)
	// {
	// 	if (*str_iter == '\n') {
	// 		serial_line_inc();
	// 	}
	// 	serial_send(*str_iter);
	// 	str_iter++;
	// }

	if(!printf_to_display)
		return 0;

	int lines = 0;
	char* last_line = str;
	for(int k = 0; str[k]; k++)
	{
		if(str[k] == '\n')
		{
			lines += 10;
			last_line = &str[k + 1];
		}
	}

	for(int i = 0; i < GFX_ALL; i++) {
		if(fbs[i].current_y + lines >= fbs[i].height - 20)
			gfx_clear(i, BLACK);

		gfx_draw_string(i, str, fbs[i].current_x, fbs[i].current_y, WHITE);
		if (!lines) {
			fbs[i].current_x += ((strlen(last_line)-1) * CHAR_WIDTH);
		}
		else {
			fbs[i].current_x = 10;
		}

		fbs[i].current_y += lines;
	}

    return 0;
}