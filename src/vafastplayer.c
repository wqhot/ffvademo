/*
 * ffvademo.c - FFmpeg/vaapi demo program
 *
 * Copyright (C) 2014 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301
 */

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include <getopt.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <va/va_drmcommon.h>
#include "ffvadisplay.h"
#include "ffvadecoder.h"
#include "ffvafilter.h"
#include "ffvarenderer.h"
#include "ffmpeg_utils.h"
#include "vaapi_utils.h"

#if USE_DRM
# include "ffvarenderer_drm.h"
#endif
#if USE_X11
# include "ffvarenderer_x11.h"
#endif
#if USE_EGL
# include "ffvarenderer_egl.h"
#endif
#define OUTTHREAD
// #ifdef OUTTHREAD
#include <threads.h>
#include <pthread.h>
#include <ctype.h>
// #endif
#include "vafastplayer.h"

// #ifdef  OUTTHREAD
mtx_t                   mutex;
static int quit_flag=0;
static int output_thread_run=1;
pthread_t *output_thread = NULL;
// #endif
#define MAX_LINE_LENGTH 200

// Default window size
#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

// Default renderer
// #if USE_DRM
// #define DEFAULT_RENDERER FFVA_RENDERER_TYPE_DRM
// #endif
// #if USE_X11
// #define DEFAULT_RENDERER FFVA_RENDERER_TYPE_X11
// #endif
// #ifndef DEFAULT_RENDERER
// #define DEFAULT_RENDERER FFVA_RENDERER_TYPE_EGL
// #endif
#define DEFAULT_RENDERER FFVA_RENDERER_TYPE_EGL

// Default memory type
#define DEFAULT_MEM_TYPE MEM_TYPE_DMA_BUF

typedef enum {
    MEM_TYPE_DMA_BUF = 1,
    MEM_TYPE_GEM_BUF,
    MEM_TYPE_MESA_IMAGE,
    MEM_TYPE_MESA_TEXTURE,
} MemType;

typedef struct {
    char *filename;
    FFVARendererType renderer_type;
    uint32_t mem_type;
    enum AVPixelFormat pix_fmt;
    int list_pix_fmts;
    uint32_t window_width;
    uint32_t window_height;
} OptionsApp;

typedef struct {
    float x0, y0, x1, y1;
} CropRect;

typedef struct {
    const void *klass;
    OptionsApp options;
    FFVADisplay *display;
    VADisplay va_display;
    FFVADecoder *decoder;
    FFVAFilter *filter;
    uint32_t filter_chroma;
    uint32_t filter_fourcc;
    FFVASurface filter_surface;
    FFVARenderer *renderer;
    uint32_t renderer_width;
    uint32_t renderer_height;
    uint32_t quit_flag;
    CropRect crop_rect;
} App;

