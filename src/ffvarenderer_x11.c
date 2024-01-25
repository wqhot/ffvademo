/*
 * ffvarenderer_x11.h - VA/X11 renderer
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
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <va/va_x11.h>
#include "ffvarenderer_x11.h"
#include "ffvarenderer_priv.h"
#include "ffvadisplay_priv.h"
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

Bool CheckCloseEvent(Display *display, XEvent *event, XPointer arg) {
    return (event->type == ClientMessage) && ((Atom) event->xclient.data.l[0] == XInternAtom(display, "WM_DELETE_WINDOW", False));
}

bool IsWindowClosed(Display *display, Window window) {
    XEvent event;
    if (XCheckIfEvent(display, &event, CheckCloseEvent, (XPointer) NULL)) {
        return true;
    }
    return false;
}

struct ffva_renderer_x11_s {
    FFVARenderer base;

    Display *display;
    uint32_t display_width;
    uint32_t display_height;
    int screen;
    Visual *visual;
    Window root_window;
    unsigned long black_pixel;
    unsigned long white_pixel;
    Window window;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t window_width_orig;
    uint32_t window_height_orig;
    bool is_fullscreen;
    bool is_fullscreen_changed;
    pthread_mutex_t mutex;
};

static const uint32_t x11_event_mask = (
    KeyPressMask        |
    KeyReleaseMask      |
    ButtonPressMask     |
    ButtonReleaseMask   |
    PointerMotionMask   |
    EnterWindowMask     |
    ExposureMask        |
    StructureNotifyMask );

// X error trap
static int x11_error_code = 0;
static int (*old_error_handler)(Display *, XErrorEvent *);
Atom wmDeleteWindow;

static int
error_handler(Display *dpy, XErrorEvent *error)
{
    x11_error_code = error->error_code;
    return 0;
}

static void
x11_trap_errors(void)
{
    x11_error_code = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

static int
x11_untrap_errors(void)
{
    XSetErrorHandler(old_error_handler);
    return x11_error_code;
}

static bool
x11_get_geometry(Display *dpy, Drawable drawable, int *x_ptr, int *y_ptr,
    unsigned int *width_ptr, unsigned int *height_ptr)
{
    Window rootwin;
    int x, y;
    unsigned int width, height, bw, depth;
    x11_trap_errors();
    XGetGeometry(dpy, drawable, &rootwin, &x, &y, &width, &height, &bw, &depth);
    if (x11_untrap_errors())
        return false;

    if (x_ptr)
        *x_ptr = x;
    if (y_ptr)
        *y_ptr = y;
    if (width_ptr)
        *width_ptr = width;
    if (height_ptr)
        *height_ptr = height;
    return true;
}

static bool
window_create(FFVARendererX11 *rnd, uint32_t width, uint32_t height)
{
    av_log(rnd, AV_LOG_ERROR, "window_create ------------------------------\n");
    int depth;
    XVisualInfo visualInfo, *vi;
    XSetWindowAttributes xswa;
    unsigned long xswa_mask;
    XWindowAttributes wattr;
    int num_visuals;
    VisualID vid;

    XGetWindowAttributes(rnd->display, rnd->root_window, &wattr);
    depth = wattr.depth;
    if (depth != 15 && depth != 16 && depth != 24 && depth != 32)
        depth = 24;

    vid = ffva_renderer_get_visual_id(rnd->base.parent);
    if (vid) {
        visualInfo.visualid = vid;
        vi = XGetVisualInfo(rnd->display, VisualIDMask, &visualInfo,
            &num_visuals);
        if (!vi || num_visuals < 1)
            goto error_create_visual;
    }
    else {
        vi = &visualInfo;
        XMatchVisualInfo(rnd->display, rnd->screen, depth, TrueColor, vi);
    }

    xswa_mask = CWBorderPixel | CWBackPixel;
    xswa.border_pixel = rnd->black_pixel;
    xswa.background_pixel = rnd->white_pixel;

    rnd->window = XCreateWindow(rnd->display, rnd->root_window,
        0, 0, width, height, 0, depth, InputOutput, vi->visual,
        xswa_mask, &xswa);
    if (vi != &visualInfo)
        XFree(vi);
    if (!rnd->window)
        goto error_create_window;
 //   wmDeleteWindow = XInternAtom(rnd->display, "WM_DELETE_WINDOW", True);
  //  XSetWMProtocols(rnd->display, rnd->window, &wmDeleteWindow, 1);
    XSelectInput(rnd->display, rnd->window, x11_event_mask);
    XMapWindow(rnd->display, rnd->window);

    rnd->window_width = width;
    rnd->window_height = height;
    rnd->base.window = (void *)(uintptr_t)rnd->window;
    return true;

    /* ERRORS */
