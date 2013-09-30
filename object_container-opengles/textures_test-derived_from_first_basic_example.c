#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <bcm_host.h>

#include "object_container.h"
#include "keyboard.h"

#define TEXTURE_WIDTH 6
#define TEXTURE_HEIGHT 4
#define PIXEL_FORMAT_SIZE 4 /* rgba is the only pixel fmt supp by ob_ct */
#define TEXTURE_DIMENSION_NUMBER 2 /* only 2d text supp by object_container */

char *generate_texture(){
    char *texture_buffer;
    char color_r[TEXTURE_WIDTH * TEXTURE_HEIGHT];
    char color_g[TEXTURE_WIDTH * TEXTURE_HEIGHT];
    char color_b[TEXTURE_WIDTH * TEXTURE_HEIGHT];
    char color_1[PIXEL_FORMAT_SIZE];
    char color_2[PIXEL_FORMAT_SIZE];
    char color_3[PIXEL_FORMAT_SIZE];
    int i;
    texture_buffer = malloc(sizeof(char) * TEXTURE_WIDTH * TEXTURE_HEIGHT *
                                 PIXEL_FORMAT_SIZE);

    color_1[0] = 0xff;
    color_1[1] = 0xff;
    color_1[2] = 0xff;

    color_2[0] = 0xff;
    color_2[1] = 0x00;
    color_2[2] = 0x00;

    color_3[0] = 0x00;
    color_3[1] = 0x00;
    color_3[2] = 0xff;

#define color(i, c) \
    color_r[i] = c[0]; \
    color_g[i] = c[1]; \
    color_b[i] = c[2]
    for (i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT ; i++) {
        if (i % TEXTURE_WIDTH == 0) { /* first column */
            color(i, color_2);
            printf(" 2");
        }
        if (i % TEXTURE_WIDTH == 1) { /* second column */
            color(i, color_2);
            printf(" 2");
        }
        if (i % TEXTURE_WIDTH == 2) {
            color(i, color_2);
            printf(" 2");
        }
        if (i % TEXTURE_WIDTH == 3) {
            color(i, color_3);
            printf(" 3");
        }
        if (i % TEXTURE_WIDTH == 4) {
            color(i, color_3);
            printf(" 3");
        }
        if (i % TEXTURE_WIDTH == 5) {
            color(i, color_3);
            printf(" 3\n");
        }
    }
    printf("\n");

#define texture_buffer_rgba_color(i) \
    texture_buffer[4*i + 0] = color_r[i]; \
    texture_buffer[4*i + 1] = color_g[i]; \
    texture_buffer[4*i + 2] = color_b[i]; \
    texture_buffer[4*i + 3] = 0xff; 

    for (i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT ; i++) {
        texture_buffer_rgba_color(i);
    }

    return texture_buffer;
}

struct state {
	uint32_t screen_width;
	uint32_t screen_height;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	int texture_width;
	int texture_height;
};

static void ogl_init(struct state *state)
{
	int32_t success;
	EGLBoolean result;
	EGLint num_config;
	static EGL_DISPMANX_WINDOW_T nativewindow;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	static const EGLint attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE,
	};

	EGLConfig config;

	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display != EGL_NO_DISPLAY);

	result = eglInitialize(state->display, NULL, NULL);
	assert(result != EGL_FALSE);

	/* Get an appropriate EGL frame buffer config. */
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(result != EGL_FALSE);

	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
	assert(state->context != EGL_NO_CONTEXT);

	success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
	assert(success >= 0);

	/* Prepare VideoCore specific nativewindow to create a window surface. */
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = state->screen_width;
	dst_rect.height = state->screen_height;

	dispman_display = vc_dispmanx_display_open(0 /* LCD */);
	dispman_update = vc_dispmanx_update_start(0);
	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
						   0 /* layer */, &dst_rect, 0 /* src */, &src_rect,
						   DISPMANX_PROTECTION_NONE, 0 /* alpha */,
						   0 /* clamp */, 0 /* transform */);

	nativewindow.element = dispman_element;
	nativewindow.width = state->screen_width;
	nativewindow.height = state->screen_height;

	vc_dispmanx_update_submit_sync(dispman_update);

	state->surface = eglCreateWindowSurface(state->display, config, &nativewindow, NULL);
	assert(state->surface != EGL_NO_SURFACE);

	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(result != EGL_FALSE);

	glShadeModel(GL_FLAT);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

