#include <vafastplayer.h>
#include <chrono>
#include <thread>
#include <fstream>
int main(int argc, char *argv[])
{
    Options opt, opt2;
    opt.filename = argv[1];
    opt.window_width = 1920;
    opt.window_height = 1080;
    opt.window_x = 0;
    opt.window_y = 0;
    opt2.filename = argv[2];
    opt2.window_width = 320;
    opt2.window_height = 180;
    opt2.window_x = 1920 - 320;
    opt2.window_y = 1080 - 180;
    Fastplayer player = vafastplayer_init(&opt);
    Fastplayer player2 = vafastplayer_init(&opt2);
    // 构造一个yuv422 1080p的彩色条纹图像
    unsigned char *data = (unsigned char *)malloc(1920 * 1080 * 4);
    std::ifstream file("output.rgb", std::ios::binary);
    file.read((char *)data, 1920 * 1080 * 2);
    if (!player) {
        fprintf(stderr, "Failed to initialize application\n");
    }
    if (vafastplayer_start(player) != 0)
    {
        fprintf(stderr, "Failed to run application\n");
    }
    if (vafastplayer_start(player2) != 0)
    {
        fprintf(stderr, "Failed to run application\n");
    }
    printf("start__\n");
    
    int yuv = vafastplayer_add_fc(player, 
                                   0x1001a,
                                   10108,
                                    1920,
                                    1080,
                                    16,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0);
    // int yuv = vafastplayer_add_image_data(player, 1920, 1080, data);
    int focus = vafastplayer_add_image(player, "/usr/share/fastplayer/focus.png", 0.5, 0.5, 1.0, 3.14/4);
    int text = vafastplayer_add_text(player, "/usr/share/fastplayer/source.otf", "IHello World!\n计算所", 48, 0.2, 0.2);
    int turret = vafastplayer_add_image(player, "/usr/share/fastplayer/tank_turret.png", 0.8, 0.8, 0.8, 3.14/7);
    vafastplayer_set_scope_center(player, 0.2, 0.7);
    
    int count = 0;
    printf("focus = %d, text = %d, turret = %d\n", focus, text, turret);
    float rad = 3.14/4;
    bool big = true;
    while (1)
    {
        bool ret = vafastplayer_adjust_image(player, focus, 0.5, 0.5, 1.0, rad);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        rad = rad + 0.01;
        
        if (count++ % 360 == 0 && count > 1)
        {
            if (big)
            {
                vafastplayer_set_size(player, 0, 0, 1280, 720);
                vafastplayer_set_size(player2, 1280 - 320, 720 - 180, 320, 180);
                vafastplayer_set_smallbox(player, !big);
                vafastplayer_set_smallbox(player2, big);
            }
            else
            {
                vafastplayer_set_size(player2, 0, 0, 1280, 720);
                vafastplayer_set_size(player, 1280 - 320, 720 - 180, 320, 180);
                vafastplayer_set_smallbox(player, !big);
                vafastplayer_set_smallbox(player2, big);
            }
            big = !big;
        }
    }

    vafastplayer_stop(player);
    vafastplayer_stop(player2);
    return 0;
}