#ifndef IMPORTS_H
#define IMPORTS_H

#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "ios/thread.h"

#define MCP_SVC_BASE ((void*)0x050567EC)

void ios_abort(const char *format, ...);

#endif
