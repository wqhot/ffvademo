/*
 * ffvarenderer_egl.c - VA/EGL renderer
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
extern "C"
{
#include "sysdeps.h"
#include <unistd.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <drm/drm_fourcc.h>
#include "egl_compat.h"
#include "vaapi_utils.h"
#include "ffvafilter.h"
#include "ffvarenderer_egl.h"
#include "ffvarenderer_priv.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#if USE_X11
# include "ffvarenderer_x11.h"
#endif
}

#include <unordered_map>
#include <locale.h>
#include <ft2build.h>
#include FT_FREETYPE_H

/* Define the VA buffer memory type to use. 0:let the driver decide */
#ifndef VA_BUFFER_MEMORY_TYPE
#define VA_BUFFER_MEMORY_TYPE 0
#endif

/* Define whether the EGL implementation owns dma_buf fd */
#ifndef EGL_image_dma_buf_import_owns_fd
#define EGL_image_dma_buf_import_owns_fd 0
#endif

/* Additional DRM formats */
#ifndef DRM_FORMAT_R8
#define DRM_FORMAT_R8   fourcc_code('R', '8', ' ', ' ')
#endif
#ifndef DRM_FORMAT_RG88
#define DRM_FORMAT_RG88 fourcc_code('R', 'G', '8', '8')
#endif
#ifndef DRM_FORMAT_GR88
#define DRM_FORMAT_GR88 fourcc_code('G', 'R', '8', '8')
#endif

#define IMAGE_FORMAT_RGBA    0
#define IMAGE_FORMAT_RGB565  1
static bool
va_format_to_drm_format(const VAImageFormat *va_format, uint32_t *format_ptr)
{
    uint32_t format;

#ifdef WORDS_BIGENDIAN
# define DRM_FORMAT_NATIVE_ENDIAN DRM_FORMAT_BIG_ENDIAN
#else
# define DRM_FORMAT_NATIVE_ENDIAN 0
#endif

    switch (va_format->fourcc) {
    case VA_FOURCC('I','4','2','0'):
        format = DRM_FORMAT_YUV420;
        break;
    case VA_FOURCC('Y','V','1','2'):
        format = DRM_FORMAT_YVU420;
        break;
    case VA_FOURCC('N','V','1','2'):
        format = DRM_FORMAT_NV12;
        break;
    case VA_FOURCC('R','G','B','X'):
        format = DRM_FORMAT_XBGR8888 | DRM_FORMAT_NATIVE_ENDIAN;
        break;
    case VA_FOURCC('R','G','B','A'):
        format = DRM_FORMAT_ABGR8888 | DRM_FORMAT_NATIVE_ENDIAN;
        break;
    case VA_FOURCC('B','G','R','X'):
        format = DRM_FORMAT_XRGB8888 | DRM_FORMAT_NATIVE_ENDIAN;
        break;
    case VA_FOURCC('B','G','R','A'):
        format = DRM_FORMAT_ARGB8888 | DRM_FORMAT_NATIVE_ENDIAN;
        break;
    default:
        format = 0;
        break;
    }
    if (!format)
        return false;

#undef DRM_FORMAT_NATIVE_ENDIAN

    if (format_ptr)
        *format_ptr = format;
    return true;
}

static uint32_t
get_va_mem_type(uint32_t flags)
{
    uint32_t va_mem_type = VA_BUFFER_MEMORY_TYPE;

    switch (flags & FFVA_RENDERER_EGL_MEM_TYPE_MASK) {
    case FFVA_RENDERER_EGL_MEM_TYPE_DMA_BUFFER:
        va_mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        break;
    case FFVA_RENDERER_EGL_MEM_TYPE_GEM_BUFFER:
        va_mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
        break;
    case FFVA_RENDERER_EGL_MEM_TYPE_MESA_IMAGE:
    case FFVA_RENDERER_EGL_MEM_TYPE_MESA_TEXTURE:
        va_mem_type = 0;
        break;
    }
    return va_mem_type;
}

/* ------------------------------------------------------------------------ */
/* --- EGL Helpers                                                      --- */
/* ------------------------------------------------------------------------ */

typedef struct egl_vtable_s             EglVTable;
typedef struct egl_context_s            EglContext;
typedef struct egl_program_s            EglProgram;

struct egl_vtable_s {
    PFNEGLCREATEIMAGEKHRPROC egl_create_image_khr;
    PFNEGLDESTROYIMAGEKHRPROC egl_destroy_image_khr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC gl_egl_image_target_texture2d_oes;
    PFNEGLCREATEDRMIMAGEMESAPROC egl_create_drm_image_mesa;
    PFNEGLEXPORTDRMIMAGEMESAPROC egl_export_drm_image_mesa;
};

struct egl_context_s {
    EglVTable vtable;
    EGLDisplay display;
    GLint visualid;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    uint32_t surface_width;
    uint32_t surface_height;
    EGLClientBuffer buffer;
    EGLImageKHR images[3];
    uint32_t num_images;
    GLenum tex_target;
    GLuint textures[3];
    uint32_t num_textures;
    const char *frag_shader_text;
    const char *vert_shader_text;
    EglProgram *program;
    bool program_changed;
    GLfloat proj[16];
    bool is_initialized;
    uint32_t target_width;
    uint32_t target_height;
};

typedef struct image_overlay {
    GLuint texture;      // OpenGL纹理ID
    // GLuint texture_uv;
    unsigned int width;           // 图片宽度
    unsigned int height;          // 图片高度
    float pos_x;         // X坐标 (归一化坐标0-1)
    float pos_y;         // Y坐标 (归一化坐标0-1)
    float scale;         // 缩放比例
    float rotation;      // 旋转角度(弧度)
    int crop_x0;
    int crop_y0;
    int crop_x1;
    int crop_y1;
    int format;
    unsigned char* data;
    EglProgram *program;
} image_overlay;

typedef struct char_font {
    image_overlay *image;
    GLuint Size_w;    // Size of glyph
    GLuint Size_h;
    GLuint Bearing_top; // Offset from baseline to left/top of glyph
    GLuint Bearing_left;
    GLuint Advance;     // Horizontal offset to advance to next glyph
    wchar_t c;
} char_font;

typedef struct text_overlay {
    char_font **chars;
    int num_chars;
    FT_Library ft;
    FT_Face face;
    bool inited;
    int font_size;
    float pos_x;
    float pos_y;
} text_overlay;

struct egl_program_s {
    GLuint program;
    GLuint frag_shader;
    GLuint vert_shader;
    int proj_uniform;
    int tex_uniforms[3];
    int center_uniform;
};

static void
egl_program_free(EglProgram *program);

static const char *vert_shader_text_default =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "\n"
    "uniform mat4 proj;\n"
    "\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = proj * vec4(position, 0.0, 1.0);\n"
    "    v_texcoord  = texcoord;\n"
    "}\n";

static const char *frag_shader_text_rgba =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "\n"
    "uniform sampler2D tex0;\n"
    "\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex0, v_texcoord);\n"
    "}\n";

/*
 * For 8-bit per sample, and RGB limited range [16, 235]:
 *
 *             219                219                       219
 *   Y =  16 + --- * Kr     * R + --- * (1 - Kr - Kb) * G + --- * Kb     * B
 *             255                255                       255
 *
 *             112     Kr         112   1 - (Kr + Kb)       112
 *   U = 128 - --- * ------ * R - --- * ------------- * G + ---          * B
 *             255   1 - Kb       255      1 - Kb           255
 *
 *             112                112   1 - (Kr + Kb)       112     Kb
 *   V = 128 + ---          * R - --- * ------------- * G - --- * ------ * B
 *             255                255      1 - Kr           255   1 - Kr
 *
 * Constants for ITU-R BT.601 (SDTV):
 *   Kb = 0.114
 *   Kr = 0.299
 *
 * Constants for ITU-R BT.709 (HDTV):
 *   Kb = 0.0722
 *   Kr = 0.2126
 *
 * Constants for SMPTE 240M:
 *   Kb = 0.087
 *   Kr = 0.212
 *
 * Matrix generation with xcas:
 *   inverse([
 *   [  Kr         ,  1-(Kr+Kb)          ,  Kb        ]*219/255,
 *   [ -Kr/(1-Kb)  , -(1-(Kr+Kb))/(1-Kb) ,  1         ]*112/255,
 *   [  1          , -(1-(Kr+Kb))/(1-Kr) , -Kb/(1-Kr) ]*112/255])
 *
 * As a reminder:
 * - Kb + Kr + Kg = 1.0
 * - Y range is [0.0, 1.0], U/V range is [-1.0, 1.0]
 */
