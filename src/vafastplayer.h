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

// 配置结构体（需保留在应用层）
typedef struct Options{
    char *filename;
    // FFVARendererType renderer_type;
    // uint32_t mem_type;
    // enum AVPixelFormat pix_fmt;
    // int list_pix_fmts;
    uint32_t window_width;
    uint32_t window_height;
} Options;

// 核心初始化函数
__attribute__((visibility("default"))) 
Fastplayer vafastplayer_init(struct Options *opts);

// 主运行函数 
__attribute__((visibility("default")))
int vafastplayer_start(Fastplayer player);


#ifdef __cplusplus
}
#endif

#endif