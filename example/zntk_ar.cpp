#include <vafastplayer.h>
#include <chrono>
#include <thread>
int main(int argc, char *argv[])
{
    Options opt, opt2;
    opt.filename = argv[1];
    opt.window_width = 1280;
    opt.window_height = 720;
    opt2.filename = argv[2];
    opt2.window_width = 1280;
    opt2.window_height = 720;
    Fastplayer player = vafastplayer_init(&opt);
    // Fastplayer player2 = vafastplayer_init(&opt2);
    if (!player) {
        fprintf(stderr, "Failed to initialize application\n");
    }
    if (vafastplayer_start(player) != 0)
    {
        fprintf(stderr, "Failed to run application\n");
    }
    // if (vafastplayer_start(player2) != 0)
    // {
    //     fprintf(stderr, "Failed to run application\n");
    // }
    printf("start__\n");
    int focus = vafastplayer_add_image(player, "/usr/share/fastplayer/focus.png", 0.5, 0.5, 1.0, 3.14/4);
    int text = vafastplayer_add_text(player, "/usr/share/fastplayer/source.otf", "IHello World!\n计算所", 48, 0.2, 0.2);
    int turret = vafastplayer_add_image(player, "/usr/share/fastplayer/tank_turret.png", 0.8, 0.8, 0.8, 3.14/7);
    vafastplayer_set_center(player, 0.2, 0.7);
    int count = 0;
    printf("focus = %d, text = %d, turret = %d\n", focus, text, turret);
    float rad = 3.14/4;
    while (1)
    {
        bool ret = vafastplayer_adjust_image(player, focus, 0.5, 0.5, 1.0, rad);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        rad = rad + 0.01;
        // if (count++ == 120)
        // {
        //     vafastplayer_crop_video(player, 0.1, 0.1, 0.8, 0.5);
        // }
    }

    vafastplayer_stop(player);
    // vafastplayer_stop(player2);
    return 0;
}