#define YUV2RGB_COLOR_BT601_LIMITED                     \
    "const vec3 yuv2rgb_ofs = vec3(0.0625, 0.5, 0.5);"  \
    "const mat3 yuv2rgb_mat = "                         \
    "    mat3(1.16438356,  0         ,  1.61651785, "   \
    "         1.16438356, -0.38584641, -0.78656070, "   \
    "         1.16438356,  2.01723214, 0          );\n"

#define YUV2RGB_COLOR_BT709_LIMITED                     \
    "const vec3 yuv2rgb_ofs = vec3(0.0625, 0.5, 0.5);"  \
    "const mat3 yuv2rgb_mat = "                         \
    "    mat3(1.16438356,  0         ,  1.79274107, "   \
    "         1.16438356, -0.21324861, -0.53290932, "   \
    "         1.16438356,  2.11240178, 0          );\n"

#define YUV2RGB_COLOR_SMPTE240M_LIMITED                 \
    "const vec3 yuv2rgb_ofs = vec3(0.0625, 0.5, 0.5);"  \
    "const mat3 yuv2rgb_mat = "                         \
    "    mat3(1.16438356,  0         ,  1.79410714, "   \
    "         1.16438356, -0.25798483, -0.54258304, "   \
    "         1.16438356,  2.07870535,  0         );\n"

#define YUV2RGB_COLOR(CONV)                             \
    U_GEN_CONCAT(YUV2RGB_COLOR_,CONV)                   \
    "vec3 rgb = (yuv - yuv2rgb_ofs) * yuv2rgb_mat;\n"   \
    "gl_FragColor = vec4(rgb, 1);\n"

#if 1
static const char *frag_shader_text_nv12 =
    // "#ifdef GL_ES\n"
    // "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    // "uniform samplerExternalOES tex0;\n"
    // "uniform samplerExternalOES tex1;\n"
    // "#else\n"
    "uniform sampler2D tex0;\n"
    "uniform sampler2D tex1;\n"
    "uniform vec2 center;                          \n"
    "const float magnification = 2.0;                          \n"
    "const float lens_magn_r = 0.125;                          \n"
    // "#endif\n"
    "\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    vec2 coord_uv = vec2(v_texcoord.x, v_texcoord.y);         \n"
    "    float mouse_dist = distance(coord_uv, center);   \n"
    "    vec4 p_y; vec4 p_uv; vec4 p_yuv; vec3 yuv;\n"
    "    if (mouse_dist < lens_magn_r) { \n"
    "    p_y  = texture2D(tex0, center + (v_texcoord - center) / magnification);\n"
    "    p_uv = texture2D(tex1, center + (v_texcoord - center) / magnification);\n"
    "    yuv  = vec3(p_y.r, p_uv.r, p_uv.g);\n"
    "    }else { \n"
    "    p_y  = texture2D(tex0, v_texcoord);\n"
    "    p_uv = texture2D(tex1, v_texcoord);\n"
    "    yuv  = vec3(p_y.r, p_uv.r, p_uv.g);\n"
    "    }\n"
    YUV2RGB_COLOR(BT601_LIMITED)
    "}\n";
#else
static const char *frag_shader_text_nv12 =
     "precision mediump float;"
		  "varying vec2 v_texcoord;"
		  "uniform sampler2D tex0;"
		  "uniform sampler2D tex1;"
		  "uniform sampler2D vTexture;"
		  "void main() {"
		  "    vec3 yuv;"
		  "    vec3 rgb;"
		  "    yuv.x = texture2D(tex0, v_texcoord).r;"
		  "    yuv.y = texture2D(tex1, v_texcoord).r - 0.5;"
		  "    yuv.z = texture2D(tex1, v_texcoord).a - 0.5;"
		  "    rgb = mat3(1.164,  1.164,  1.164,"
		  "               0.0,   -0.213,  2.112,"
		  "               1.793, -0.533,  0.0) * yuv;"
		  "    gl_FragColor = vec4(rgb, 1.0);"
		  "}";
#endif		  

static const char *frag_shader_text_yuv =
    "#ifdef GL_ES\n"
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES tex0;\n"
    "uniform samplerExternalOES tex1;\n"
    "uniform samplerExternalOES tex2;\n"
    "#else\n"
    "uniform sampler2D tex0;\n"
    "uniform sampler2D tex1;\n"
    "uniform sampler2D tex2;\n"
    "#endif\n"
    "\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    vec4 p_y = texture2D(tex0, v_texcoord);\n"
    "    vec4 p_u = texture2D(tex1, v_texcoord);\n"
    "    vec4 p_v = texture2D(tex2, v_texcoord);\n"
    "    vec3 yuv = vec3(p_y.r, p_u.r, p_v.r);\n"
    YUV2RGB_COLOR(BT601_LIMITED)
    "}\n";

static const char *frag_shader_text_rgb565 =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D tex0;\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    vec3 rgb;\n"
    "    rgb = texture2D(tex0, v_texcoord).rgb;\n"
    "    gl_FragColor = vec4(rgb, 1.0);\n"
    "}\n";

    
#if USE_GLES_VERSION != 0
static const char *frag_shader_text_egl_external =
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "\n"
    "uniform samplerExternalOES tex0;\n"
    "\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex0, v_texcoord);\n"
    "}\n";
#endif

static const char* vert_shader_image =  
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "\n"
    "uniform mat4 proj;\n"
    "uniform mat4 model;\n"         // 新增模型矩阵
    "\n"
    "attribute vec2 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 v_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = proj * model * vec4(position, 0.0, 1.0);\n"
    "    v_texcoord  = texcoord;\n"
    "}\n";

#if USE_GLES_VERSION != 1
static GLuint
egl_compile_shader(GLenum type, const char *source)
{
    GLuint shader;
    GLint status;
    char log[BUFSIZ];
    GLsizei log_length;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        av_log(NULL, AV_LOG_ERROR, "failed to compile %s shader\n",
            type == GL_FRAGMENT_SHADER ? "fragment" :
            type == GL_VERTEX_SHADER ? "vertex" : "<unknown>");

        glGetShaderInfoLog(shader, sizeof(log), &log_length, log);
        av_log(NULL, AV_LOG_ERROR, "info log: %s\n", log);
        return 0;
    }
    return shader;
}

static EglProgram *
egl_program_new(const char *frag_shader_text, const char *vert_shader_text)
{
    EglProgram *program;
    char msg[BUFSIZ];
    GLsizei msglen;
    GLint status;

    program = (EglProgram *)calloc(1, sizeof(*program));
    if (!program)
        return NULL;

    program->frag_shader =
        egl_compile_shader(GL_FRAGMENT_SHADER, frag_shader_text);
    if (!program->frag_shader)
        goto error;

    program->vert_shader =
        egl_compile_shader(GL_VERTEX_SHADER, vert_shader_text);
    if (!program->vert_shader)
        goto error;

    program->program = glCreateProgram();
    if (!program->program)
        goto error;
    glAttachShader(program->program, program->frag_shader);
    glAttachShader(program->program, program->vert_shader);
    glBindAttribLocation(program->program, 0, "position");
    glBindAttribLocation(program->program, 1, "texcoord");
    glLinkProgram(program->program);

    glGetProgramiv(program->program, GL_LINK_STATUS, &status);
    if (!status) {
        glGetProgramInfoLog(program->program, sizeof(msg), &msglen, msg);
        av_log(NULL, AV_LOG_ERROR, "failed to link program: %s\n", msg);
        goto error;
    }

    glUseProgram(program->program);
    program->proj_uniform    = glGetUniformLocation(program->program, "proj");
    program->tex_uniforms[0] = glGetUniformLocation(program->program, "tex0");
    program->tex_uniforms[1] = glGetUniformLocation(program->program, "tex1");
    program->tex_uniforms[2] = glGetUniformLocation(program->program, "tex2");
    program->center_uniform  = glGetUniformLocation(program->program, "center");
    glUseProgram(0);
    return program;

error:
    egl_program_free(program);
    return NULL;
}

static void
egl_program_free(EglProgram *program)
{
    if (!program)
        return;

    if (program->program)
        glDeleteProgram(program->program);
    if (program->frag_shader)
        glDeleteShader(program->frag_shader);
    if (program->vert_shader)
        glDeleteShader(program->vert_shader);
    free(program);
}
#endif

