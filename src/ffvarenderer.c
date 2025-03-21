/*
 * ffvarenderer.c - VA renderer abstraction
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

#include "sysdeps.h"
#include "ffvarenderer.h"
#include "ffvarenderer_priv.h"

FFVARenderer *
ffva_renderer_new(const FFVARendererClass *klass, FFVADisplay *display,
    uint32_t flags)
{
    FFVARenderer *rnd;

    rnd = calloc(1, klass->size ? klass->size : sizeof(*rnd));
    if (!rnd)
        return NULL;

    rnd->klass = klass;
    rnd->display = display;
    if (klass->init && !klass->init(rnd, flags))
        goto error;
    return rnd;

error:
    ffva_renderer_free(rnd);
    return NULL;
}

// Releases all renderer resources
void
ffva_renderer_free(FFVARenderer *rnd)
{
    FFVARendererClass *klass;

    if (!rnd)
        return;

    klass = FFVA_RENDERER_GET_CLASS(rnd);
    if (klass->finalize)
        klass->finalize(rnd);
    free(rnd);
}

// Releases renderer object and resets the supplied pointer to NULL
void
ffva_renderer_freep(FFVARenderer **rnd_ptr)
{
    if (!rnd_ptr)
        return;
    ffva_renderer_free(*rnd_ptr);
    *rnd_ptr = NULL;
}

// Returns the type of the supplied renderer
FFVARendererType
ffva_renderer_get_type(FFVARenderer *rnd)
{
    if (!rnd)
        return 0;
    return FFVA_RENDERER_GET_CLASS(rnd)->type;
}

// Returns the size of the rendering device
bool
ffva_renderer_get_size(FFVARenderer *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr)
{
    FFVARendererClass *klass;

    if (!rnd)
        return false;

    klass = FFVA_RENDERER_GET_CLASS(rnd);
    if (klass->get_size && !klass->get_size(rnd, &rnd->width, &rnd->height))
        return false;

    if (width_ptr)
        *width_ptr = rnd->width;
    if (height_ptr)
        *height_ptr = rnd->height;
    return true;
}

// Resizes the rendering device to the supplied dimensions
bool
ffva_renderer_set_size(FFVARenderer *rnd, uint32_t width, uint32_t height)
{
    FFVARendererClass *klass;

    if (!rnd || !width || !height)
        return false;
    av_log(rnd, AV_LOG_ERROR, "ffva_renderer_set_size ------------------------------\n");
    klass = FFVA_RENDERER_GET_CLASS(rnd);
    return klass->set_size ? klass->set_size(rnd, width, height) : true;
}

// Submits the supplied surface to the rendering device
bool
ffva_renderer_put_surface(FFVARenderer *rnd, FFVASurface *surface,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags)
{
    FFVARendererClass *klass;
    VARectangle src_rect_tmp, dst_rect_tmp;
    uint32_t width, height;

    if (!rnd || !surface || surface->id == VA_INVALID_ID)
        return false;

    if (!src_rect) {
        src_rect_tmp.x = 0;
        src_rect_tmp.y = 0;
        src_rect_tmp.width = surface->width;
        src_rect_tmp.height = surface->height;
        src_rect = &src_rect_tmp;
    }

    if (!dst_rect) {
        if (!ffva_renderer_get_size(rnd, &width, &height))
            return false;
        dst_rect_tmp.x = 0;
        dst_rect_tmp.y = 0;
        dst_rect_tmp.width = width;
        dst_rect_tmp.height = height;
        dst_rect = &dst_rect_tmp;
    }

    klass = FFVA_RENDERER_GET_CLASS(rnd);
    return klass->put_surface ? klass->put_surface(rnd, surface, src_rect,
        dst_rect, flags) : true;
}

// Returns the native display associated to the supplied renderer
void *
ffva_renderer_get_native_display(FFVARenderer *rnd)
{
    if (!rnd || !rnd->display)
        return NULL;
    return ffva_display_get_native_display(rnd->display);
}

// Returns the native window associated to the supplied renderer
void *
ffva_renderer_get_native_window(FFVARenderer *rnd)
{
    if (!rnd)
        return NULL;
    return rnd->window;
}

uintptr_t
ffva_renderer_get_visual_id(FFVARenderer *rnd)
{
    FFVARendererClass *klass;

    if (!rnd)
        return 0;

    klass = FFVA_RENDERER_GET_CLASS(rnd);
    return klass->get_visual_id ? klass->get_visual_id(rnd) : 0;
}

bool
ffva_render_get_signal_window_close(FFVARenderer *rnd){
     FFVARendererClass *klass;
     if (!rnd)
         return 0;
     klass=FFVA_RENDERER_GET_CLASS(rnd);
     return klass->widow_close(rnd);
}

int ffva_renderer_load_image(FFVARenderer *rnd, const char *image_path, float x, float y, float scale, float rotation)
{
    FFVARendererClass *klass;
    if (!rnd)
        return -1;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    return klass->renderer_load_image(rnd, image_path, x, y, scale, rotation);
}

int ffva_renderer_add_image_data(FFVARenderer *rnd, int width, int height, unsigned char *data)
{
    FFVARendererClass *klass;
    if (!rnd)
        return -1;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    return klass->renderer_add_image_data(rnd, width, height, data);
}

int ffva_renderer_add_fc(FFVARenderer *rnd, unsigned src_id, 
    unsigned vid, 
    unsigned width, 
    unsigned height, 
    unsigned colorbit, 
    int colorType,
    unsigned local_port_id,
    unsigned syn_flag,
    unsigned posx,
    unsigned posy)
{
    FFVARendererClass *klass;
    if (!rnd)
        return -1;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    return klass->renderer_add_fc(rnd, src_id, vid, width, height, colorbit, colorType, local_port_id, syn_flag, posx, posy);
}

int ffva_renderer_load_text(FFVARenderer *rnd, const char *font_path, const char *text_context, int font_size, float x, float y)
{
    FFVARendererClass *klass;
    if (!rnd)
        return -1;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    return klass->renderer_load_text(rnd, font_path, text_context, font_size, x, y);
}

bool ffva_renderer_adjust_image(FFVARenderer *rnd, int image_id, float x, float y, float scale, float rotation)
{
    FFVARendererClass *klass;
    if (!rnd)
        return false;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    return klass->renderer_adjust_image(rnd, image_id, x, y, scale, rotation);
}

void ffva_renderer_set_center(FFVARenderer *rnd, float x, float y)
{
    FFVARendererClass *klass;
    if (!rnd)
        return;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    klass->renderer_set_center(rnd, x, y);
    return;
}

void ffva_renderer_resize(FFVARenderer *rnd, int x, int y, uint32_t width, uint32_t height)
{
    FFVARendererClass *klass;
    if (!rnd)
        return;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    klass->renderer_resize(rnd, x, y, width, height);
    return;
}

void ffva_renderer_update_image(FFVARenderer *rnd, int image_id, unsigned char *data)
{
    FFVARendererClass *klass;
    if (!rnd)
        return;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    klass->renderer_update_image(rnd, image_id, data);
    return;
}

bool ffva_renderer_set_always_on_top(FFVARenderer *rnd, bool enable)
{
    FFVARendererClass *klass;
    if (!rnd)
        return false;
    klass=FFVA_RENDERER_GET_CLASS(rnd);
    if (!klass)
    {
        printf("Error: get kclass\n");
    }
    return klass->renderer_set_always_above(rnd, enable);
}