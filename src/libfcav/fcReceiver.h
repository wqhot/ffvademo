#pragma once
#include <stdlib.h>
#include <mutex>
#include <condition_variable>

class FCReceiver
{
public:
    typedef enum
    {
        FC_AV_RGB = 0,
        FC_AV_YUV,
    } ColorType;
    void init(unsigned src_id, 
              unsigned vid, 
              unsigned width, 
              unsigned height, 
              unsigned colorbit, 
              ColorType colorType,
              unsigned local_port_id = 0,
              unsigned syn_flag = 0x40000000,
              unsigned posx = 0,
              unsigned posy = 0);
    void start();
    void stop();
    void gotFrame();
    void getFrame(unsigned char *buf);
    unsigned char *getDirectBuffer();
    static FCReceiver *getInstance();

private:
    unsigned src_id;
    unsigned vid;
    unsigned height;
    unsigned width;
    unsigned colorbit;
    unsigned local_port_id;
    unsigned syn_flag;
    unsigned posx;
    unsigned posy;
    ColorType colorType;
    unsigned char *rxBuf;
    std::mutex mtx;
    std::condition_variable cond;
    std::mutex mtxCopy;
    unsigned char *rgbBuf;

    static void recvFrame();
};