static void
egl_program_freep(EglProgram **program_ptr)
{
#if USE_GLES_VERSION != 1
    EglProgram * const program = *program_ptr;

    if (program) {
        egl_program_free(program);
        *program_ptr = NULL;
    }
#endif
}

static void
gl_texture_init_defaults(GLuint texture, GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void
matrix_set_identity(GLfloat *m)
{
#define MAT(m,r,c) (m)[(c) * 4 + (r)]
    MAT(m,0,0) = 1.0; MAT(m,0,1) = 0.0; MAT(m,0,2) = 0.0; MAT(m,0,3) = 0.0;
    MAT(m,1,0) = 0.0; MAT(m,1,1) = 1.0; MAT(m,1,2) = 0.0; MAT(m,1,3) = 0.0;
    MAT(m,2,0) = 0.0; MAT(m,2,1) = 0.0; MAT(m,2,2) = 1.0; MAT(m,2,3) = 0.0;
    MAT(m,3,0) = 0.0; MAT(m,3,1) = 0.0; MAT(m,3,2) = 0.0; MAT(m,3,3) = 1.0;
#undef MAT
}

/* ------------------------------------------------------------------------ */
/* --- EGL Renderer                                                     --- */
/* ------------------------------------------------------------------------ */

struct ffva_renderer_egl_s {
    FFVARenderer base;
    FFVARenderer *native_renderer;
    void *native_display;
    void *native_window;

    EglContext egl_context;
    VADisplay va_display;
    VAImage va_image;
    VABufferInfo va_buf_info;
    uint32_t va_mem_type;

    bool use_mesa_texture;
    bool use_mesa_image;
    FFVASurface mesa_surface;
    FFVAFilter *mesa_filter;
    image_overlay** overlays;
    int num_overlays;
    text_overlay** text_overlays;
    int num_text_overlays;
    float center_x;
    float center_y;
};

static bool
ensure_native_renderer(FFVARendererEGL *rnd, uint32_t flags)
{
    FFVADisplay * const display = rnd->base.display;
    FFVARenderer *native_renderer = rnd->native_renderer;

    if (!native_renderer) {
        switch (ffva_display_get_type(display)) {
#if USE_X11
        case FFVA_DISPLAY_TYPE_X11:
            native_renderer = ffva_renderer_x11_new(display, 0);
            break;
#endif
        }
        if (!native_renderer)
            return false;
        rnd->native_renderer = native_renderer;
        native_renderer->parent = &rnd->base;
    }

    rnd->native_display = ffva_renderer_get_native_display(native_renderer);
    rnd->native_window = ffva_renderer_get_native_window(native_renderer);
    rnd->va_display = ffva_display_get_va_display(display);
    return true;
}

static bool
ensure_display(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    EGLint version_major, version_minor;
    const char *str;

    if (!egl->display) {
        egl->display = eglGetDisplay((EGLNativeDisplayType)rnd->native_display);
        if (!egl->display)
            goto error_create_display;
    }

    if (!egl->is_initialized) {
        if (!eglInitialize(egl->display, &version_major, &version_minor))
            goto error_initialize;
        egl->is_initialized = true;

        str = eglQueryString(egl->display, EGL_VENDOR);
        av_log(rnd, AV_LOG_DEBUG, "EGL vendor: %s\n", str);
        str = eglQueryString(egl->display, EGL_VERSION);
        av_log(rnd, AV_LOG_DEBUG, "EGL version: %s\n", str);
        str = eglQueryString(egl->display, EGL_CLIENT_APIS);
        av_log(rnd, AV_LOG_DEBUG, "EGL client APIs: %s\n", str);
    }
    return true;

    /* ERRORS */
error_create_display:
    av_log(rnd, AV_LOG_ERROR, "failed to create EGL display\n");
    return false;
error_initialize:
    av_log(rnd, AV_LOG_ERROR, "failed to initialize EGL subsystem\n");
    return false;
}

static bool
ensure_vtable(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    EglVTable * const vtable = &egl->vtable;
    const char *extensions;
    int i;

    const char *egl_extensions_required[] = {
        "EGL_KHR_image_pixmap",
        "EGL_KHR_image_base",
        "EGL_EXT_image_dma_buf_import",
        "EGL_KHR_gl_texture_2D_image",
        "EGL_MESA_drm_image",
        NULL
    };

    const char *gl_extensions_required[] = {
        "GL_OES_EGL_image",
#if USE_GLES_VERSION != 0
        "GL_OES_EGL_image_external",
#endif
        NULL
    };

    extensions = eglQueryString(egl->display, EGL_EXTENSIONS);
    if (!extensions)
        return false;
    av_log(rnd, AV_LOG_DEBUG, "EGL extensions: %s\n", extensions);

    for (i = 0; egl_extensions_required[i] != NULL; i++) {
        const char * const name = egl_extensions_required[i];
        if (!strstr(extensions, name)) {
            av_log(rnd, AV_LOG_ERROR, "EGL stack does not support %s\n", name);
            // return false;
        }
    }

    vtable->egl_create_image_khr =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    vtable->egl_destroy_image_khr =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    if (!vtable->egl_create_image_khr || !vtable->egl_destroy_image_khr) {
        av_log(rnd, AV_LOG_ERROR, "failed to load EGL_KHR_image_base hooks\n");
        return false;
    }

    vtable->egl_create_drm_image_mesa =
        (PFNEGLCREATEDRMIMAGEMESAPROC)eglGetProcAddress("eglCreateDRMImageMESA");
    vtable->egl_export_drm_image_mesa =
        (PFNEGLEXPORTDRMIMAGEMESAPROC)eglGetProcAddress("eglExportDRMImageMESA");
    if (!vtable->egl_create_drm_image_mesa ||
        !vtable->egl_export_drm_image_mesa) {
        av_log(rnd, AV_LOG_ERROR, "failed to load EGL_MESA_drm_image hooks\n");
        // return false;
    }

    extensions = (const char *)glGetString(GL_EXTENSIONS);
    if (!extensions)
        return false;
    av_log(rnd, AV_LOG_DEBUG, "GL extensions: %s\n", extensions);

    for (i = 0; gl_extensions_required[i] != NULL; i++) {
        const char * const name = gl_extensions_required[i];
        if (!strstr(extensions, name)) {
            av_log(rnd, AV_LOG_ERROR, "GL stack does not support %s\n", name);
            // return false;
        }
    }

    vtable->gl_egl_image_target_texture2d_oes =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    if (!vtable->gl_egl_image_target_texture2d_oes) {
        av_log(rnd, AV_LOG_ERROR, "failed to load GL_OES_EGL_image hooks\n");
        return false;
    }
    return true;
}
static bool
ensure_config(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    EGLConfig config;
    EGLint num_configs, vid;

    const EGLint attribs[] = {
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         0,
        EGL_DEPTH_SIZE,         24,
        EGL_RENDERABLE_TYPE,    OPENGL_BIT,
        EGL_NONE
    };

    if (!ensure_display(rnd))
        return 0;

    if (!egl->config) {
        if (!eglChooseConfig(egl->display, attribs, &config, 1, &num_configs))
            goto error_choose_config;
        if (num_configs != 1)
            return 0;
        egl->config = config;
    }

    if (!egl->visualid) {
        if (!eglGetConfigAttrib(egl->display, config, EGL_NATIVE_VISUAL_ID,
                &vid))
            goto error_choose_visual;
        egl->visualid = vid;
    }
    return egl->visualid;

    /* ERRORS */
error_choose_config:
    av_log(rnd, AV_LOG_ERROR, "failed to get an EGL visual config\n");
    return 0;
error_choose_visual:
    av_log(rnd, AV_LOG_ERROR, "failed to get EGL visual id\n");
    return 0;
}

static bool
ensure_context(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    const EGLint *attribs;

    if (!ensure_native_renderer(rnd, 0))
        return false;

    eglBindAPI(OPENGL_API);

    switch (OPENGL_BIT) {
    case EGL_OPENGL_ES2_BIT: {
        const EGLint gles2_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        attribs = gles2_attribs;
        break;
    }
    case EGL_OPENGL_ES3_BIT_KHR: {
        const EGLint gles3_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };
        attribs = gles3_attribs;
        break;
    }
    default:
        attribs = NULL;
        break;
    }

    egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT,
        attribs);
    if (!egl->context)
        goto error_create_context;

    egl->surface = eglCreateWindowSurface(egl->display, egl->config,
        (EGLNativeWindowType)rnd->native_window, NULL);
    if (!egl->surface)
        goto error_create_surface;
    av_log(NULL, AV_LOG_ERROR, "eglMakeCurrent--------------------------------- :    \n");
    eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);

    if (!ensure_vtable(rnd))
        return false;

    glClearColor(0.0, 0.0, 0.0, 0.0);
