#pragma once
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <dirent.h>

inline int get_fc_pci_reg_path(char *buf)
{
    DIR *dir;
    struct dirent *dir_ent;

    strcpy(buf, "/sys/module/fcve/drivers/pci:myFc2V/");

    if (0 == (dir = opendir(buf)))
    {
        printf("open directory fail!\n");
        return -1;
    }
    while (0 != (dir_ent = readdir(dir)))
    {
        if (!memcmp(dir_ent->d_name, "0000:", 5))
        {
            strcpy(buf + strlen(buf), dir_ent->d_name);
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return -1;
}

inline static FILE *open_reg_by_name(const char *reg_path, const char *name, const char *mode)
{
    char filename[256];
    FILE *fp;

    sprintf(filename, "%s/card_r%s", reg_path, name);
    return fopen(filename, mode);
}

inline static void write_reg(const char *reg_path, const char *name, unsigned val)
{
    char buf[16];
    FILE *fp = open_reg_by_name(reg_path, name, "wb");

    if (fp)
    {
        sprintf(buf, "0x%08x", val);
        fwrite(buf, 10, sizeof(buf), fp);
        fclose(fp);
    }
}

inline static unsigned write_reg_any(const char *reg_path, unsigned regNr, unsigned val)
{
    write_reg(reg_path, "Addr", regNr);
    write_reg(reg_path, "Data", val);
}

inline static int get_mod_status()
{
    FILE *f = fopen("/proc/modules", "r");
    if (!f)
    {
        return -1;
    }
    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, f) > 0)
    {
        if (!strncmp(line, "fcve", 4))
        {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return -1;
}

inline static int is_asic_ver2(char *reg_path)
{
    FILE *fp = open_reg_by_name(reg_path, "900", "rb");

    if (!fp)
        return 0;
    fclose(fp);
    return 1;
}

inline static unsigned myatoi(const char *p)
{
    unsigned retv = 0, u, i = 0;

    if ((p[0] == '0') && (p[1] == 'x'))
    {
        i = 2;

        while (((p[i] >= '0') && (p[i] <= '9')) || ((p[i] >= 'a') && (p[i] <= 'z')) || ((p[i] >= 'A') && (p[i] <= 'Z')))
        {
            if (p[i] <= '9')
                u = p[i] - '0';
            else if (p[i] <= 'Z')
                u = p[i] - 'A' + 10;
            else
                u = p[i] - 'a' + 10;
            retv = retv * 16 + u;
            ++i;
        }
        return retv;
    }
    while ((p[i] >= '0') && (p[i] <= '9'))
    {
        retv = retv * 10 + p[i] - '0';
        ++i;
    }
    return retv;
}

inline static const char *nr_to_name(unsigned regNr)
{
    static char name[16];
    sprintf(name, "%02x", regNr);
    return name;
}

static unsigned read_reg(const char *reg_path, const char *name)
{
    char buf[32] = {0}, *p;
    FILE *fp = open_reg_by_name(reg_path, name, "rb");

    if (fp)
    {
        fread(buf, 1, sizeof(buf), fp);
        fclose(fp);

        if (0 != (p = strstr(buf, "= ")))
            return myatoi(p + 2);
    }
    return 0;
}

inline static int insmod_fc_driver()
{
    char buf[256];
    int ret = 0;

    if (get_mod_status() == 0)
    {
        return 0;
    }

    int res = system("insmod ./fcve.ko param_no_port_reset=1 p2p_portid=10 param_enable_ip=1 param_ip_stack=1");
    if (res != 0)
    {
        printf("insmod fc driver failed\n");
        return -1;
    }

    if (get_mod_status() != 0)
    {
        return -1;
    }

    // read r14
    char reg_path[256];
    if (get_fc_pci_reg_path(reg_path) == 0)
    {
        unsigned int r14 = read_reg(reg_path, "14");
        int try_count = 0;

        while (!((r14 & 0xf000) == 0xf000 || (r14 & 0xf000) == 0xd000))
        {
            usleep(100);
            write_reg(reg_path, "14", 0xf0c00000);
            r14 = read_reg(reg_path, "14");
            if (try_count++ > 100)
            {
                printf("read r14 failed\n");
                return -1;
            }
        }
        printf("fc driver is loaded\n");
    }
    else
    {
        printf("get fc pci reg path failed\n");
        return -1;
    }

    return 0;
}

inline static int port_login(char *reg_path, unsigned portNr, unsigned target_portid)
{
    unsigned regNr = (is_asic_ver2(reg_path) ? 0x824 : 0x24) + portNr * 0x100;

    write_reg(reg_path, nr_to_name(regNr), 0x80000000 + target_portid);
    sleep(1);
    int loginResult = (read_reg(reg_path, nr_to_name(regNr)) & 0x20000000);
    if (loginResult == 0x20000000)
    {
        loginResult = 1;
    }
    else
    {
        loginResult = -1;
    }
    return loginResult;
}