#ifndef _IRQ_H
#define _IRQ_H

#include "types.h"

u32 irq_kill(void);
void irq_restore(u32 cookie);

#endif // _IRQ_H