#if USE_GLES_VERSION == 0
    glEnable(GL_TEXTURE_2D);
#endif
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;

    /* ERRORS */
error_create_context:
    av_log(rnd, AV_LOG_ERROR, "failed to create EGL context\n");
    return false;
error_create_surface:
    av_log(rnd, AV_LOG_ERROR, "failed to create EGL surface\n");
    return false;
}

image_overlay* image_overlay_create(const char* filename) 
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    printf("%s: %d x %d x %d\n", filename, width, height, channels);
    if (!data) return NULL;
    image_overlay* overlay = (image_overlay*)calloc(1, sizeof(image_overlay));
    overlay->width = width;
    overlay->height = height;
    overlay->scale = 1.0f;
    overlay->program = NULL;
    overlay->texture = 0;
    overlay->data = data;
    overlay->pos_x = 0;
    overlay->pos_y = 0;
    overlay->rotation = 0.0f;
    overlay->crop_x0 = 0;
    overlay->crop_x1 = width;
    overlay->crop_y0 = 0;
    overlay->crop_y1 = height;
    overlay->format = IMAGE_FORMAT_RGBA;
    return overlay;
}

image_overlay* image_overlay_create_yuv422(int width, int height)
{
    unsigned char* data = (unsigned char*)calloc(width * height * 4, sizeof(unsigned char));
    if (!data) return NULL;
    image_overlay* overlay = (image_overlay*)calloc(1, sizeof(image_overlay));
    overlay->width = width;
    overlay->height = height;
    overlay->scale = 1.0f;
    overlay->program = NULL;
    overlay->texture = 0;
    overlay->data = data;
    overlay->pos_x = 0.5;
    overlay->pos_y = 0.5;
    overlay->rotation = 0.0f;
    overlay->crop_x0 = 0;
    overlay->crop_x1 = width;
    overlay->crop_y0 = 0;
    overlay->crop_y1 = height;
    overlay->format = IMAGE_FORMAT_RGB565;
    return overlay;
}

static void create_image_overlay_context(image_overlay* overlay)
{
    const char* frag_shader = overlay->format == IMAGE_FORMAT_RGB565 ? frag_shader_text_rgb565 : frag_shader_text_rgba;
    
    overlay->program = egl_program_new(frag_shader, vert_shader_image);
    if (!overlay->program) {
        glDeleteTextures(1, &overlay->texture);
        // glDeleteTextures(1, &overlay->texture_uv);
        free(overlay);
        printf("Failed to create program\n");
        return;
    }
    
    glUseProgram(overlay->program->program);
    glGenTextures(1, &overlay->texture);
    glBindTexture(GL_TEXTURE_2D, overlay->texture);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                overlay->width, overlay->height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, overlay->data);

    
    gl_texture_init_defaults(overlay->texture, GL_TEXTURE_2D);
    if (overlay->format == IMAGE_FORMAT_RGBA)
    {
        stbi_image_free(overlay->data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

static bool update_image_overlay_texture(FFVARendererEGL* rnd, int index, 
                                        const unsigned char* data)
{
    if (index < 0 || index >= rnd->num_overlays)
        return false;
    image_overlay* overlay = rnd->overlays[index];
    if (!overlay)
        return false;

    if (overlay->format == IMAGE_FORMAT_RGB565) 
    {
        memcpy(overlay->data, data, overlay->width * overlay->height * 2);
    }
    return true;
}

static void matrix_multiply(GLfloat *result, GLfloat *a, GLfloat *b)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i*4+j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[i*4+j] += a[i*4+k] * b[k*4+j];
            }
        }
    }
}

static void render_image_overlay(FFVARendererEGL* rnd, FFVASurface *s, image_overlay* overlay,
        const VARectangle *src_rect, const VARectangle *dst_rect) 
{
    EglContext* const egl = &rnd->egl_context;
    if (overlay->width == 0 || overlay->height == 0)
    {
        return;
    }
    if (!overlay->program)
    {
        create_image_overlay_context(overlay);
    }
    EglProgram *program;
    GLfloat x0, y0, x1, y1;
    GLfloat texcoords[4][2];
    GLfloat positions[4][2];
    uint32_t i;

    float aspect_ratio = (float)dst_rect->width / (float)dst_rect->height;

    int overlay_w = overlay->crop_x1 - overlay->crop_x0;
    int overlay_h = overlay->crop_y1 - overlay->crop_y0;

    // Source coords in VA surface
    x0 = (GLfloat)overlay->crop_x0 / overlay->width;
    y0 = (GLfloat)overlay->crop_y0 / overlay->height;
    x1 = (GLfloat)overlay->crop_x1 / overlay->width;;
    y1 = (GLfloat)overlay->crop_y1 / overlay->height;
    texcoords[0][0] = x0; texcoords[0][1] = y1;
    texcoords[1][0] = x1; texcoords[1][1] = y1;
    texcoords[2][0] = x1; texcoords[2][1] = y0;
    texcoords[3][0] = x0; texcoords[3][1] = y0;

    // Target coords in EGL surface
    x0 =  2.0f * ((GLfloat)overlay->pos_x - (GLfloat)overlay_w / dst_rect->width / 2) - 1.0f;
    y0 =  2.0f * ((GLfloat)overlay->pos_y - (GLfloat)overlay_h / dst_rect->height / 2) - 1.0f;
    x1 =  2.0f * ((GLfloat)overlay->pos_x + (GLfloat)overlay_w / dst_rect->width / 2) - 1.0f;
    y1 =  2.0f * ((GLfloat)overlay->pos_y + (GLfloat)overlay_h / dst_rect->height / 2) - 1.0f;
    positions[0][0] = x0; positions[0][1] = y0;
    positions[1][0] = x1; positions[1][1] = y0;
    positions[2][0] = x1; positions[2][1] = y1;
    positions[3][0] = x0; positions[3][1] = y1;

    GLfloat model[16], model_r[16];
    matrix_set_identity(model);

    model[0] = overlay->scale;
    model[5] = overlay->scale;
    float cos_rot = cos(overlay->rotation);
    float sin_rot = sin(overlay->rotation);

    GLfloat rot[16] = {
        cos_rot, -sin_rot * aspect_ratio, 0, 0,
        sin_rot / aspect_ratio, cos_rot,  0, 0,
        0,       0,        1, 0,
        0,       0,        0, 1
    };
    matrix_multiply(model_r, rot, model);

    program = overlay->program;

    if (program) {
        glUseProgram(program->program);
        glUniformMatrix4fv(program->proj_uniform, 1, GL_FALSE, egl->proj);
        int model_uniform = glGetUniformLocation(program->program, "model");
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, model_r);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, positions);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, overlay->texture);
    if (program)
        glUniform1i(program->tex_uniforms[0], 0);
    if (overlay->format == IMAGE_FORMAT_RGB565)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                    overlay->width, overlay->height,  
                    0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, overlay->data);
    }
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    if (program)
        glUseProgram(0);
}

static bool
renderer_init(FFVARendererEGL *rnd, uint32_t flags)
{
    EglContext * const egl = &rnd->egl_context;

    if (!ensure_native_renderer(rnd, flags))
        return false;
    if (!ensure_display(rnd))
        return false;

    matrix_set_identity(egl->proj);
    va_image_init_defaults(&rnd->va_image);
    rnd->va_mem_type = get_va_mem_type(flags);
    ffva_surface_init_defaults(&rnd->mesa_surface);

    switch (flags & FFVA_RENDERER_EGL_MEM_TYPE_MASK) {
    case FFVA_RENDERER_EGL_MEM_TYPE_MESA_TEXTURE:
        rnd->use_mesa_texture = true;
        // fall-through
    case FFVA_RENDERER_EGL_MEM_TYPE_MESA_IMAGE:
        rnd->use_mesa_image = true;
        break;
    }

    if (rnd->use_mesa_image) {
        rnd->mesa_filter = ffva_filter_new(rnd->base.display);
        if (!rnd->mesa_filter)
            return false;
    }
    return true;
}

static void
renderer_clear_images(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    uint32_t i;

    if (egl->num_images < 1)
        return;

    for (i = 0; i < egl->num_images; i++) {
        if (egl->images[i] == EGL_NO_IMAGE_KHR)
            continue;
        egl->vtable.egl_destroy_image_khr(egl->display, egl->images[i]);
        egl->images[i] = EGL_NO_IMAGE_KHR;
    }
    egl->num_images = 0;
}

