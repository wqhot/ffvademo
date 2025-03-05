#include <stdio.h>
#include "ulibuser.h"

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        printf("syntax error.\n");
        return  -1;
    }
    printf("%x\n",rw_write_reg(myatoi(argv[1]), myatoi(argv[2]), myatoi(argv[3])/*ofs*/, myatoi(argv[4])/*regvalue*/));
    return  0;
}
