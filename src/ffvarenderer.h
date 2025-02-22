/*
 * ffvarenderer.h - VA renderer abstraction
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

#ifndef FFVA_RENDERER_H
#define FFVA_RENDERER_H

#include "ffvadisplay.h"
#include "ffvasurface.h"

#define FFVA_RENDERER(rnd) \
    ((FFVARenderer *)(rnd))

typedef struct ffva_renderer_s          FFVARenderer;

typedef enum {
    FFVA_RENDERER_TYPE_X11 = 1,
    FFVA_RENDERER_TYPE_EGL,
    FFVA_RENDERER_TYPE_DRM,
} FFVARendererType;

/** Releases all renderer resources */
void
ffva_renderer_free(FFVARenderer *rnd);

/** Releases renderer object and resets the supplied pointer to NULL */
void
ffva_renderer_freep(FFVARenderer **rnd_ptr);

/** Returns the type of the supplied renderer */
FFVARendererType
ffva_renderer_get_type(FFVARenderer *rnd);

/** Returns the size of the rendering device */
bool
ffva_renderer_get_size(FFVARenderer *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr);

/** Resizes the rendering device to the supplied dimensions */
bool
ffva_renderer_set_size(FFVARenderer *rnd, uint32_t width, uint32_t height);

/** Submits the supplied surface to the rendering device */
bool
ffva_renderer_put_surface(FFVARenderer *rnd, FFVASurface *surface,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags);

/** Returns the native display associated to the supplied renderer */
void *
ffva_renderer_get_native_display(FFVARenderer *rnd);

/** Returns the native window associated to the supplied renderer */
void *
ffva_renderer_get_native_window(FFVARenderer *rnd);

bool
ffva_render_get_signal_window_close(FFVARenderer *rnd);

int ffva_renderer_load_image(FFVARenderer *rnd, const char *image_path, float x, float y, float scale, float rotation);

int ffva_renderer_load_text(FFVARenderer *rnd, const char *font_path, const char *text_context, int font_size, float x, float y);

bool ffva_renderer_adjust_image(FFVARenderer *rnd, int image_id, float x, float y, float scale, float rotation);

void ffva_renderer_set_center(FFVARenderer *rnd, float x, float y);

void ffva_renderer_resize(FFVARenderer *rnd, int x, int y, uint32_t width, uint32_t height);

#endif /* FFVA_RENDERER_H */