#define OFFSET(x) offsetof(App, options.x)
static const AVOption app_options[] = {
    { "filename", "path to video file to decode", OFFSET(filename),
      AV_OPT_TYPE_STRING, },
    { "window_width", "window width", OFFSET(window_width),
      AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 4096 },
    { "window_height", "window height", OFFSET(window_height),
      AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 4096 },
    { "renderer", "renderer type to use", OFFSET(renderer_type),
      AV_OPT_TYPE_FLAGS, { .i64 = DEFAULT_RENDERER }, 0, INT_MAX, 0,
      "renderer" },
    { "drm", "DRM", 0, AV_OPT_TYPE_CONST, { .i64 = FFVA_RENDERER_TYPE_DRM },
      0, 0, 0, "renderer" },
    { "x11", "X11", 0, AV_OPT_TYPE_CONST, { .i64 = FFVA_RENDERER_TYPE_X11 },
      0, 0, 0, "renderer" },
    { "egl", "EGL", 0, AV_OPT_TYPE_CONST, { .i64 = FFVA_RENDERER_TYPE_EGL },
      0, 0, 0, "renderer" },
    { "mem_type", "memory type for VA buffer exports", OFFSET(mem_type),
      AV_OPT_TYPE_FLAGS, { .i64 = 0}, 0, UINT_MAX, 0, "mem_type" },
    { "dma_buf", "DMA buffer handle", 0, AV_OPT_TYPE_CONST,
      { .i64 = MEM_TYPE_DMA_BUF }, 0, 0, 0, "mem_type" },
    { "gem_buf", "GEM buffer handle", 0, AV_OPT_TYPE_CONST,
      { .i64 = MEM_TYPE_GEM_BUF }, 0, 0, 0, "mem_type" },
    { "mesa_image", "EGLImage exported from Mesa", 0, AV_OPT_TYPE_CONST,
      { .i64 = MEM_TYPE_MESA_IMAGE }, 0, 0, 0, "mem_type" },
    { "mesa_texture", "GL texture exported from Mesa", 0, AV_OPT_TYPE_CONST,
      { .i64 = MEM_TYPE_MESA_TEXTURE }, 0, 0, 0, "mem_type" },
    { "pix_fmt", "output pixel format", OFFSET(pix_fmt),
      AV_OPT_TYPE_PIXEL_FMT, { .i64 = AV_PIX_FMT_NONE }, -1, AV_PIX_FMT_NB-1, },
    { "list_pix_fmts", "list output pixel formats", OFFSET(list_pix_fmts),
      AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, },
    { NULL, }
};

static void
app_free(App *app);

char* getInput(char line[], size_t len)
{
   printf ("--------按q退出程序-------\n");
   return fgets(line, len, stdin);
}

static const char *
get_basename(const char *filename)
{
    const char * const s = strrchr(filename, '/');

    return s ? s + 1 : filename;
}

static void
print_help(const char *prog)
{
    printf("Usage: %s <video>\n", get_basename(prog));
    printf("\n");
    printf("Options:\n");
    printf("  %-28s  display this help and exit\n",
           "-h, --help");
    printf("  %-28s  window width (int) [default=0]\n",
           "-x, --window-width=WIDTH");
    printf("  %-28s  window height (int) [default=0]\n",
           "-y, --window-height=HEIGHT");
    printf("  %-28s  select a particular renderer (string) [default='x11']\n",
           "-r, --renderer=TYPE");
    printf("  %-28s  VA buffer export memory type (string) [default='auto']\n",
           "-m, --mem-type=TYPE");
    printf("  %-28s  output pixel format (AVPixelFormat) [default=none]\n",
           "-f, --format=FORMAT");
    printf("  %-28s  list output pixel formats\n",
           "    --list-formats");
}

static const AVClass *
app_class(void)
{
    static const AVClass g_class = {
        .class_name     = "FFVADemo",
        .item_name      = av_default_item_name,
        .option         = app_options,
        .version        = LIBAVUTIL_VERSION_INT,
    };
    return &g_class;
}

static App *
app_new(void)
{
    App *app;

    app = calloc(1, sizeof(*app));
    if (!app)
        return NULL;

    app->crop_rect = (CropRect) {
        .x0 = 0.0f,
        .y0 = 0.0f,
        .x1 = 1.0f,
        .y1 = 1.0f,
    };
    app->klass = app_class();
    av_opt_set_defaults(app);
    ffva_surface_init_defaults(&app->filter_surface);
    return app;
}

static void
app_free(App *app)
{
    if (!app)
        return;

    ffva_renderer_freep(&app->renderer);
    va_destroy_surface(app->va_display, &app->filter_surface.id);
    ffva_filter_freep(&app->filter);
    ffva_decoder_freep(&app->decoder);
    ffva_display_freep(&app->display);
    av_opt_free(app);
    free(app);
}

static bool
app_ensure_display(App *app)
{
    if (!app->display) {
        app->display = ffva_display_new(NULL);
        if (!app->display)
            goto error_create_display;
        app->va_display = ffva_display_get_va_display(app->display);
        av_log(NULL, AV_LOG_ERROR, "app_ensure_display display  : %x    \n", app->va_display);
    }
    return true;

    /* ERRORS */
error_create_display:
    av_log(app, AV_LOG_ERROR, "failed to create VA display\n");
    return false;
}

