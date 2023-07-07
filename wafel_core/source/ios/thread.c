#include "ios/thread.h"

#include "ios/svc.h"

int usleep(u32 amt)
{
    ios_retval ret;
    u32 tmpQueue;
    u32 throwaway;

    ret = iosCreateMessageQueue(&tmpQueue, 1);
    if (ret >= 0)
    {
        int qh = ret;
        ret = iosCreateTimer(amt, 0, qh, 0xBABECAFE);
        int th = ret;
        if (ret < 0)
        {
            iosDestroyMessageQueue(qh);
            return ret;
        }
        else
        {
            iosReceiveMessage(qh, &throwaway, 0);
            iosDestroyTimer(th);
            iosDestroyMessageQueue(qh);
            return 0;
        }
    }
    return ret;
}

int msleep(u32 amt)
{
    return usleep(amt*1000);
}
