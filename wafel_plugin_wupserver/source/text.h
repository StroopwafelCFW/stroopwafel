#ifndef TEXT_H
#define TEXT_H

#include <wafel/types.h>

void clearScreen(u32 color);
void drawString(char* str, int x, int y);
void print_log(int x, int y, const char *format, ...);
void print(int x, int y, const char *format, ...);

#endif