static void ogl_exit(struct state *state)
{
	/* FIXME: what do we need to clear here? */
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(state->display, state->surface);

	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(state->display, state->surface);
	eglDestroyContext(state->display, state->context);
	eglTerminate(state->display);
}


void count_fps()
{
	/*  measuring time. gettimeofday may not be accurate accordingly to
	    http://blog.habets.pp.se/2010/09/gettimeofday-should-never-be-used-to-measure-time
	    however, rpi crashes just by declaring  "struct timespec ts_start". */
	static struct timeval time0, time1;
	static long int framecounter = 0;

	time0 = time1;
	gettimeofday(&time1, NULL);
	if ((time1.tv_sec - time0.tv_sec) >= 1) { /* once per second. */
		printf("frames drawn in the last second (approximate) = %li\n", framecounter);
		framecounter = 1;
	} else {
		framecounter++;
	}
}

int main(int argc, char **argv)
{
	GLfloat color_coords[1*3*4+1*4*4] = {
		/* purple */
		0.5f,  0.2f, 0.5f,  1.f,
		0.5f,  0.2f, 0.5f,  1.f,
		0.5f,  0.2f, 0.5f,  1.f,
		/* very light blue - rectangle color */
		0.5f,  0.5f, 0.7f,  1.f,
		0.5f,  0.5f, 0.7f,  1.f,
		0.5f,  0.5f, 0.7f,  1.f,
		0.5f,  0.5f, 0.7f,  1.f,
	};

	GLfloat vertex_coords[1*3*3+1*3*4] = {
		/* triangle */
		 0.00f,  0.055901f,  0.0f,
		-0.05f, -0.055901f,  0.0f,
		 0.05f, -0.055901f,  0.0f,
		/* rectangle */
		0.00+-0.12f, -0.12f, -1.0f,
		0.00+ 0.12f, -0.12f, -1.0f,
		0.00+ 0.12f,  0.12f, -1.0f,
		0.00+-0.12f,  0.12f, -1.0f,
	};

	GLfloat texture_coordinates[ (4+3) * TEXTURE_DIMENSION_NUMBER ] = {
			/* triangle */
			 0.5f, 1.f,
			 0.f,  0.f,
			 1.f,  0.f,
			/* rectangle */
			 0.0f,  0.0f,
			 2.0f,  0.0f,
			 2.0f,  2.0f,
			 0.0f,  2.0f, 
		};

	struct state state;
	keyboard_t *keyboard;
	int key_pressed;
	struct object *triangle, *rectangle;
	struct object_container *object_container;
	char *texture_buffer_triangle;
	char *texture_buffer_rectangle;

	/* This must be initialized before anything else related to Graphics */
	bcm_host_init();

	memset(&state, 0, sizeof(struct state));
	ogl_init(&state);

	keyboard = keyboard_open(argc > 1 ? argv[1] : "/dev/input/event0");

	/* init gl color blending and set background color */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.8f, 0.6f, 0.4f, 1.0f);

	//texture_buffer_triangle = generate_texture();
	texture_buffer_triangle = NULL;
	//texture_buffer_rectangle = NULL;
	texture_buffer_rectangle = generate_texture();

	triangle  = object_new(3, &vertex_coords[0], &color_coords[0],
                        &texture_coordinates[0], texture_buffer_triangle, 6, 4);

	rectangle = object_new(4, &vertex_coords[3 /* x,y,z */ *3 /* triangle bef. */],
        &color_coords[4 /*rgba*/ *3 /* triangle before */], &texture_coordinates[2 /*
        texture is plane */ *3 /* triangle before */], texture_buffer_rectangle,
                                                                          6, 4);
	object_container = object_container_new();
	object_container_add_object(object_container, rectangle);
	object_container_add_object(object_container, triangle);

	object_container_prepare_to_draw(object_container);

	do {
		glClear(GL_COLOR_BUFFER_BIT);
		object_container_draw_objects(object_container);
		count_fps();
		key_pressed = keyboard_read(keyboard);
		eglSwapBuffers(state.display, state.surface);
	} while (key_pressed != KEY_ESC);

	object_container_delete_objects(object_container);
	object_container_delete(object_container);
	keyboard_close(keyboard);
	ogl_exit(&state);
	return 0;
}