static void
renderer_clear_textures(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;

    if (egl->num_textures < 1)
        return;

    glDeleteTextures(egl->num_textures, egl->textures);
    egl->num_textures = 0;
}

static void
renderer_finalize(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    av_log(rnd, AV_LOG_ERROR,"renderer_finalize----------------------- 0");
    if (egl->display)
        eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
            EGL_NO_CONTEXT);

    egl_program_freep(&egl->program);

    renderer_clear_images(rnd);
    renderer_clear_textures(rnd);

    if (egl->surface) {
        eglDestroySurface(egl->display, egl->surface);
        egl->surface = NULL;
    }

    if (egl->context) {
        eglDestroyContext(egl->display, egl->context);
        egl->context = NULL;
    }

    if (egl->display) {
        eglTerminate(egl->display);
        egl->display = NULL;
    }
    ffva_renderer_freep(&rnd->native_renderer);

    ffva_filter_freep(&rnd->mesa_filter);
    va_destroy_surface(rnd->va_display, &rnd->mesa_surface.id);
}

static void image_overlay_set_transform(image_overlay* overlay, 
                                        float x, float y, 
                                        float scale, float rotation) 
{
    if (overlay->format == IMAGE_FORMAT_RGB565)
    {
        return;
    }
    overlay->pos_x = x;
    overlay->pos_y = y;
    overlay->scale = scale;
    overlay->rotation = rotation;
}

static void render_text(FFVARendererEGL* rnd, FFVASurface *s, text_overlay* text,
    const VARectangle *src_rect, const VARectangle *dst_rect) 
{
    GLfloat cur_x = 0;
    GLfloat cur_y = 0;
    for(int i = 0; i < text->num_chars; i++)
    {
        GLfloat pos_x = text->pos_x + (text->chars[i]->Bearing_left + cur_x) / dst_rect->width;
        GLfloat pos_y = text->pos_y - (text->chars[i]->Size_h - text->chars[i]->Bearing_top - cur_y) / dst_rect->height;
        cur_x += (text->chars[i]->Advance >> 6);
        if (text->chars[i]->c == '\n')
        {
            cur_x = 0;
            cur_y -= text->font_size;
        }
        if (text->chars[i]->c  < 32)
        {
            continue;
        }
        image_overlay_set_transform(text->chars[i]->image, pos_x, pos_y, 1.0, 0.0);
        render_image_overlay(rnd, s, text->chars[i]->image, src_rect, dst_rect);
    }
}


static uintptr_t
renderer_get_visual_id(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;

    if (!ensure_config(rnd))
        return 0;
    return egl->visualid;
}

static bool
renderer_get_size(FFVARendererEGL *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr)
{
    if (!rnd->native_renderer)
        return false;
    return ffva_renderer_get_size(rnd->native_renderer, width_ptr, height_ptr);
}

static bool
renderer_set_size(FFVARendererEGL *rnd, uint32_t width, uint32_t height)
{
    EglContext * const egl = &rnd->egl_context;
    av_log(rnd, AV_LOG_ERROR, "egl renderer_set_size ------------------------------\n");
    if (!rnd->native_renderer)
        return false;
    if (!ffva_renderer_set_size(rnd->native_renderer, width, height))
        return false;

    if (!ensure_context(rnd))
        return false;
    // glViewport(0, 0, width, height);
    egl->surface_width = width;
    egl->surface_height = height;
    return true;
}

static void
renderer_resize(FFVARendererEGL *rnd, int x, int y, uint32_t width, uint32_t height)
{
    av_log(rnd, AV_LOG_ERROR, "egl renderer_resize ------------------------------\n");
    if (!rnd->native_renderer)
        return;
    ffva_renderer_resize(rnd->native_renderer, x, y, width, height);
    return;
}

static void
renderer_set_shader_text(FFVARendererEGL *rnd, const char *frag_shader_text,
    const char *vert_shader_text)
{
    EglContext * const egl = &rnd->egl_context;

    if (!vert_shader_text)
        vert_shader_text = vert_shader_text_default;

    if (egl->frag_shader_text != frag_shader_text) {
        egl->frag_shader_text = frag_shader_text;
        egl->program_changed = true;
    }

    if (egl->vert_shader_text != vert_shader_text) {
        egl->vert_shader_text = vert_shader_text;
        egl->program_changed = true;
    }
}

// Creates GL textures and binds them to the existing EGGL images
static bool
renderer_bind_textures(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    GLuint texture;
    uint32_t i;

    for (i = 0; i < egl->num_images; i++) {
        glGenTextures(1, &texture);
        glBindTexture(egl->tex_target, texture);
        gl_texture_init_defaults(texture, egl->tex_target);
        egl->vtable.gl_egl_image_target_texture2d_oes(egl->tex_target,
            egl->images[i]);
        glBindTexture(egl->tex_target, 0);
        egl->textures[egl->num_textures++] = texture;
    }
    return true;
}

