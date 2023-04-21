#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;

typedef enum { false, true } bool;

enum flags
{
	ExtractFlag = (1<<0),
	InfoFlag = (1<<1),
	PlainFlag = (1<<2),
	VerboseFlag = (1<<3),
	VerifyFlag = (1<<4),
	RawFlag = (1<<5),
	ShowKeysFlag = (1<<6),
	DecompressCodeFlag = (1<<7)
};

enum validstate
{
	Unchecked = 0,
	Good = 1,
	Fail = 2,
};

enum sizeunits
{
	sizeKB = 0x400,
	sizeMB = 0x100000,
};

#endif
