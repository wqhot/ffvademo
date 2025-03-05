#pragma once
#include "remoteSetup.h"
#include "fcUtils.h"

int start_cam(unsigned port_id, unsigned tgt_id, unsigned win_id, unsigned w, unsigned h, unsigned r20100)
{
    char reg_path[256];
    if (get_fc_pci_reg_path(reg_path) != 0)
    {
        printf("get fc pci reg path failed\n");
        return -1;
    }

    unsigned addr = 0;
    unsigned data = 0x30000;
    remote_setup(port_id, 0xFFFF8000, 1, &addr, &data);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0xFC, 0x11223344);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x10, 0x80000);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1224, 2000);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1200, win_id);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1208, w << 16);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x120C, h * w);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1210, w << 16 + h);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1214, 0);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1218, 0);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x121C, 0xb0000000 + tgt_id);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x122C, 0x44);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1228, 0x20000000);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    // wait for bit29 cleared. so 0x200xxxxx is wrong, output 0x10000 is right, and the length should be 7.
    unsigned ctl_reg = rw_read_reg(port_id, 0xFFFF0000, 0x1228);
    if (ctl_reg > 0xfffff || ctl_reg < 0x10000)
    {
        usleep(2000 * 1000);
    }
    printf("ctl_reg = %x\n", ctl_reg);

    rw_write_reg(port_id, 0xFFFF0000, 0x1200, win_id);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x1228, 0x80000800);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x20100, r20100);
    usleep(50 * 1000);
    printf("49c = %x\n", read_reg(reg_path, "49c"));

    rw_write_reg(port_id, 0xFFFF0000, 0x20110, 1);
    return 0;
}

int stop_cam(unsigned port_id)
{
    char reg_path[256];
    if (get_fc_pci_reg_path(reg_path) != 0)
    {
        printf("get fc pci reg path failed\n");
        return -1;
    }

    unsigned addr = 0;
    unsigned data = 0x30000;
    remote_setup(port_id, 0xFFFF8000, 1, &addr, &data);
    usleep(50 * 1000);

    rw_write_reg(port_id, 0xFFFF0000, 0xFC, 0x11223344);
    usleep(50 * 1000);

    rw_write_reg(port_id, 0xFFFF0000, 0x10, 0x80000);
    usleep(50 * 1000);
    
    rw_write_reg(port_id, 0xFFFF0000, 0x20110, 0);
    usleep(50 * 1000);

    rw_write_reg(port_id, 0xFFFF0000, 0x1224, 0x800007d0);
    usleep(50 * 1000);
    return 0;
}