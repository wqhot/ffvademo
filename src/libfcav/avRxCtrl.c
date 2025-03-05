#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ulibuser.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>

unsigned g_fill = 0;

/*
struct fb_fix_screeninfo {
	char id[16];
	unsigned long smem_start;
	__u32 smem_len;
	__u32 type;
	__u32 type_aux;
	__u32 visual;
	__u16 xpanstep;
	__u16 ypanstep;
	__u16 ywrapstep;
	__u32 line_length;
	unsigned long mmio_start;
	__u32 mmio_len;
	__u32 accel;
	__u16 capabilities;
};
*/
void printPF(struct fb_fix_screeninfo* pF)
{
    printf("------- FIX_screeninfo -----------\n");
    printf("id = %s\n", pF->id);
    printf("smem_start = 0x%lx\n", pF->smem_start);
    printf("smem_len = 0x%x\n", pF->smem_len);
    printf("type = 0x%x\n", pF->type);
    printf("type_aux = 0x%x\n", pF->type_aux);
    printf("visual = 0x%x\n", pF->visual);
    printf("line_len = 0x%x\n", pF->line_length);
    printf("mmio_start = 0x%lx\n", pF->mmio_start);
    printf("mmio_len = 0x%x\n", pF->mmio_len);
    printf("capabilities = 0x%x\n", pF->capabilities);
}

void printPV(struct fb_var_screeninfo* pV)
{
    printf("x_res=%d, y_res=%d\n", pV->xres, pV->yres);
}

int get_fb_info(struct fb_fix_screeninfo* pF, struct fb_var_screeninfo* pV)
{
    int retv, fd = open("/dev/fb0", O_RDWR, 0);

    if (fd <= 0)
    {
        printf("open fb0 return %d\n", fd);
        return  fd;
    }
    if ((retv = ioctl(fd, FBIOGET_FSCREENINFO, pF)) < 0)
    {
        printf("FBIOGET_FSCREENINFO fail %d\n", retv);
        close(fd);
        return  retv;
    } else
        printPF(pF);
    retv = ioctl(fd, FBIOGET_VSCREENINFO, pV);
    printPV(pV);
//////////
//    printf("bits_per_pixel = %d\n", pV->bits_per_pixel);
//    pV->bits_per_pixel = 32;
//    retv = ioctl(fd, FBIOPUT_VSCREENINFO, pV);
//    printf("FBIOPUT_VSCREENINFO return %d\n", retv);
//    retv = ioctl(fd, FBIOGET_VSCREENINFO, pV);
//    printf("bits_per_pixel = %d\n", pV->bits_per_pixel);
//////////r
    close(fd);

    if (retv < 0)
        printf("FBIOGET_VSCREENINFO fail %d\n", retv);
    return  0;
}

static void my_handler(void* ctx)
{
    printf("user handler: receive frame, vid=%d!\n", *(unsigned*)ctx);
}

int main(int argc, char* argv[])
{
    unsigned src_id = myatoi(argv[1]), local_port_id = 0, syn_flag = 0x40000000, posx = 0, posy = 0;
    int  tid;
    char* rxBuf = 0;
    unsigned vid, width=1024, height=768, clrBits=32, clrType=0;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;

    if ((argc < 3)||(argc >11))
    {
        printf("usage: avRxCtrl <srcId> <videoId> [width=1024] [height=768] [clrBits=32] [clrType=0] [local_port_id] [syn_flag] [x] [y]\n");
        return -1;
    }
    vid = myatoi(argv[2]);

    if (argc >3)
        width = myatoi(argv[3]);

    if (argc >4)
        height = myatoi(argv[4]);

    if (argc >5)
        clrBits = myatoi(argv[5]);

    if (argc >6)
        clrType = myatoi(argv[6]);

    if ((argc >7) && (myatoi(argv[7]) == getNPortId(1)))
        local_port_id = 1; //判断是否第2路FC接收

    if (argc >8)
        syn_flag = (myatoi(argv[8]) ? 0x40000000 : 0);

    if (argc >9)
        posx = myatoi(argv[9]);

    if (argc >10)
        posy = myatoi(argv[10]);

    //if (get_fb_info(&fix, &var) <0)
    //    return  -1;

    //printf("browAvTxCtrl return 0x%x.\n", browAvTxCtrl(src_id, vid, self_id, width, height, 0, 0, width, height));

    //rxBuf = map_videobuf(fix.smem_start, width*height*clrBits/8);
    /*rxBuf = map_txbuf(width*height*clrBits/8); 
    if (!rxBuf)
    {
        printf("rxBuf mmap error!!!\n");
        return	-1;
    } else
        printf("allocate avrx mem %p\n", rxBuf);
    */
    tid = add_avrx_handler(my_handler, -1/*means any vid*/);
    printf("add_avrx_handler return %d\n", tid);
    printf("before call av_rx_ctrl.\n");
    usleep(125000);
    av_rx_ctrl(vid, 1, src_id, posx, posy, width, height, clrBits + clrType*65536 + local_port_id*0x80000000 + syn_flag, rxBuf);
    printf("after call av_rx_ctrl.\n");
    while (1)
    {
        usleep(50*1000);
        //memset(rxBuf, g_fill, width*height*clrBits/8);
        //g_fill += 0x22;
        //printf("press any key to continue.\n");
        getchar();
        //av_rx_ctrl(vid, 1, src_id, posx, posy, width, height, clrBits, rxBuf);
    }
    return 0;
}
