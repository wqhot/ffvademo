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
} Options;


FASTPLAYERAPI
Fastplayer vafastplayer_init(struct Options *opts);

FASTPLAYERAPI
int vafastplayer_add_image(Fastplayer player, char *image_path, float x, float y, float scale, float rotation);

FASTPLAYERAPI
int vafastplayer_add_text(Fastplayer player, const char* font, const char *text, int font_size, float x, float y);

FASTPLAYERAPI
bool vafastplayer_adjust_image(Fastplayer player, int image_id, float x, float y, float scale, float rotation);

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