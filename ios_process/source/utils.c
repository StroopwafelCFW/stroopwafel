#include "utils.h"

#include "ios/svc.h"
#include "imports.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void debug_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    static char buffer[0x100];

    vsnprintf(buffer, 0xFF, format, args);
    svc_sys_write(buffer);

    va_end(args);
}