static bool
app_ensure_decoder(App *app)
{
    if (!app->decoder) {
        app->decoder = ffva_decoder_new(app->display);
        if (!app->decoder)
            goto error_create_decoder;
    }
    return true;

    /* ERRORS */
error_create_decoder:
    av_log(app, AV_LOG_ERROR, "failed to create FFmpeg/vaapi decoder\n");
    return false;
}

static bool
app_ensure_filter(App *app)
{
    const OptionsApp * const options = &app->options;
    const int *formats, *format = NULL;

    if (!app->filter) {
        app->filter = ffva_filter_new(app->display);
        if (!app->filter)
            goto error_create_filter;
    }

    if (options->pix_fmt == AV_PIX_FMT_NONE)
        return true;

    formats = ffva_filter_get_formats(app->filter);
    if (formats) {
        for (format = formats; *format != AV_PIX_FMT_NONE; format++) {
            if (*format == options->pix_fmt)
                break;
        }
    }
    if (!format || *format == AV_PIX_FMT_NONE)
        goto error_unsupported_format;

    if (!ffmpeg_to_vaapi_pix_fmt(options->pix_fmt, &app->filter_fourcc,
            &app->filter_chroma))
        goto error_unsupported_format;
    return true;

    /* ERRORS */
error_create_filter:
    av_log(app, AV_LOG_ERROR, "failed to create video processing pipeline\n");
    return false;
error_unsupported_format:
    av_log(app, AV_LOG_ERROR, "unsupported output format %s   , options->pix_fmt : %d  \n",
        av_get_pix_fmt_name(options->pix_fmt), options->pix_fmt);
    return false;
}

static bool
app_ensure_filter_surface(App *app, uint32_t width, uint32_t height)
{
    FFVASurface * const s = &app->filter_surface;
    VASurfaceID va_surface;
    VASurfaceAttrib attrib;
    VAStatus va_status;

    if (!app->filter)
        return true; // VPP not needed (checked in app_ensure_filter())

    if (width == s->width && height == s->height)
        return true;

    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = app->filter_fourcc;

    va_destroy_surface(app->va_display, &s->id);
    av_log(NULL, AV_LOG_ERROR, "filter vaCreateSurfaces  : %x    \n", app->va_display);
    va_status = vaCreateSurfaces(app->va_display, app->filter_chroma,
        width, height, &va_surface, 1, &attrib, 1);
    av_log(NULL, AV_LOG_ERROR, "app_ensure_filter_surface va_surface : %d    \n", va_surface);
    if (!va_check_status(va_status, "vaCreateSurfaces()"))
        return false;

    ffva_surface_init(s, va_surface, app->filter_chroma, width, height);
    s->fourcc = app->filter_fourcc;
    return true;
}

static bool
app_ensure_renderer(App *app)
{
    const OptionsApp * const options = &app->options;
    uint32_t flags = 0;

    if (!app->renderer) {
        switch (options->renderer_type) {
#if USE_DRM
        case FFVA_RENDERER_TYPE_DRM:
            app->renderer = ffva_renderer_drm_new(app->display, flags);
            break;
#endif
#if USE_X11
        case FFVA_RENDERER_TYPE_X11:
            app->renderer = ffva_renderer_x11_new(app->display, flags);
            break;
#endif
#if USE_EGL
        case FFVA_RENDERER_TYPE_EGL:
            switch (options->mem_type) {
            case MEM_TYPE_DMA_BUF:
                flags |= FFVA_RENDERER_EGL_MEM_TYPE_DMA_BUFFER;
                break;
            case MEM_TYPE_GEM_BUF:
                flags |= FFVA_RENDERER_EGL_MEM_TYPE_GEM_BUFFER;
                break;
            case MEM_TYPE_MESA_IMAGE:
                flags |= FFVA_RENDERER_EGL_MEM_TYPE_MESA_IMAGE;
                break;
            case MEM_TYPE_MESA_TEXTURE:
                flags |= FFVA_RENDERER_EGL_MEM_TYPE_MESA_TEXTURE;
                break;
            }
            app->renderer = ffva_renderer_egl_new(app->display, flags);
            break;
#endif
        default:
            break;
        }
        if (!app->renderer)
            goto error_create_renderer;
    }
    return true;

    /* ERRORS */
error_create_renderer:
    av_log(app, AV_LOG_ERROR, "failed to create renderer\n");
    return false;
}