error_create_visual:
    av_log(rnd, AV_LOG_ERROR, "failed to create X visual (id:%zu)\n",
        (size_t)visualInfo.visualid);
    if (vi)
        XFree(vi);
    return false;
error_create_window:
    av_log(rnd, AV_LOG_ERROR, "failed to create X window of size %ux%u\n",
        width, height);
    return false;
}

static void
window_destroy(FFVARendererX11 *rnd)
{
    XDestroyWindow(rnd->display, rnd->window);
}

static bool
renderer_init(FFVARendererX11 *rnd, uint32_t flags)
{
    FFVADisplay * const display = rnd->base.display;

    if (ffva_display_get_type(display) != FFVA_DISPLAY_TYPE_X11)
        return false;
    rnd->display = display->native_display;
    rnd->screen = DefaultScreen(rnd->display);
    rnd->display_width  = DisplayWidth(rnd->display, rnd->screen);
    rnd->display_height = DisplayHeight(rnd->display, rnd->screen);
    rnd->visual = DefaultVisual(rnd->display, rnd->screen);
    rnd->root_window = RootWindow(rnd->display, rnd->screen);
    rnd->black_pixel = BlackPixel(rnd->display, rnd->screen);
    rnd->white_pixel = WhitePixel(rnd->display, rnd->screen);
    pthread_mutex_init(&rnd->mutex,NULL);
    return true;
}

static void
renderer_finalize(FFVARendererX11 *rnd)
{
    if (rnd->window) {
        window_destroy(rnd);
        rnd->window = None;
    }

    if (rnd->display) {
        XFlush(rnd->display);
        XSync(rnd->display, False);
    }
}

static bool
renderer_get_size(FFVARendererX11 *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr)
{
    bool success;
    XEvent event;
    if (rnd->is_fullscreen_changed) {
        XFlush(rnd->display);
        XSync(rnd->display, False);
        success = x11_get_geometry(rnd->display, rnd->window, NULL, NULL,
            &rnd->window_width, &rnd->window_height);
        rnd->is_fullscreen_changed = false;
        if (!success)
            return false;
    }
    #if 0
    XWindowAttributes window_attrs;
    int revert_to=0;
    XGetInputFocus(rnd->display, &rnd->window, &revert_to);
    if (XGetWindowAttributes(rnd->display, rnd->window, &window_attrs) == 0) {
        fprintf(stderr, "Failed to get window attributes\n");
        XCloseDisplay(rnd->display);
        return 1;
    }
    if (window_attrs.map_state == IsViewable) {
        printf("Window is open\n");
    } else {
        printf("Window is closed\n");
        return false;
    }
    #endif
        if (!rnd->display || !rnd->window)
        {
            return false;
        }
        if(XCheckMaskEvent(rnd->display, StructureNotifyMask, &event)) {
        // 处理事件
        if (event.type == DestroyNotify) {
            return false;
        }
        }
        //pthread_mutex_lock(&rnd->mutex);
#if 1
    x11_trap_errors();
    if(rnd->display){
        XFlush(rnd->display);
        XSync(rnd->display, False);
        success = x11_get_geometry(rnd->display, rnd->window, NULL, NULL,
            &rnd->window_width, &rnd->window_height);
        if (!success)
         return false;
    }
#endif
    if (width_ptr)
        *width_ptr = rnd->window_width;
    if (height_ptr)
        *height_ptr = rnd->window_height;
    return true;
}

