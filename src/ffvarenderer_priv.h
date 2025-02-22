/*
 * ffvarenderer_priv.h - VA renderer abstraction (private definitions)
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

#ifndef FFVA_RENDERER_PRIV_H
#define FFVA_RENDERER_PRIV_H

#include "ffvarenderer.h"

#define FFVA_RENDERER_CLASS(klass) \
    ((FFVARendererClass *)(klass))
#define FFVA_RENDERER_GET_CLASS(rnd) \
    FFVA_RENDERER_CLASS(FFVA_RENDERER(rnd)->klass)

typedef struct ffva_renderer_class_s    FFVARendererClass;

typedef bool (*FFVARendererInitFunc)(FFVARenderer *rnd, uint32_t flags);
typedef void (*FFVARendererFinalizeFunc)(FFVARenderer *rnd);
typedef uintptr_t (*FFVARendererGetVisualIdFunc)(FFVARenderer *rnd);
typedef bool (*FFVARendererGetSizeFunc)(FFVARenderer *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr);
typedef bool (*FFVARendererSetSizeFunc)(FFVARenderer *rnd, uint32_t width,
    uint32_t height);
typedef bool (*FFVARendererPutSurfaceFunc)(FFVARenderer *rnd, FFVASurface *s,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags);
typedef bool (*FFVARendererWindowcloseFunc)(FFVARenderer *rnd);
typedef int (*FFVARendererLoadImageFunc)(FFVARenderer *rnd, const char *image_path, float x, float y, float scale, float rotate);
typedef int (*FFVARendererLoadTextFunc)(FFVARenderer *rnd, const char *font_path, const char *text, int font_size, float x, float y);
typedef bool (*FFVARendererAdjustImageFunc)(FFVARenderer *rnd, int image_id, float x, float y, float scale, float rotate);
typedef void (*FFVARendererSetCenterFunc)(FFVARenderer *rnd, float x, float y);
typedef void (*FFVARendererResizeFunc)(FFVARenderer *rnd, int x, int y, uint32_t width, uint32_t height);

struct ffva_renderer_s {
    const void *klass;
    FFVARenderer *parent;
    FFVADisplay *display;
    void *window;
    uint32_t width;
    uint32_t height;
};

struct ffva_renderer_class_s {
    AVClass base;
    uint32_t size;
    FFVARendererType type;
    FFVARendererInitFunc init;
    FFVARendererFinalizeFunc finalize;
    FFVARendererGetVisualIdFunc get_visual_id;
    FFVARendererGetSizeFunc get_size;
    FFVARendererSetSizeFunc set_size;
    FFVARendererPutSurfaceFunc put_surface;
    FFVARendererWindowcloseFunc widow_close;
    FFVARendererLoadImageFunc renderer_load_image;
    FFVARendererLoadTextFunc renderer_load_text;
    FFVARendererAdjustImageFunc renderer_adjust_image;
    FFVARendererSetCenterFunc renderer_set_center;
    FFVARendererResizeFunc renderer_resize;
};

DLL_HIDDEN
FFVARenderer *
ffva_renderer_new(const FFVARendererClass *klass, FFVADisplay *display,
    uint32_t flags);

DLL_HIDDEN
uintptr_t
ffva_renderer_get_visual_id(FFVARenderer *rnd);

#endif /* FFVA_RENDERER_PRIV_H */
