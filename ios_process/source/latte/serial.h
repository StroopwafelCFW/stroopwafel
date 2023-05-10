
#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "types.h"

void serial_fatal();
void serial_force_terminate();
void serial_send_u32(u32 val);
int serial_in_read(u8* out);
void serial_poll();
void serial_allow_zeros();
void serial_disallow_zeros();
void serial_send(u8 val);
void serial_line_inc();
void serial_clear();
void serial_line_noscroll();
int serial_send_str(const char* str);
int serial_printf(const char* fmt, ...);

#endif // __SERIAL_H__