#include "fcReceiver.h"
#include "ulibuser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <thread>
#include <stdlib.h>

FCReceiver *FCReceiver::getInstance()
{
    static FCReceiver *instance = new FCReceiver();
    return instance;
}

static void sig_handler(void *ctx)
{
    FCReceiver::getInstance()->gotFrame();
    // printf("got frame\n");
}

void FCReceiver::gotFrame()
{
    cond.notify_all();
}

void FCReceiver::init(unsigned src_id,
                      unsigned vid,
                      unsigned width,
                      unsigned height,
                      unsigned colorbit,
                      ColorType colorType,
                      unsigned local_port_id,
                      unsigned syn_flag,
                      unsigned posx,
                      unsigned posy)
{

    if (colorbit != 16)
    {
        printf("fc color bit error.\n");
        return;
    }
    if (colorType != FC_AV_RGB)
    {
        printf("fc color type error.\n");
        return;
    }
    printf("rw_init: %d\n", rw_Initial());

    printf("av_init: %d\n", av_tx_init());

    printf("FC rows=%d, cols=%d\n", height, width);

    this->src_id = src_id;
    this->vid = vid;
    this->height = height;
    this->width = width;
    this->colorbit = colorbit;
    this->colorType = colorType;

    long rxBuf_ptr = (long)rxBuf;
    rxBuf = map_txbuf(height * width * colorbit / 8);
    if (!rxBuf)
    {
        printf("rxBuf mmap error!!!\n");
        return;
    }
    else
    {
        printf("allocate avrx mem %p\n", rxBuf);
    }
    usleep(125000);
}

void FCReceiver::stop()
{
    munmap(rxBuf, height * width * colorbit / 8);
    rxBuf = NULL;
    cond.notify_all();
}

void FCReceiver::start()
{
    if (!rxBuf)
    {
        printf("rxBuf is NULL!!!\n");
        return;
    }

    rgbBuf = new unsigned char[height * width * 2];
    std::thread fc_thread(recvFrame);
    fc_thread.detach();

    usleep(125000);
}

void FCReceiver::recvFrame()
{
    char *grp_ids = getenv("FP_FC_GRPID");
    char *vids = getenv("FP_FC_GRPID");

    unsigned grp_id = myatoi(grp_ids);
    FCReceiver::getInstance()->vid = myatoi(vids);

    FCReceiver::getInstance()->local_port_id = getNPortId(0);
    
    av_rx_ctrl(FCReceiver::getInstance()->vid, 1,
               FCReceiver::getInstance()->src_id,
               FCReceiver::getInstance()->posx,
               FCReceiver::getInstance()->posy,
               FCReceiver::getInstance()->width,
               FCReceiver::getInstance()->height,
               FCReceiver::getInstance()->colorbit +
                   (int)(FCReceiver::getInstance()->colorType) * 0xffff +
                //    FCReceiver::getInstance()->local_port_id * 0x80000000 +
                   FCReceiver::getInstance()->syn_flag,
               FCReceiver::getInstance()->rxBuf);

    int res_sig = add_avrx_handler(sig_handler, -1);
    printf("group id = 0x%x, vid = %d, res_sig = 0x%x\n", grp_id, FCReceiver::getInstance()->vid, res_sig);
    printf("port id = 0x%x\n", FCReceiver::getInstance()->local_port_id);

    // common_Initial(0,0,0,0,0); //TODO
    common_ExitGrp(grp_id, FCReceiver::getInstance()->local_port_id);
    common_AddGrp(grp_id, FCReceiver::getInstance()->local_port_id);

    write_reg("70", 0xff & grp_id);

    while (FCReceiver::getInstance()->rxBuf)
    {
        std::unique_lock<std::mutex> lock(FCReceiver::getInstance()->mtx);
        FCReceiver::getInstance()->cond.wait(lock);
        {
            std::lock_guard<std::mutex> lock(FCReceiver::getInstance()->mtxCopy);
            memcpy(FCReceiver::getInstance()->rgbBuf, FCReceiver::getInstance()->rxBuf, FCReceiver::getInstance()->height * FCReceiver::getInstance()->width * 2);
        }
    }
    printf("rgbBuf release\n");
    delete[] FCReceiver::getInstance()->rgbBuf;
}

unsigned char *FCReceiver::getDirectBuffer()
{
    return rgbBuf;
}

// maybe dangerous
void FCReceiver::getFrame(unsigned char *buf)
{
    if (!rgbBuf)
    {
        printf("rgbBuf is NULL!!!\n");
        return;
    }
    std::lock_guard<std::mutex> lock(mtxCopy);
    memcpy(buf, rgbBuf, height * width * 2);
}