static bool
renderer_bind_dma_buf(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    VAImage * const va_image = &rnd->va_image;
    VABufferInfo * const va_buf_info = &rnd->va_buf_info;
    EGLImageKHR image;
    GLint attribs[23], *attrib;
    uint32_t i, num_fds = 0;
    uint32_t drm_format;
    int fds[3];
#if 0
#if USE_GLES_VERSION != 0

    if (va_format_to_drm_format(&va_image->format, &drm_format)) {
        attrib = attribs;
        *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
        *attrib++ = drm_format;
        *attrib++ = EGL_WIDTH;
        *attrib++ = va_image->width;
        *attrib++ = EGL_HEIGHT;
        *attrib++ = va_image->height;
        for (i = 0; i < va_image->num_planes; i++) {
            int fd = (intptr_t)va_buf_info->handle;
            if (EGL_image_dma_buf_import_owns_fd && (fd = dup(fd)) < 0)
                goto error_cleanup;
            fds[num_fds++] = fd;

            *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT + 3*i;
            *attrib++ = fd;
            *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT + 3*i;
            *attrib++ = va_image->offsets[i];
            *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT + 3*i;
            *attrib++ = va_image->pitches[i];
        }
        *attrib++ = EGL_NONE;

        image = egl->vtable.egl_create_image_khr(egl->display, EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL, attribs);
        if (image) {
            egl->images[egl->num_images++] = image;
            egl->tex_target = GL_TEXTURE_EXTERNAL_OES;
            renderer_set_shader_text(rnd, frag_shader_text_egl_external, NULL);
            return true;
        }

        if (EGL_image_dma_buf_import_owns_fd) {
            for (i = 0; i < num_fds; i++)
                close(fds[i]);
        }
        num_fds = 0;
    }
#endif
#endif

    for (i = 0; i < va_image->num_planes; i++) {
        int fd = (intptr_t)va_buf_info->handle;
        if (EGL_image_dma_buf_import_owns_fd && (fd = dup(fd)) < 0)
            goto error_cleanup;
        // av_log(rnd, AV_LOG_ERROR,
        //         "fd : %d   \n",
        //         fd);
        fds[num_fds++] = fd;
    }

    switch (va_image->format.fourcc) {
    case VA_FOURCC('N','V','1','2'): {
        for (i = 0; i < va_image->num_planes; i++) {
            const uint32_t is_uv_plane = i > 0;

            attrib = attribs;
            *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
            *attrib++ = is_uv_plane ? DRM_FORMAT_GR88 : DRM_FORMAT_R8;
            *attrib++ = EGL_WIDTH;
            *attrib++ = (va_image->width + is_uv_plane) >> is_uv_plane;
            *attrib++ = EGL_HEIGHT;
            *attrib++ = (va_image->height + is_uv_plane) >> is_uv_plane;
            *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
            *attrib++ = fds[i];
            *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
            *attrib++ = va_image->offsets[i];
            *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
            *attrib++ = va_image->pitches[i];
            *attrib++ = EGL_NONE;
            image = egl->vtable.egl_create_image_khr(egl->display,
                EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                attribs);
            if (!image) {
                av_log(rnd, AV_LOG_ERROR,
                    "failed to import VA buffer (NV12:%s) into EGL image\n",
                    is_uv_plane ? "UV" : "Y");
                goto error_cleanup;
            }
            egl->images[egl->num_images++] = image;
        }
        renderer_set_shader_text(rnd, frag_shader_text_nv12, NULL);
        break;
    }
    case VA_FOURCC('I','4','2','0'):
    case VA_FOURCC('Y','V','1','2'): {
        const uint32_t swap_uv_planes =
            va_image->format.fourcc != VA_FOURCC('I','4','2','0');

        for (i = 0; i < va_image->num_planes; i++) {
            const uint32_t is_uv_plane = i > 0;
            const uint32_t p = i ^ (3 & -(is_uv_plane & swap_uv_planes));

            attrib = attribs;
            *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
            *attrib++ = DRM_FORMAT_R8;
            *attrib++ = EGL_WIDTH;
            *attrib++ = (va_image->width + is_uv_plane) >> is_uv_plane;
            *attrib++ = EGL_HEIGHT;
            *attrib++ = (va_image->height + is_uv_plane) >> is_uv_plane;
            *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
            *attrib++ = fds[i];
            *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
            *attrib++ = va_image->offsets[p];
            *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
            *attrib++ = va_image->pitches[p];
            *attrib++ = EGL_NONE;
            image = egl->vtable.egl_create_image_khr(egl->display,
                EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                attribs);
            if (!image) {
                av_log(rnd, AV_LOG_ERROR,
                    "failed to import VA buffer (%s:%c) into EGL image\n",
                    swap_uv_planes ? "YV12" : "I420",
                    p == 0 ? 'Y' : p == 1 ? 'U' : 'V');
                goto error_cleanup;
            }
            egl->images[egl->num_images++] = image;
        }
        renderer_set_shader_text(rnd, frag_shader_text_yuv, NULL);
        break;
    }
    case VA_FOURCC('R','G','B','A'):
    case VA_FOURCC('B','G','R','A'): {
        if (!va_format_to_drm_format(&va_image->format, &drm_format))
            goto error_unsupported_format;
        i = 0;
        attrib = attribs;
        *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
        *attrib++ = drm_format;
        *attrib++ = EGL_WIDTH;
        *attrib++ = va_image->width;
        *attrib++ = EGL_HEIGHT;
        *attrib++ = va_image->height;
        *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
        *attrib++ = fds[i];
        *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        *attrib++ = va_image->offsets[0];
        *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        *attrib++ = va_image->pitches[0];
        *attrib++ = EGL_NONE;
        image = egl->vtable.egl_create_image_khr(egl->display, EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL, attribs);

		// EGLAttrib egl_img_attributes[] = {
		// 	EGL_LINUX_DRM_FOURCC_EXT, drm_format,
		// 	EGL_WIDTH, va_image->width,
		// 	EGL_HEIGHT, va_image->height,
		// 	EGL_DMA_BUF_PLANE0_FD_EXT, fds[i],
		// 	EGL_DMA_BUF_PLANE0_OFFSET_EXT, va_image->offsets[0],
		// 	EGL_DMA_BUF_PLANE0_PITCH_EXT, va_image->pitches[0],
		// 	EGL_NONE};
		// image = eglCreateImage(egl->display, EGL_NO_CONTEXT,
		// 										 EGL_LINUX_DMA_BUF_EXT, NULL, egl_img_attributes);

		if (!image) {
            av_log(rnd, AV_LOG_ERROR,
                "failed to import VA buffer (%.4s) into EGL image\n",
                (char *)&va_image->format.fourcc);
            goto error_cleanup;
        }
        egl->images[egl->num_images++] = image;
        renderer_set_shader_text(rnd, frag_shader_text_rgba, NULL);
        break;
    }
    default:
        goto error_unsupported_format;
    }

#if 0
#if USE_GLES_VERSION == 0
    egl->tex_target = GL_TEXTURE_2D;
#else
    egl->tex_target = GL_TEXTURE_EXTERNAL_OES;
#endif
#endif
egl->tex_target = GL_TEXTURE_2D;
    return true;

    /* ERRORS */
error_unsupported_format:
    av_log(rnd, AV_LOG_ERROR, "unsupported VA buffer format %.4s\n",
        (char *)&va_image->format.fourcc);
    // fall-through
error_cleanup:
    if (EGL_image_dma_buf_import_owns_fd) {
        for (i = 0; i < num_fds; i++){
              av_log(rnd, AV_LOG_ERROR, "error_cleanup close fd: %d  \n",fds[i]);
            close(fds[i]);
        }
    }
    return false;
}

static bool
renderer_bind_gem_buf(FFVARendererEGL *rnd)
{
    EglContext * const egl = &rnd->egl_context;
    VAImage * const va_image = &rnd->va_image;
    VABufferInfo * const va_buf_info = &rnd->va_buf_info;
    EGLImageKHR image;
    GLint attribs[23], *attrib;
    
    switch (va_image->format.fourcc) {
    case VA_FOURCC('B','G','R','A'): {
        attrib = attribs;
        *attrib++ = EGL_DRM_BUFFER_FORMAT_MESA;
        *attrib++ = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
        *attrib++ = EGL_WIDTH;
        *attrib++ = va_image->width;
        *attrib++ = EGL_HEIGHT;
        *attrib++ = va_image->height;
        *attrib++ = EGL_DRM_BUFFER_STRIDE_MESA;
        *attrib++ = va_image->pitches[0] / 4;
        *attrib++ = EGL_NONE;
        image = egl->vtable.egl_create_image_khr(egl->display, EGL_NO_CONTEXT,
            EGL_DRM_BUFFER_MESA, (EGLClientBuffer)va_buf_info->handle, attribs);
        if (!image) {
            av_log(rnd, AV_LOG_ERROR,
                "failed to import VA buffer (%.4s) into EGL image\n",
                (char *)&va_image->format.fourcc);
            return false;
        }
        egl->images[egl->num_images++] = image;
        renderer_set_shader_text(rnd, frag_shader_text_rgba, NULL);
        break;
    }
    default:
        goto error_unsupported_format;
    }

    egl->tex_target = GL_TEXTURE_2D;
    return true;

    /* ERRORS */
error_unsupported_format:
    av_log(rnd, AV_LOG_ERROR, "unsupported VA buffer format %.4s\n",
        (char *)&va_image->format.fourcc);
    return false;
}

static bool
renderer_bind_mesa_image(FFVARendererEGL *rnd, FFVASurface *s)
{
    EglContext * const egl = &rnd->egl_context;
    EglVTable * const vtable = &egl->vtable;
    FFVASurface * const d = &rnd->mesa_surface;
    VASurfaceAttribExternalBuffers va_extbuf;
    unsigned long va_extbuf_handle;
    VASurfaceAttrib va_attribs[2], *va_attrib;
    VASurfaceID va_surface;
    uint32_t va_fourcc;
    VAStatus va_status;
    EGLImageKHR image;
    EGLint name, stride;
    GLint attribs[23], *attrib;
    GLuint texture;
    int ret;

    if (s->width != d->width || s->height != d->height) {
        renderer_clear_images(rnd);
        renderer_clear_textures(rnd);
        va_destroy_surface(rnd->va_display, &rnd->mesa_surface.id);

        if (rnd->use_mesa_texture) {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            gl_texture_init_defaults(texture, GL_TEXTURE_2D);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->width, s->height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
            egl->textures[egl->num_textures++] = texture;

            attrib = attribs;
            *attrib++ = EGL_IMAGE_PRESERVED_KHR;
            *attrib++ = EGL_TRUE;
            *attrib++ = EGL_NONE;
            image = vtable->egl_create_image_khr(egl->display, egl->context,
                EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(uintptr_t)texture,
                attribs);
            if (!image)
                goto error_create_image;
            egl->images[egl->num_images++] = image;
            va_fourcc = VA_FOURCC('R','G','B','A');
        }
        else {
            attrib = attribs;
            *attrib++ = EGL_DRM_BUFFER_FORMAT_MESA;
            *attrib++ = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
            *attrib++ = EGL_WIDTH;
            *attrib++ = s->width;
            *attrib++ = EGL_HEIGHT;
            *attrib++ = s->height;
            *attrib++ = EGL_DRM_BUFFER_USE_MESA;
            *attrib++ = EGL_DRM_BUFFER_USE_SHARE_MESA;
            *attrib++ = EGL_NONE;
            image = vtable->egl_create_drm_image_mesa(egl->display, attribs);
            if (!image)
                goto error_create_image;
            egl->images[egl->num_images++] = image;
            va_fourcc = VA_FOURCC('B','G','R','A');
        }

        if (!vtable->egl_export_drm_image_mesa(egl->display, image, &name,
                NULL, &stride))
            goto error_export_image;

        va_attrib = va_attribs;
        va_attrib->type = VASurfaceAttribExternalBufferDescriptor;
        va_attrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
        va_attrib->value.type = VAGenericValueTypePointer;
        va_attrib->value.value.p = &va_extbuf;
        va_attrib++;
        va_attrib->type = VASurfaceAttribMemoryType;
        va_attrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
        va_attrib->value.type = VAGenericValueTypeInteger;
        va_attrib->value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
        va_attrib++;

        va_extbuf_handle = name;
        va_extbuf.pixel_format = va_fourcc;
        va_extbuf.width = s->width;
        va_extbuf.height = s->height;
        va_extbuf.data_size = va_extbuf.height * stride;
        va_extbuf.num_planes = 1;
        va_extbuf.pitches[0] = stride;
        va_extbuf.offsets[0] = 0;
        va_extbuf.buffers = &va_extbuf_handle;
        va_extbuf.num_buffers = 1;
        va_extbuf.flags = 0;
        va_extbuf.private_data = NULL;
        va_status = vaCreateSurfaces(rnd->va_display, VA_RT_FORMAT_RGB32,
            s->width, s->height, &va_surface, 1, va_attribs,
            va_attrib - va_attribs);
        if (!va_check_status(va_status, "vaCreateSurfaces()"))
            goto error_create_surface;
        ffva_surface_init(d, va_surface, VA_RT_FORMAT_RGB32,
            s->width, s->height);
    }

    ret = ffva_filter_process(rnd->mesa_filter, s, d, 0);
    if (ret != 0)
        goto error_transfer_surface;

    egl->tex_target = GL_TEXTURE_2D;
    renderer_set_shader_text(rnd, frag_shader_text_rgba, NULL);
    return true;

    /* ERRORS */
error_create_image:
    av_log(rnd, AV_LOG_ERROR, "failed to create Mesa DRM image of size %ux%d\n",
        s->width, s->height);
    return false;
error_export_image:
    av_log(rnd, AV_LOG_ERROR, "failed to export Mesa DRM image %p\n", image);
    return false;
error_create_surface:
    av_log(rnd, AV_LOG_ERROR, "failed to create Mesa VA surface\n");
    return false;
error_transfer_surface:
    av_log(rnd, AV_LOG_ERROR, "failed to transfer VA surface to Mesa image\n");
    return false;
}

