#ifndef IMPORTS_H
#define IMPORTS_H

#include <stdlib.h>
#include <stdarg.h>
#include "types.h"

#define MCP_SVC_BASE ((void*)0x050567EC)

// TODO replace
void usleep(u32 time);
void ios_abort(const char *format, ...);

#endif
