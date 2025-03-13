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
#include <X11/Xatom.h> 
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
    int window_x;
    int window_y;
    uint32_t wanted_window_width;
    uint32_t wanted_window_height;
    int wanted_window_x;
    int wanted_window_y;
    uint32_t window_width_orig;
    uint32_t window_height_orig;
    bool is_fullscreen;
    bool is_fullscreen_changed;
    pthread_mutex_t mutex;
    bool is_always_on_top;
    bool is_always_ont_top_changed;
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
        1, 1, width, height, 0, depth, InputOutput, vi->visual,
        xswa_mask, &xswa);
    if (vi != &visualInfo)
        XFree(vi);
    if (!rnd->window)
        goto error_create_window;
    // 设置无边框属性
    Atom motif_hints = XInternAtom(rnd->display, "_MOTIF_WM_HINTS", False);
    if (motif_hints != None) {
        typedef struct {
            unsigned long flags;
            unsigned long functions;
            unsigned long decorations;
            long input_mode;
            unsigned long status;
        } MotifWmHints;
        
        MotifWmHints hints = {0};
        hints.flags = 2;       // 表示要设置decorations属性
        hints.decorations = 0; // 0表示无边框
        
        XChangeProperty(rnd->display, rnd->window,
                       motif_hints, motif_hints,
                       32, PropModeReplace,
                       (unsigned char *)&hints,
                       sizeof(hints)/sizeof(long));
    }
    Atom window_type = XInternAtom(rnd->display, "_NET_WM_WINDOW_TYPE", False);
    Atom normal_type = XInternAtom(rnd->display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    XChangeProperty(rnd->display, rnd->window, window_type, 
                XA_ATOM, 32, PropModeReplace,
                (unsigned char *)&normal_type, 1);

    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PPosition | PSize | USPosition | USSize;
    size_hints->x = 0;
    size_hints->y = 0;
    size_hints->width = width;
    size_hints->height = height;
    XSetWMNormalHints(rnd->display, rnd->window, size_hints);
    XFree(size_hints);
    XResizeWindow(rnd->display, rnd->window, width, height);
    XMoveWindow(rnd->display, rnd->window, 0, 0);

//   wmDeleteWindow = XInternAtom(rnd->display, "WM_DELETE_WINDOW", True);
  //  XSetWMProtocols(rnd->display, rnd->window, &wmDeleteWindow, 1);
    XSelectInput(rnd->display, rnd->window, x11_event_mask);
    XMapWindow(rnd->display, rnd->window);

    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = rnd->window;
    ev.xclient.message_type = XInternAtom(rnd->display, "_NET_WM_STATE", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
    ev.xclient.data.l[1] = XInternAtom(rnd->display, "_NET_WM_STATE_FULLSCREEN", False);
    ev.xclient.data.l[2] = 0;

    XSendEvent(rnd->display, DefaultRootWindow(rnd->display), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &ev);

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

static bool
renderer_set_always_on_top(FFVARendererX11 *rnd, bool enable) {
    rnd->is_always_on_top = enable;
    rnd->is_always_ont_top_changed = true;
    return true;
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
        success = x11_get_geometry(rnd->display, rnd->window, &rnd->window_x, &rnd->window_y,
            &rnd->window_width, &rnd->window_height);
        rnd->is_fullscreen_changed = false;
        if (!success)
            return false;
    }
    if (rnd->is_always_ont_top_changed)
    { 
        Atom net_wm_state = XInternAtom(rnd->display, "_NET_WM_STATE", False);
        Atom above_state = XInternAtom(rnd->display, "_NET_WM_STATE_ABOVE", False);
        XEvent xev;

        if (net_wm_state == None || above_state == None)
            return false;

        xev.type = ClientMessage;
        xev.xclient.window = rnd->window;
        xev.xclient.message_type = net_wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = rnd->is_always_on_top ? 1 /* _NET_WM_STATE_ADD */ : 0 /* _NET_WM_STATE_REMOVE */;
        xev.xclient.data.l[1] = above_state;
        xev.xclient.data.l[2] = 0;

        XSendEvent(rnd->display, RootWindow(rnd->display, rnd->screen), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        // XSync(rnd->display, False);
        rnd->is_always_ont_top_changed = false;
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
        success = x11_get_geometry(rnd->display, rnd->window, &rnd->window_x, &rnd->window_y,
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

static void
renderer_resize(FFVARendererX11 *rnd, int x, int y, uint32_t width, uint32_t height)
{
    av_log(rnd, AV_LOG_ERROR, "x11   renderer_resize ------------------------------\n");
    if (!rnd->window || !rnd->display)
        return;

    XWindowAttributes wattr;

    XGetWindowAttributes(rnd->display, rnd->root_window, &wattr);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    Atom wm_state = XInternAtom(rnd->display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(rnd->display, "_NET_WM_STATE_FULLSCREEN", False);

    int status = XGetWindowProperty(rnd->display, rnd->window, wm_state,
                                    0, (~0L), False, AnyPropertyType,
                                    &actual_type, &actual_format,
                                    &nitems, &bytes_after, &data);

    if (status == Success && data) {
        Atom *states = (Atom *)data;
        for (unsigned long i = 0; i < nitems; ++i) {
            if (states[i] == fullscreen) {
                XFree(data);
                rnd->is_fullscreen = true;
                break;
            }
        }
        // XFree(data);
    }
    if (rnd->is_fullscreen)
    {
        printf("fullscreen\n");
        XEvent xev;
        Atom wm_state = XInternAtom(rnd->display, "_NET_WM_STATE", False);
        Atom fullscreen = XInternAtom(rnd->display, "_NET_WM_STATE_FULLSCREEN", False);

        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = rnd->window;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 0; // _NET_WM_STATE_ADD/REMOVE
        xev.xclient.data.l[1] = fullscreen;
        xev.xclient.data.l[2] = 0; // No second property

        XSendEvent(rnd->display, DefaultRootWindow(rnd->display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        XSync(rnd->display, False);
        rnd->is_fullscreen = false;
    }
    {
        XEvent x_event;
        Atom wm_fullscreen;

        x_event.type = ClientMessage;
        x_event.xclient.window = rnd->window;
        x_event.xclient.message_type = XInternAtom(rnd->display, "_NET_WM_STATE", False);
        x_event.xclient.format = 32;
        x_event.xclient.data.l[0] = 0;  /* 0 = Windowed, 1 = Fullscreen */
        wm_fullscreen = XInternAtom(rnd->display, "_NET_WM_STATE_FULLSCREEN", False);
        x_event.xclient.data.l[1] = wm_fullscreen;
        x_event.xclient.data.l[2] = wm_fullscreen;

        XSendEvent(rnd->display, RootWindow(rnd->display, rnd->screen), False, ClientMessage, &x_event);

        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        unsigned char *data = NULL;
        Atom wm_state = XInternAtom(rnd->display, "_NET_WM_STATE", False);
        Atom fullscreen = XInternAtom(rnd->display, "_NET_WM_STATE_FULLSCREEN", False);

        int status = XGetWindowProperty(rnd->display, rnd->window, wm_state,
                                        0, (~0L), False, AnyPropertyType,
                                        &actual_type, &actual_format,
                                        &nitems, &bytes_after, &data);

        if (status == Success && data) {
            Atom *states = (Atom *)data;
            for (unsigned long i = 0; i < nitems; ++i) {
                if (states[i] == fullscreen) {
                    XFree(data);
                    rnd->is_fullscreen = true;
                    break;
                }
            }
            // XFree(data);
        }
    }
    
    if (rnd->is_fullscreen)
    {
        printf("fullscreen too\n");
    }
    // if (rnd->window_width == wattr.width && rnd->window_height == wattr.height)
    // {
    //     // system()
    //     char cmd[256], cmd2[256];
    //     sprintf(cmd, "wmctrl -i -r %d -b toggle,fullscreen\n", rnd->window);
    //     printf("cmd : %s", cmd);
    //     system(cmd);
    //     sprintf(cmd2, "wmctrl -i -r %d -e 0,%d,%d,%d,%d", rnd->window, x, y, width, height);
    //     printf("cmd2 : %s", cmd2);
    //     system(cmd2);
    // }
    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PPosition | PSize | USPosition | USSize;
    size_hints->x = x;
    size_hints->y = y;
    size_hints->width = width;
    size_hints->height = height;
    XSetWMNormalHints(rnd->display, rnd->window, size_hints);
    XFree(size_hints);
    // XResizeWindow(rnd->display, rnd->window, width, height);
    // XMoveWindow(rnd->display, rnd->window, 0, 0);

    // XResizeWindow(rnd->display, rnd->window, width, height);
    XMoveResizeWindow(rnd->display, rnd->window, x, y, width, height);
    // rnd->window_width = width;
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
        .renderer_resize = (FFVARendererResizeFunc)renderer_resize,
        .renderer_set_always_above = (FFVARendererSetAlwaysAboveFunc)renderer_set_always_on_top,
    };
    return &g_class;
}

// Creates a new renderer object from the supplied VA display
FFVARenderer *
ffva_renderer_x11_new(FFVADisplay *display, uint32_t flags)
{
    return ffva_renderer_new(ffva_renderer_x11_class(), display, flags);
}
