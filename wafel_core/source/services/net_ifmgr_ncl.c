#include <stdlib.h>
#include <string.h>
#include "services/net_ifmgr_ncl.h"
#include "imports.h"
#include "ios/svc.h"

static int ifmgrncl_handle = 0;

int ifmgrnclInit()
{
	if(ifmgrncl_handle) return ifmgrncl_handle;
	
	int ret = iosOpen("/dev/net/ifmgr/ncl", 0);

	if(ret > 0)
	{
		ifmgrncl_handle = ret;
		return ifmgrncl_handle;
	}

	return ret;
}

int ifmgrnclExit()
{
	int ret = iosClose(ifmgrncl_handle);

	ifmgrncl_handle = 0;

	return ret;
}

static void* allocIobuf(u32 size)
{
	void* ptr = iosAlloc(0xCAFF, size);

	if(ptr) memset(ptr, 0x00, size);

	return ptr;
}

static void freeIobuf(void* ptr)
{
	iosFree(0xCAFF, ptr);
}

int	IFMGRNCL_GetInterfaceStatus(u16 interface_id, u16* out_status)
{
	u8* iobuf1 = allocIobuf(0x2);
	u16* inbuf = (u16*)iobuf1;
	u8* iobuf2 = allocIobuf(0x8);
	u16* outbuf = (u16*)iobuf2;

	inbuf[0] = interface_id;

	int ret = iosIoctl(ifmgrncl_handle, 0x14, inbuf, 0x2, outbuf, 0x8);

	if(!ret && out_status) *out_status = outbuf[2];

	freeIobuf(iobuf1);
	freeIobuf(iobuf2);
	return ret;
}
