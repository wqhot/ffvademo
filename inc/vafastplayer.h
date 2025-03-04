#ifndef LIB_VAFASTPLAYER_H
#define LIB_VAFASTPLAYER_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* Fastplayer;
#define FASTPLAYERAPI __attribute__((visibility("default")))

// 配置结构体（需保留在应用层）
typedef struct Options{
    char *filename;
    uint32_t window_width;
    uint32_t window_height;
    int window_x;
    int window_y;
} Options;


FASTPLAYERAPI
Fastplayer vafastplayer_init(struct Options *opts);

FASTPLAYERAPI
int vafastplayer_add_image(Fastplayer player, char *image_path, float x_u, float y_u, float scale_u, float rotation);

FASTPLAYERAPI
int vafastplayer_add_text(Fastplayer player, const char* font, const char *text, int font_size_p, float x_u, float y_u);

FASTPLAYERAPI
bool vafastplayer_adjust_image(Fastplayer player, int image_id, float x_u, float y_u, float scale, float rotation);

FASTPLAYERAPI
void vafastplayer_crop_video(Fastplayer player, float crop_lb_x_u, float crop_lb_y_u, float crop_rt_x_u, float crop_rt_y_u);

FASTPLAYERAPI
void vafastplayer_set_center(Fastplayer player, float x_u, float y_u);

FASTPLAYERAPI
void vafastplayer_set_size(Fastplayer player, int x_p, int y_p, uint32_t width_p, uint32_t height_p);

FASTPLAYERAPI
void vafastplayer_update_image(Fastplayer player, int image_id, const unsigned char *data);

// FASTPLAYERAPI
// bool vafastplayer_adjust_text(Fastplayer player, int text_id, const char *text, float x, float y);

FASTPLAYERAPI
int vafastplayer_start(Fastplayer player);

FASTPLAYERAPI
int vafastplayer_stop(Fastplayer player);


#ifdef __cplusplus
}
#endif

#endif