static bool
renderer_bind_surface(FFVARendererEGL *rnd, FFVASurface *s)
{
    VAStatus va_status;

    if (rnd->use_mesa_image)
        return renderer_bind_mesa_image(rnd, s);

    va_image_init_defaults(&rnd->va_image);
    // av_log(NULL, AV_LOG_ERROR, "renderer_bind_surface va_surface : %d    \n", s->id);
    va_status = vaDeriveImage(rnd->va_display, s->id, &rnd->va_image);
    if (!va_check_status(va_status, "vaDeriveImage()"))
        return false;

    memset(&rnd->va_buf_info, 0, sizeof(rnd->va_buf_info));
    rnd->va_buf_info.mem_type = rnd->va_mem_type;
    va_status = vaAcquireBufferHandle(rnd->va_display, rnd->va_image.buf,
        &rnd->va_buf_info);
    if (!va_check_status(va_status, "vaAcquireBufferHandle()"))
        return false;

    switch (rnd->va_buf_info.mem_type) {
    case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME:
        if (!renderer_bind_dma_buf(rnd))
            return false;
        break;
    case VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM:
        if (!renderer_bind_gem_buf(rnd))
            return false;
        break;
    default:
        av_log(rnd, AV_LOG_ERROR, "unsupported VA buffer memory (%.4s)\n",
            (char *)&rnd->va_buf_info.mem_type);
        return false;
    }
    return true;
}

static bool
renderer_unbind_mesa_image(FFVARendererEGL *rnd)
{
    return true;
}

static bool
renderer_unbind_surface(FFVARendererEGL *rnd)
{
    VAStatus va_status;
    uint32_t has_errors = 0;

    if (rnd->use_mesa_image)
        return renderer_unbind_mesa_image(rnd);

    if (rnd->va_buf_info.mem_size > 0) {
        va_status = vaReleaseBufferHandle(rnd->va_display, rnd->va_image.buf);
        if (!va_check_status(va_status, "vaReleaseBufferHandle()"))
            has_errors++;
        rnd->va_buf_info.mem_size = 0;
    }

    if (rnd->va_image.image_id != VA_INVALID_ID) {
        va_status = vaDestroyImage(rnd->va_display, rnd->va_image.image_id);
        if (!va_check_status(va_status, "vaDestroyImage()"))
            has_errors++;
        va_image_init_defaults(&rnd->va_image);
    }
    return !has_errors;
}

static bool
renderer_redraw(FFVARendererEGL *rnd, FFVASurface *s,
    const VARectangle *src_rect, const VARectangle *dst_rect)
{
    bool no_video = false;
    EglContext * const egl = &rnd->egl_context;
    EglProgram *program;
    GLfloat x0, y0, x1, y1;
    GLfloat texcoords[4][2];
    GLfloat positions[4][2];
    if (rnd->num_overlays > 0)
    {
        if (rnd->overlays[0]->format == IMAGE_FORMAT_RGB565)
        {
            no_video = true;
        }
    }
    if (no_video)
    {
        rnd->egl_context.target_width = rnd->overlays[0]->width;
        rnd->egl_context.target_height = rnd->overlays[0]->height;
        float scale = (float)dst_rect->width / rnd->overlays[0]->width;
        rnd->overlays[0]->scale = scale;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glViewport(0, 0, dst_rect->width, dst_rect->height);
        goto NO_VIDEO;
    }
    rnd->egl_context.target_width = dst_rect->width;
    rnd->egl_context.target_height = dst_rect->height;
    uint32_t i;

    // Source coords in VA surface
    x0 = (GLfloat)src_rect->x / s->width;
    y0 = (GLfloat)src_rect->y / s->height;
    x1 = (GLfloat)(src_rect->x + src_rect->width) / s->width;
    y1 = (GLfloat)(src_rect->y + src_rect->height) / s->height;
    texcoords[0][0] = x0; texcoords[0][1] = y1;
    texcoords[1][0] = x1; texcoords[1][1] = y1;
    texcoords[2][0] = x1; texcoords[2][1] = y0;
    texcoords[3][0] = x0; texcoords[3][1] = y0;

    // Target coords in EGL surface
    x0 = -1.0f;
    y1 =  1.0f;
    x1 =  1.0f;
    y0 = -1.0f;
    positions[0][0] = x0; positions[0][1] = y0;
    positions[1][0] = x1; positions[1][1] = y0;
    positions[2][0] = x1; positions[2][1] = y1;
    positions[3][0] = x0; positions[3][1] = y1;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, dst_rect->width, dst_rect->height);

#if USE_GLES_VERSION == 1
    glBindTexture(egl->tex_target, egl->textures[0]);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, positions);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    program = egl->program;
    if (egl->program_changed) {
        egl_program_freep(&egl->program);
        egl->program_changed = false;

        program = egl_program_new(egl->frag_shader_text, egl->vert_shader_text);
        if (!program)
            return false;
        egl->program = program;
    }

    if (program) {
        glUseProgram(program->program);
        glUniformMatrix4fv(program->proj_uniform, 1, GL_FALSE, egl->proj);
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, positions);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

    for (i = 0; i < egl->num_textures; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(egl->tex_target, egl->textures[i]);
        if (program)
            glUniform1i(program->tex_uniforms[i], i);
    }
    if (program)
    {
        glUniform2f(program->center_uniform, rnd->center_x, rnd->center_y);
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    if (program)
        glUseProgram(0);
#endif
NO_VIDEO:
    for (int i = 0; i < rnd->num_overlays; i++) 
    {
        render_image_overlay(rnd, s, rnd->overlays[i],src_rect, dst_rect);
    }
    for (int i = 0; i < rnd->num_text_overlays; i++)
    {
        render_text(rnd, s, rnd->text_overlays[i],src_rect, dst_rect);
    }
    eglSwapBuffers(egl->display, egl->surface);
    return true;
}

