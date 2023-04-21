#ifndef _UTILS_H_
#define _UTILS_H_

#include "types.h"

#ifdef _WIN32
#define PATH_SEPERATOR '\\'
#else
#define PATH_SEPERATOR '/'
#endif

#ifndef MAX_PATH
	#define MAX_PATH 255
#endif

#ifdef __cplusplus
extern "C" {
#endif

u32 align(u32 offset, u32 alignment);
u64 align64(u64 offset, u32 alignment);
u64 getle64(const void* p);
u32 getle32(const void* p);
u32 getle16(const void* p);
u64 getbe64(const void* p);
u32 getbe32(const void* p);
u32 getbe16(const void* p);
void putle16(void* p, u16 n);
void putle32(void* p, u32 n);
void putbe16(void* p, u16 n);
void putbe32(void* p, u32 n);

void readkeyfile(u8* key, const char* keyfname);
void memdump(FILE* fout, const char* prefix, const u8* data, u32 size);
void hexdump(void *ptr, int buflen);
int key_load(char *name, u8 *out_buf);
int hex2bytes(const char* text, unsigned int textlen, unsigned char* bytes, unsigned int size);

int makedir(const char* dir);

#ifdef __cplusplus
}
#endif

#endif // _UTILS_H_
