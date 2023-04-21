
#include <stdio.h>
#include <string.h>
#include "utils.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif


u32 align(u32 offset, u32 alignment)
{
	u32 mask = ~(alignment-1);

	return (offset + (alignment-1)) & mask;
}

u64 align64(u64 offset, u32 alignment)
{
	u64 mask = ~(alignment-1);

	return (offset + (alignment-1)) & mask;
}

u64 getle64(const void* p)
{
    u8* p8 = (u8*)p;
	u64 n = p8[0];

	n |= (u64)p8[1]<<8;
	n |= (u64)p8[2]<<16;
	n |= (u64)p8[3]<<24;
	n |= (u64)p8[4]<<32;
	n |= (u64)p8[5]<<40;
	n |= (u64)p8[6]<<48;
	n |= (u64)p8[7]<<56;
	return n;
}

u64 getbe64(const void* p)
{
	u64 n = 0;
    u8* p8 = (u8*)p;

	n |= (u64)p8[0]<<56;
	n |= (u64)p8[1]<<48;
	n |= (u64)p8[2]<<40;
	n |= (u64)p8[3]<<32;
	n |= (u64)p8[4]<<24;
	n |= (u64)p8[5]<<16;
	n |= (u64)p8[6]<<8;
	n |= (u64)p8[7]<<0;
	return n;
}

u32 getle32(const void* p)
{
    u8* p8 = (u8*)p;
	return (p8[0]<<0) | (p8[1]<<8) | (p8[2]<<16) | (p8[3]<<24);
}

u32 getbe32(const void* p)
{
    u8* p8 = (u8*)p;
	return (p8[0]<<24) | (p8[1]<<16) | (p8[2]<<8) | (p8[3]<<0);
}

u32 getle16(const void* p)
{
    u8* p8 = (u8*)p;
	return (p8[0]<<0) | (p8[1]<<8);
}

u32 getbe16(const void* p)
{
    u8* p8 = (u8*)p;
	return (p8[0]<<8) | (p8[1]<<0);
}

void putle16(void* p, u16 n)
{
    u8* p8 = (u8*)p;
	p8[0] = n;
	p8[1] = n>>8;
}

void putle32(void* p, u32 n)
{
    u8* p8 = (u8*)p;
	p8[0] = n;
	p8[1] = n>>8;
	p8[2] = n>>16;
	p8[3] = n>>24;
}

void putbe16(void* p, u16 n)
{
    u8* p8 = (u8*)p;
	p8[1] = n;
	p8[0] = n>>8;
}

void putbe32(void* p, u32 n)
{
    u8* p8 = (u8*)p;
	p8[3] = n;
	p8[2] = n>>8;
	p8[1] = n>>16;
	p8[0] = n>>24;
}


void readkeyfile(u8* key, const char* keyfname)
{
	FILE* f = fopen(keyfname, "rb");
	u32 keysize = 0;

	if (0 == f)
	{
		fprintf(stdout, "Error opening key file\n");
		goto clean;
	}

	fseek(f, 0, SEEK_END);
	keysize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (keysize != 16)
	{
		fprintf(stdout, "Error key size mismatch, got %d, expected %d\n", keysize, 16);
		goto clean;
	}

	if (16 != fread(key, 1, 16, f))
	{
		fprintf(stdout, "Error reading key file\n");
		goto clean;
	}

clean:
	if (f)
		fclose(f);
}

void hexdump(void *ptr, int buflen)
{
	u8 *buf = (u8*)ptr;
	int i, j;

	for (i=0; i<buflen; i+=16)
	{
		printf("%06x: ", i);
		for (j=0; j<16; j++)
		{ 
			if (i+j < buflen)
			{
				printf("%02x ", buf[i+j]);
			}
			else
			{
				printf("   ");
			}
		}

		printf(" ");

		for (j=0; j<16; j++) 
		{
			if (i+j < buflen)
			{
				printf("%c", (buf[i+j] >= 0x20 && buf[i+j] <= 0x7e) ? buf[i+j] : '.');
			}
		}
		printf("\n");
	}
}

void memdump(FILE* fout, const char* prefix, const u8* data, u32 size)
{
	u32 i;
	u32 prefixlen = strlen(prefix);
	u32 offs = 0;
	u32 line = 0;
	while(size)
	{
		u32 max = 32;

		if (max > size)
			max = size;

		if (line==0)
			fprintf(fout, "%s", prefix);
		else
			fprintf(fout, "%*s", prefixlen, "");


		for(i=0; i<max; i++)
			fprintf(fout, "%02X", data[offs+i]);
		fprintf(fout, "\n");
		line++;
		size -= max;
		offs += max;
	}
}


static int ishex(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	if (c >= 'a' && c <= 'f')
		return 1;
	return 0;

}

static unsigned char hextobin(char c)
{
	if (c >= '0' && c <= '9')
		return c-'0';
	if (c >= 'A' && c <= 'F')
		return c-'A'+0xA;
	if (c >= 'a' && c <= 'f')
		return c-'a'+0xA;
	return 0;
}

int hex2bytes(const char* text, unsigned int textlen, unsigned char* bytes, unsigned int size)
{
	unsigned int i, j;
	unsigned int hexcount = 0;

	for(i=0; i<textlen; i++)
	{
		if (ishex(text[i]))
			hexcount++;
	}

	if (hexcount != size*2)
	{
		fprintf(stdout, "Error, expected %d hex characters when parsing text \"", size*2);
		for(i=0; i<textlen; i++)
			fprintf(stdout, "%c", text[i]);
		fprintf(stdout, "\"\n");
		
		return -1;
	}

	for(i=0, j=0; i<textlen; i++)
	{
		if (ishex(text[i]))
		{
			if ( (j&1) == 0 )
				bytes[j/2] = hextobin(text[i])<<4;
			else
				bytes[j/2] |= hextobin(text[i]);
			j++;
		}
	}
	
	return 0;
}

int makedir(const char* dir)
{
#ifdef _WIN32
	return _mkdir(dir);
#else
	return mkdir(dir, 0777);
#endif
}
