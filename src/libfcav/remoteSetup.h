#pragma once
#include <unistd.h>
#include <stdio.h>
#include "ulibuser.h"

struct remote_setup_
{
    unsigned id;
    unsigned addrHi;
    unsigned addrLo;
    unsigned bytes;
};

int remote_setup(unsigned port_id, unsigned buff_id, unsigned num, unsigned *addr, unsigned *bytes)
{
    unsigned tgtPrtId = 0, tgtBufId = 0, i, memCnt = 1;
    struct remote_setup_ param[16];

    tgtPrtId = port_id;
    tgtBufId = buff_id;
    memCnt = num;

    rw_lock(tgtPrtId, tgtBufId);
    usleep(500);

    for (i = 0; i < memCnt; ++i)
    {
        param[i].id = 0xFFFF0000 + i;
        param[i].addrHi = i ? 4 : 0;
        param[i].addrLo = addr[i];
        param[i].bytes = bytes[i];

        printf("setting share mem %x : %d bytes @%x%08x.\n", param[i].id, param[i].bytes, param[i].addrHi, param[i].addrLo);
    }
    usleep(500);
    rw_write_mem(tgtPrtId, tgtBufId, 0, param, sizeof(param[0]) * memCnt);
    usleep(500);
    rw_unlock(tgtPrtId, tgtBufId);
}