static bool
app_ensure_renderer_size(App *app, uint32_t width, uint32_t height)
{
    if (!app_ensure_renderer(app))
        return false;

    if (app->renderer_width != width || app->renderer_height != height) {
        if (!ffva_renderer_set_size(app->renderer, width, height))
            return false;
        app->renderer_width = width;
        app->renderer_height = height;
    }
    return true;
}

static bool
app_process_surface(App *app, FFVASurface *s, const VARectangle *rect,
    uint32_t flags)
{
    FFVASurface * const d = &app->filter_surface;

    if (!app_ensure_filter_surface(app, s->width, s->height))
        return false;

    if (ffva_filter_set_cropping_rectangle(app->filter, rect) < 0)
        return false;
    // av_log(NULL, AV_LOG_ERROR, "app_process_surface-----------------\n");
    if (ffva_filter_process(app->filter, s, d, flags) < 0)
        return false;
    return true;
}

static bool
app_render_surface(App *app, FFVASurface *s, const VARectangle *rect,
    uint32_t flags)
{
    const OptionsApp * const options = &app->options;
    uint32_t renderer_width, renderer_height;

    renderer_width = options->window_width ? options->window_width :
        rect->width;
    renderer_height = options->window_height ? options->window_height :
        rect->height;
    if (!app_ensure_renderer_size(app, renderer_width, renderer_height))
        return false;

    if (app->filter) {
        if (!app_process_surface(app, s, rect, flags))
            return false;

        // drop deinterlacing, color standard and scaling flags
        flags &= ~(VA_TOP_FIELD|VA_BOTTOM_FIELD|0xf0|VA_FILTER_SCALING_MASK);

        return ffva_renderer_put_surface(app->renderer, &app->filter_surface,
            NULL, NULL, flags);
    }
    return ffva_renderer_put_surface(app->renderer, s, rect, NULL, flags);
}

static int
app_renderer_add_image(App *app, const char *image_path, float x, float y, float scale, float rotation)
{
    return ffva_renderer_load_image(app->renderer, image_path, x, y, scale, rotation);
}

static int
app_renderer_add_text(App *app, const char *font_path, const char *text_context, int font_size, float x, float y)
{
    return ffva_renderer_load_text(app->renderer, font_path, text_context, font_size, x, y);
}

static bool
app_renderer_adjust_image(App *app, int image_id, float x, float y, float scale, float rotation)
{
    return ffva_renderer_adjust_image(app->renderer, image_id, x, y, scale, rotation);
}

static void
app_renderer_set_center(App *app, float x, float y)
{
    return ffva_renderer_set_center(app->renderer, x, y);
}

static int
app_render_frame(App *app, FFVADecoderFrame *dec_frame)
{
    FFVASurface * const s = dec_frame->surface;
    AVFrame * const frame = dec_frame->frame;
    const VARectangle *rect;
    VARectangle tmp_rect;
    uint32_t i, flags;

    if (dec_frame->has_crop_rect)
        rect = &dec_frame->crop_rect;
    else {
        tmp_rect.x = app->crop_rect.x0 * s->width;
        tmp_rect.y = app->crop_rect.y0 * s->height;
        tmp_rect.width = (app->crop_rect.x1 - app->crop_rect.x0) * s->width;
        tmp_rect.height = (app->crop_rect.y1 - app->crop_rect.y0) * s->height;
        rect = &tmp_rect;
    }

    flags = 0;
    for (i = 0; i < 1 + !!frame->interlaced_frame; i++) {
        flags &= ~(VA_TOP_FIELD|VA_BOTTOM_FIELD);
        if (frame->interlaced_frame) {
            flags |= ((i == 0) ^ !!frame->top_field_first) == 0 ?
                VA_TOP_FIELD : VA_BOTTOM_FIELD;
        }
        if (!app_render_surface(app, s, rect, flags))
            return AVERROR_UNKNOWN;
    }
    return 0;
}

