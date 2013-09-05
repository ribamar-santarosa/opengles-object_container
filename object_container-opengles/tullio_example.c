/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <linux/input.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

typedef struct
{
    uint32_t screen_width;
    uint32_t screen_height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    GLuint vshader;
    GLuint fshader;
    GLuint program;
    GLuint buf;

    GLuint attr_vertex, unif_offset;
} CUBE_STATE_T;

static CUBE_STATE_T _state, *state = &_state;

#define check() assert(glGetError() == 0)

static void init_ogl(CUBE_STATE_T *state)
{
    int32_t success = 0;
    EGLBoolean result;
    EGLint num_config;

    static EGL_DISPMANX_WINDOW_T nativewindow;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    static const EGLint attribute_list[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    static const EGLint context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLConfig config;

    // Get an EGL display connection.
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(state->display != EGL_NO_DISPLAY);
    check();

    // Initialize the EGL display connection.
    result = eglInitialize(state->display, NULL, NULL);
    assert(EGL_FALSE != result);
    check();

    // Get an appropriate EGL frame buffer configuration.
    result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);
    check();

    // Get an appropriate EGL frame buffer configuration.
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    check();

    // Create an EGL rendering context.
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(state->context != EGL_NO_CONTEXT);
    check();

    // Create an EGL window surface.
    success = graphics_get_display_size(0/* LCD */, &state->screen_width, &state->screen_height);
    assert(success >= 0);

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = state->screen_width;
    dst_rect.height = state->screen_height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = state->screen_width << 16;
    src_rect.height = state->screen_height << 16;

    dispman_display = vc_dispmanx_display_open(0/* LCD */);
    dispman_update = vc_dispmanx_update_start(0);

    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
        0/*layer*/, &dst_rect, 0/*src*/,
        &src_rect, DISPMANX_PROTECTION_NONE, 0/*alpha*/, 0/*clamp*/, 0/*transform*/);

    nativewindow.element = dispman_element;
    nativewindow.width = state->screen_width;
    nativewindow.height = state->screen_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    check();

    state->surface = eglCreateWindowSurface(state->display, config, &nativewindow, NULL);
    assert(state->surface != EGL_NO_SURFACE);
    check();

    // Connect the context to the surface.
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);
    check();

    // Set background color and clear buffers.
    glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    check();
}

static void init_shaders(CUBE_STATE_T *state)
{
    static const GLfloat vertex_data[] = {
        -1.0, -1.0, 1.0, 1.0,
        1.0, -1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        -1.0, 1.0, 1.0, 1.0
    };
    const GLchar *vshader_source =
        "attribute vec4 vertex;"
        "void main(void)"
        "{"
        "    gl_Position = vertex;"
        "}";
    const GLchar *fshader_source =
        "uniform vec2 offset;"
        "void main(void)"
        "{"
        "    float x = offset.x - gl_FragColor.x;"
        "    float y = offset.y - gl_FragColor.y;"
        "    float d = sqrt(x * x + y * y);"
        "    gl_FragColor = vec4(d, d, d, 1);"
        "}";

    state->vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(state->vshader, 1, &vshader_source, 0);
    glCompileShader(state->vshader);
    check();

    state->fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(state->fshader, 1, &fshader_source, 0);
    glCompileShader(state->fshader);
    check();

    state->program = glCreateProgram();
    glAttachShader(state->program, state->vshader);
    glAttachShader(state->program, state->fshader);
    glLinkProgram(state->program);
    check();

    state->attr_vertex = glGetAttribLocation(state->program, "vertex");
    state->unif_offset = glGetUniformLocation(state->program, "offset");

    glClearColor(0.0, 1.0, 1.0, 1.0);
    glGenBuffers(1, &state->buf);
    check();

    // Prepare viewport.
    glViewport(0, 0, state->screen_width, state->screen_height);
    check();

    // Upload vertex data to a buffer.
    glBindBuffer(GL_ARRAY_BUFFER, state->buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(state->attr_vertex, 4, GL_FLOAT, 0, 16, 0);
    glEnableVertexAttribArray(state->attr_vertex);
    check();
}

static void draw_triangles(CUBE_STATE_T *state, GLfloat cx, GLfloat cy, GLfloat x, GLfloat y)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, state->buf);
    check();
    glUseProgram(state->program);
    check();
    glUniform2f(state->unif_offset, x, y);
    check();

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    check();

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glFlush();
    glFinish();
    check();

    eglSwapBuffers(state->display, state->surface);
    check();
}

static void get_key(float *outx, float *outy)
{
    static int fd = -1;
    const int width = state->screen_width, height = state->screen_height;
    static float x = 0, y = 0;

    if (fd < 0)
       fd = open("/dev/input/by-id/usb-Dell_Dell_USB_Keyboard_Hub-event-kbd", O_RDONLY);
    else {
        struct input_event ev;
        read(fd, &ev, sizeof(struct input_event));
        if (ev.type == 1)
            if (ev.value == 0) {
                switch (ev.code) {
                    case KEY_UP:
                    case KEY_W:
                        y -= 0.1;
                        break;
                    case KEY_DOWN:
                    case KEY_S:
                        y += 0.1;
                        break;
                    case KEY_LEFT:
                    case KEY_A:
                        x -= 0.1;
                        break;
                    case KEY_RIGHT:
                    case KEY_D:
                        x += 0.1;
                        break;
                    case KEY_ESC:
                        close(fd);
                        exit(0);
                        break;
                }
                if (x < 0)
                    x = 0;
                if (y < 0)
                    y = 0;
                if (x > width)
                    x = width;
                if (y > height)
                    y = height;
            }
    }
    if (outx) *outx = x;
    if (outy) *outy = y;
}

int main()
{
    GLfloat cx, cy;
    bcm_host_init();

    // Clear application state.
    memset(state, 0, sizeof(*state));

    // Start OGLES.
    init_ogl(state);
    init_shaders(state);
    cx = state->screen_width / 2;
    cy = state->screen_height / 2;

    while (1) {
        float x, y;
        get_key(&x, &y);
        draw_triangles(state, cx, cy, x, y);
    }
    return 0;
}
