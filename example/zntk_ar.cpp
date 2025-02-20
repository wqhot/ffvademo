#include <vafastplayer.h>
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
    return 0;
}