static bool
app_render_widow_close(App *app)
{
    return ffva_render_get_signal_window_close(app->renderer);
}

static int
app_decode_frame(App *app)
{
    FFVADecoderFrame *dec_frame;
    int ret;

    ret = ffva_decoder_get_frame(app->decoder, &dec_frame);
    if (ret == 0) {
        ret = app_render_frame(app, dec_frame);
        ffva_decoder_put_frame(app->decoder, dec_frame);
		/*show too fast, use 25fps instead*/
		// usleep(40000);
    }
    return ret;
}

static bool
app_list_formats(App *app)
{
    const int *formats;
    int i;

    if (!app_ensure_display(app))
        return false;
    if (!app_ensure_filter(app))
        return false;

    formats = ffva_filter_get_formats(app->filter);
    if (!formats)
        return false;

    printf("List of supported output pixel formats:");
    for (i = 0; formats[i] != AV_PIX_FMT_NONE; i++) {
        if (i > 0)
            printf(",");
        printf(" %s", av_get_pix_fmt_name(formats[i]));
    }
    printf("\n");
    return true;
}

static bool
app_list_info(App *app)
{
    const OptionsApp * const options = &app->options;
    bool list_info = false;

    if (options->list_pix_fmts) {
        app_list_formats(app);
        list_info = true;
    }
    return list_info;
}
#if 0
static void* wrapper_output_thread(void*arg)
{
    bool ret=false;
    App *app=(App *)arg;
    ret=app_render_widow_close(app);
    if(ret==true)
    {
      //  mtx_lock(&mutex);
        app->quit_flag=1;
      //  mtx_unlock(&mutex);
    }
}
#endif

#if 1
static void* wrapper_output_thread(void*arg)
{
    App *app = (App*)arg;
    char errbuf[BUFSIZ];
    int ret;
    do {
        ret = app_decode_frame(app);
        ret = 0;
        if(app->quit_flag==1){
            printf("receive quit signal \n");
            break;
        }
    } while (ret == 0 || ret == AVERROR(EAGAIN));
    if (ret != AVERROR_EOF)
        goto error_decode_frame;
    ffva_decoder_stop(app->decoder);
    ffva_decoder_close(app->decoder);
    return true;

    /* ERRORS */
error_decode_frame:
    av_log(app, AV_LOG_ERROR, "failed to decode frame: %s\n",
        ffmpeg_strerror(ret, errbuf));
}

#endif
static bool
app_run(App *app)
{
    const OptionsApp * const options = &app->options;
    FFVADecoderInfo info;
    bool need_filter;
    char errbuf[BUFSIZ];
    int ret;

    if (app_list_info(app))
        return true;

    if (!options->filename)
        goto error_no_filename;

    need_filter = options->pix_fmt != AV_PIX_FMT_NONE;

    if (!app_ensure_display(app))
        return false;
    if (need_filter && !app_ensure_filter(app))
        return false;
    if (!app_ensure_renderer(app))
        return false;
    if (!app_ensure_decoder(app))
        return false;
    if (!app_ensure_renderer(app))
        return false;

    if (ffva_decoder_open(app->decoder, options->filename) < 0)
        return false;
    if (ffva_decoder_start(app->decoder) < 0)
        return false;

    if (!ffva_decoder_get_info(app->decoder, &info))
        return false;
#ifdef OUTTHREAD
       if (!output_thread) {
            output_thread_run = 1;
            output_thread = (pthread_t *)calloc(1, sizeof(pthread_t));
            if(output_thread==NULL){
                printf("out_thread is null !----\n");
                return false;
            }
            pthread_create(output_thread, NULL, wrapper_output_thread, app);
          }
#endif

    return true;

    /* ERRORS */
error_no_filename:
    av_log(app, AV_LOG_ERROR, "no video file specified on command line\n");
    return false;
}