static bool
renderer_set_size(FFVARendererX11 *rnd, uint32_t width, uint32_t height)
{
     av_log(rnd, AV_LOG_ERROR, "x11   renderer_set_size ------------------------------\n");
    if (!rnd->window)
        return window_create(rnd, width, height);

    XResizeWindow(rnd->display, rnd->window, width, height);
    rnd->window_width = width;
    rnd->window_height = height;
    return true;
}

static bool
renderer_put_surface(FFVARendererX11 *rnd, FFVASurface *surface,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags)
{
    VADisplay const va_display = rnd->base.display->va_display;
    VAStatus va_status;
    int revert_to=0;
    if (!va_display || !rnd->window)
        return false;
#if 0
    XWindowAttributes window_attrs;
    XGetInputFocus(rnd->display, &rnd->window, &revert_to);
    if (XGetWindowAttributes(rnd->display, rnd->window, &window_attrs) == 0) {
        fprintf(stderr, "Failed to get window attributes\n");
        XCloseDisplay(rnd->display);
        return 1;
    }

    if (window_attrs.map_state == IsViewable) {
        printf("Window is open\n");
    } else {
        printf("Window is closed\n");
        goto error_put_surface;
    }
#endif
    va_status = vaPutSurface(va_display, surface->id, rnd->window,
        src_rect->x, src_rect->y, src_rect->width, src_rect->height,
        dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height,
        NULL, 0, flags);
    if (va_status != VA_STATUS_SUCCESS)
        goto error_put_surface;
    return true;

    /* ERRORS */
error_put_surface:
    av_log(rnd, AV_LOG_ERROR, "failed to render surface 0x%08x (%s)",
        surface->id, vaErrorStr(va_status));
    return false;
}

#if 1
static bool
render_window_close(FFVARendererX11 *rnd)
{
    //XSelectInput(rnd->display, rnd->window, StructureNotifyMask);

    sleep(3);
    while(1)
    {
        XEvent event;
        if (!rnd->display || !rnd->window)
        {
            continue;
        }
        if(XCheckMaskEvent(rnd->display, StructureNotifyMask, &event)) {
        // 处理事件
        if (event.type == DestroyNotify) {
            break;
        }
            }
        //pthread_mutex_lock(&rnd->mutex);
       // XNextEvent(rnd->display, &event);
//        if (event.type == ClientMessage && event.xclient.data.l[0] == WM_DELETE_WINDOW) {
          //  break;
     //   }
        //pthread_mutex_unlock(&rnd->mutex);
    }
    return true;
}
#endif
static const FFVARendererClass *
ffva_renderer_x11_class(void)
{
    static const FFVARendererClass g_class = {
        .base = {
            .class_name = "FFVARendererX11",
            .item_name  = av_default_item_name,
            .option     = NULL,
            .version    = LIBAVUTIL_VERSION_INT,
        },
        .size           = sizeof(FFVARendererX11),
        .type           = FFVA_RENDERER_TYPE_X11,
        .init           = (FFVARendererInitFunc)renderer_init,
        .finalize       = (FFVARendererFinalizeFunc)renderer_finalize,
        .get_size       = (FFVARendererGetSizeFunc)renderer_get_size,
        .set_size       = (FFVARendererSetSizeFunc)renderer_set_size,
        .put_surface    = (FFVARendererPutSurfaceFunc)renderer_put_surface,
        .widow_close    = (FFVARendererWindowcloseFunc)render_window_close,
    };
    return &g_class;
}

// Creates a new renderer object from the supplied VA display
FFVARenderer *
ffva_renderer_x11_new(FFVADisplay *display, uint32_t flags)
{
    return ffva_renderer_new(ffva_renderer_x11_class(), display, flags);
}