static bool
renderer_put_surface(FFVARendererEGL *rnd, FFVASurface *surface,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags)
{
    uint32_t has_errors = 0;

    if (!rnd->use_mesa_image)
        renderer_clear_images(rnd);
    if (!rnd->use_mesa_texture)
        renderer_clear_textures(rnd);

	/*before display, need to syncsurface*/
	vaSyncSurface(rnd->va_display, surface->id);

    if (!renderer_bind_surface(rnd, surface)) {
        av_log(rnd, AV_LOG_ERROR, "failed to bind VA surface 0x%08x\n",
            surface->id);
        has_errors++;
    }

    if (!rnd->use_mesa_texture && !renderer_bind_textures(rnd)) {
        av_log(rnd, AV_LOG_ERROR, "failed to bind GL textures\n");
        has_errors++;
    }

    if (!renderer_redraw(rnd, surface, src_rect, dst_rect)) {
        av_log(rnd, AV_LOG_ERROR, "failed to redraw EGL surface\n");
        has_errors++;
    }

    if (!renderer_unbind_surface(rnd)) {
        av_log(rnd, AV_LOG_ERROR, "failed to unbind VA surface 0x%08x\n",
            surface->id);
        has_errors++;
    }
    return !has_errors;
}

static wchar_t *ToWchar(const char *s)
{
    const char *locale = "zh_CN.utf8";
    const char *src = s;
    if (src == NULL)
    {
        return NULL;
    }

    // 根据环境变量设置locale
    setlocale(LC_CTYPE, locale);
    // 得到转化为需要的宽字符大小
    int w_size = mbstowcs(NULL, src, 0) + 1;
    // w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能使locale
    // 没有设置正确)
    if (w_size == 0)
    {
        return NULL;
    }
    wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * (w_size + 1));
    int ret = mbstowcs(dest, src, strlen(src));
    dest[w_size] = 0;
    if (ret <= 0)
    {
        return NULL;
    }

    int l = 0;
    while (dest[l] != 0)
    {
        l++;
    }
    return dest;
}

static bool
init_text_overlay(text_overlay *text, const char *font, unsigned int font_size)
{
    if (text->inited)
    {
        return true;
    }
    if (FT_Init_FreeType(&text->ft) != 0)
    {
        printf("Failed to init freetype\n");
        return false;
    }
    if (FT_New_Face(text->ft, font, 0, &text->face))
    {
        printf("Failed to load font %s\n", font);
        return false;
    }
    FT_Set_Pixel_Sizes(text->face, 0, font_size);
    text->num_chars = 0;
    text->inited = true;
    text->font_size = font_size;
}

static bool
set_text_overlay(FFVARendererEGL *rnd, text_overlay *text, const char *text_str, float x, float y)
{
    wchar_t *wstr = ToWchar(text_str);
    // FFVA
    int pos = 0;
    if (!wstr)
    {
        return false;
    }
    GLfloat cur_x = x;
    GLfloat cur_y = y;
    text->pos_x = x;
    text->pos_y = y;
    while (wstr[pos] != 0)
    {
        FT_Error err = FT_Load_Char(text->face, wstr[pos], FT_LOAD_RENDER);
        if (err)
        {
            return false;
        }
        FT_Render_Glyph(text->face->glyph, FT_RENDER_MODE_NORMAL);
        image_overlay* font_image = (image_overlay*)calloc(1, sizeof(image_overlay));
        font_image->width = text->face->glyph->bitmap.width;
        font_image->height = text->face->glyph->bitmap.rows;
        font_image->crop_x0 = 0;
        font_image->crop_x1 = font_image->width;
        font_image->crop_y0 = 0;
        font_image->crop_y1 = font_image->height;
        font_image->scale = 1.0;
        font_image->rotation = 0.0;
        font_image->data = (unsigned char*)malloc(font_image->width * font_image->height * 4);
        for (int i = 0; i < font_image->height; i++)
        {
            for (int j = 0; j < font_image->width; j++)
            {
                font_image->data[i * font_image->width * 4 + j * 4 + 0] = 255;
                font_image->data[i * font_image->width * 4 + j * 4 + 1] = 255;
                font_image->data[i * font_image->width * 4 + j * 4 + 2] = 255;
                font_image->data[i * font_image->width * 4 + j * 4 + 3] = text->face->glyph->bitmap.buffer[i * font_image->width + j];
            }
        }
        char_font *charf = (char_font*)calloc(1, sizeof(char_font));
        charf->c = wstr[pos];
        charf->image = font_image;
        charf->Size_w = font_image->width;
        charf->Size_h = font_image->height;
        charf->Bearing_left = text->face->glyph->bitmap_left;
        charf->Bearing_top = text->face->glyph->bitmap_top;
        charf->Advance = text->face->glyph->advance.x;
        text->chars = (char_font **)realloc(text->chars, (text->num_chars + 1) * sizeof(char_font*));
        text->chars[text->num_chars] = charf;
        image_overlay_set_transform(text->chars[text->num_chars]->image, x, y, 1.0, 0.0);
        text->num_chars = text->num_chars + 1;
        cur_x += (charf->Advance >> 6);
        pos++;
    }
}

static bool renderer_adjust_image(FFVARendererEGL *rnd, int image_id, float x, float y, float scale, float rotation)
{
    if (!rnd) 
        return false;
    if (image_id >= rnd->num_overlays)
    {
        return false;
    }
    image_overlay_set_transform(rnd->overlays[image_id], x, y, scale, rotation);
    return true;
}

static void renderer_set_center(FFVARendererEGL *rnd, float x, float y)
{
    if (!rnd) 
        return;
    rnd->center_x = x;
    rnd->center_y = y;
    return;
}

static int renderer_load_image(FFVARendererEGL *rnd, const char *image_path, float x, float y, float scale, float rotation)
{
    if (!rnd) 
        return -1;
    rnd->overlays = (image_overlay **)realloc(rnd->overlays, (rnd->num_overlays + 1) * sizeof(image_overlay*));
    image_overlay* overlay;
    if (image_path == NULL)
    {
        overlay = image_overlay_create_yuv422((int)x, int(y));
    }
    else
    {
        overlay = image_overlay_create(image_path);
    }
    if (!overlay)
    {
        av_log(rnd, AV_LOG_ERROR, "failed to create image overlay\n");
        return -1;
    }
    rnd->overlays[rnd->num_overlays] = overlay;
    image_overlay_set_transform(rnd->overlays[rnd->num_overlays], x, y, scale, rotation);
    rnd->num_overlays = rnd->num_overlays + 1;
    return rnd->num_overlays - 1;
}

static int renderer_load_text(FFVARendererEGL *rnd, const char *font_path, const char *text_context, int font_size, float x, float y)
{
    if (!rnd) 
        return -1;
    rnd->text_overlays = (text_overlay **)realloc(rnd->text_overlays, (rnd->num_text_overlays + 1) * sizeof(text_overlay*));
    text_overlay *text = (text_overlay*)calloc(1, sizeof(text_overlay));
    init_text_overlay(text, font_path, font_size);
    set_text_overlay(rnd, text, text_context, x, y);
    if (!text)
    {
        av_log(rnd, AV_LOG_ERROR, "failed to create image overlay\n");
        return -1;
    }
    rnd->text_overlays[rnd->num_text_overlays] = text;
    rnd->num_text_overlays = rnd->num_text_overlays + 1;
    return rnd->num_text_overlays - 1;
}

static const FFVARendererClass *
ffva_renderer_egl_class(void)
{
    static const FFVARendererClass g_class = {
        .base = {
            .class_name = "FFVARendererEGL",
            .item_name  = av_default_item_name,
            .option     = NULL,
            .version    = LIBAVUTIL_VERSION_INT,
        },
        .size           = sizeof(FFVARendererEGL),
        .type           = FFVA_RENDERER_TYPE_EGL,
        .init           = (FFVARendererInitFunc)renderer_init,
        .finalize       = (FFVARendererFinalizeFunc)renderer_finalize,
        .get_visual_id  = (FFVARendererGetVisualIdFunc)renderer_get_visual_id,
        .get_size       = (FFVARendererGetSizeFunc)renderer_get_size,
        .set_size       = (FFVARendererSetSizeFunc)renderer_set_size,
        .put_surface    = (FFVARendererPutSurfaceFunc)renderer_put_surface,
        .renderer_load_image = (FFVARendererLoadImageFunc)renderer_load_image,
        .renderer_load_text = (FFVARendererLoadTextFunc)renderer_load_text,
        .renderer_adjust_image = (FFVARendererAdjustImageFunc)renderer_adjust_image,
        .renderer_set_center = (FFVARendererSetCenterFunc)renderer_set_center,
        .renderer_resize = (FFVARendererResizeFunc)renderer_resize,
        .renderer_update_image = (FFVARendererUpdateImageFunc)update_image_overlay_texture,
    };
    return &g_class;
}

// Creates a new renderer object from the supplied VA display
FFVARenderer *
ffva_renderer_egl_new(FFVADisplay *display, uint32_t flags)
{
    return ffva_renderer_new(ffva_renderer_egl_class(), display, flags);
}