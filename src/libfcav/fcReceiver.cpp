#include "fcReceiver.h"
#include "ulibuser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <thread>

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

    av_rx_ctrl(vid, 1, src_id, 
               posx, posy, width, height, 
               colorbit + (int)colorType * 0xffff + local_port_id * 0x80000000 + syn_flag,
               rxBuf);

    rgbBuf = new unsigned char[height * width * 2];
    std::thread fc_thread(recvFrame);
    fc_thread.detach();

    int res_sig = add_avrx_handler(sig_handler, vid);
    usleep(125000);
}

void FCReceiver::recvFrame()
{   
    av_rx_ctrl(FCReceiver::getInstance()->vid, 1, 
               FCReceiver::getInstance()->src_id, 
               FCReceiver::getInstance()->posx, 
               FCReceiver::getInstance()->posy, 
               FCReceiver::getInstance()->width, 
               FCReceiver::getInstance()->height, 
               FCReceiver::getInstance()->colorbit + 
               (int)(FCReceiver::getInstance()->colorType) * 0xffff + 
               FCReceiver::getInstance()->local_port_id * 0x80000000 + 
               FCReceiver::getInstance()->syn_flag,
               FCReceiver::getInstance()->rxBuf);
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