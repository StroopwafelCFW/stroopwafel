#ifndef NET_IFMGR_NCL
#define NET_IFMGR_NCL

#include "../types.h"

LINKABLE int ifmgrnclInit();
LINKABLE int ifmgrnclExit();

LINKABLE int	IFMGRNCL_GetInterfaceStatus(u16 interface_id, u16* out_status);

#endif
