#include <vafastplayer.h>
#include <chrono>
#include <thread>
int main(int argc, char *argv[])
{
    Options opt;
    opt.filename = argv[1];
    opt.window_width = 1280;
    opt.window_height = 720;
    Fastplayer player = vafastplayer_init(&opt);
    if (!player) {
        fprintf(stderr, "Failed to initialize application\n");
    }
    if (vafastplayer_start(player) != 0)
    {
        fprintf(stderr, "Failed to run application\n");
    }
    printf("start__\n");
    int focus = vafastplayer_add_image(player, "/usr/share/fastplayer/focus.png", 0.5, 0.5, 1.0, 3.14/4);
    int text = vafastplayer_add_text(player, "/usr/share/fastplayer/source.otf", "IHello World!\n计算所", 48, 0.2, 0.2);
    int turret = vafastplayer_add_image(player, "/usr/share/fastplayer/tank_turret.png", 0.8, 0.8, 0.8, 3.14/7);
    printf("focus = %d, text = %d, turret = %d\n", focus, text, turret);
    float rad = 3.14/4;
    while (1)
    {
        bool ret = vafastplayer_adjust_image(player, focus, 0.5, 0.5, 1.0, rad);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rad = rad + 0.01;
    }

    vafastplayer_stop(player);
    return 0;
}