static bool
app_parse_options(App *app, struct Options *options)
{
    OptionsApp *opt = &(app->options);
    opt->filename = options->filename;
    opt->window_width = options->window_width;
    opt->window_height = options->window_height;
    opt->mem_type = MEM_TYPE_DMA_BUF;
    opt->pix_fmt = AV_PIX_FMT_NONE;
    opt->list_pix_fmts = 0;
    return true;
}

int vafastplayer_add_image(Fastplayer player, char *image_path, float x, float y, float scale, float rotation)
{
    App *app = (App *)player;
    return app_renderer_add_image(app, image_path, x, y, scale, rotation);
}

int vafastplayer_add_text(Fastplayer player, const char* font, const char *text, int font_size, float x, float y)
{
    App *app = (App *)player;
    return app_renderer_add_text(app, font, text, font_size, x, y);
}

bool vafastplayer_adjust_image(Fastplayer player, int image_id, float x, float y, float scale, float rotation)
{
    App *app = (App *)player;
    return app_renderer_adjust_image(app, image_id, x, y, scale, rotation);
}

void vafastplayer_crop_video(Fastplayer player, float crop_start_x_u, float crop_start_y_u, float crop_end_x_u, float crop_end_y_u)
{
    App *app = (App *)player;
    float temp_y0 = crop_start_y_u;
    float temp_y1 = crop_end_y_u;
    crop_start_y_u = 1.0f - temp_y1;
    crop_end_y_u = 1.0f - temp_y0;
    if (crop_start_x_u >= 0 && crop_start_y_u >= 0 && crop_end_x_u > crop_start_x_u && crop_end_y_u > crop_start_y_u && crop_end_x_u <= 1 && crop_end_y_u <= 1)
    {
        app->crop_rect.x0 = crop_start_x_u;
        app->crop_rect.y0 = crop_start_y_u;
        app->crop_rect.x1 = crop_end_x_u;
        app->crop_rect.y1 = crop_end_y_u;
    }
}

void vafastplayer_set_center(Fastplayer player, float x_u, float y_u)
{
    App *app = (App *)player;
    app_renderer_set_center(app, x_u, y_u);
}

Fastplayer vafastplayer_init(struct Options *opts)
{
    App *app;
    int ret = EXIT_FAILURE;

    app = app_new();


    if (!app || !app_parse_options(app, opts))
    {
        app_free(app);
        return NULL;
    }
    return (void *)app;
}

int vafastplayer_start(Fastplayer player)
{
    App *app = (App *)player;
    int ret = -1;
    if (!app || !app_run(app))
        goto cleanup;
    ret = 0;
    return ret;
cleanup:
    app_free(app);
    ret = -1;
    return ret;
}

int vafastplayer_stop(Fastplayer player)
{
    App *app = (App *)player;
#ifdef OUTTHREAD
    if(output_thread) {
        pthread_join(*output_thread, NULL);
        free(output_thread);
        output_thread = NULL;
    }
#endif
    return 0;
}

// int
// main(int argc, char *argv[])
// {
//     Options opt;
//     opt.filename = argv[1];
//     opt.window_width = 1280;
//     opt.window_height = 720;
//     Fastplayer player = vafastplayer_init(&opt);
//     if (!player) {
//         fprintf(stderr, "Failed to initialize application\n");
//     }
//     if (vafastplayer_start(player) != 0)
//     {
//         fprintf(stderr, "Failed to run application\n");
//     }
//     printf("start__\n");
//     return 0;
// }
