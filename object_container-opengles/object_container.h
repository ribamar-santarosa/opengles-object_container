#ifndef __OBJECT_CONTAINER__H__
#define __OBJECT_CONTAINER__H__

#include <stdlib.h>
#include <GLES2/gl2.h>

struct object { /* a polygon or a solid */
	size_t num_of_vertices;
	GLfloat *vertex_coordinates;
	GLfloat *color_coordinates;
	GLfloat *texture_coordinates;
	size_t vertex_pos_on_container; /* vertex_coord ==
			      obj_cont->vertex_coord[vertex_pos_on_container] */
	size_t color_pos_on_container;
	size_t texture_pos_on_container;
	GLuint texture_object;
	char *texture_buffer;
	GLsizei texture_width, texture_height;
}; 

struct shaders_state {
   unsigned int checksum;
   GLuint verbose;
   GLuint vshader;
   GLuint fshader;
   GLuint mshader;
   GLuint program;
   GLuint program2;
   GLuint tex_fb;
   GLuint tex;
   GLuint buf;
// julia attribs
   GLuint unif_color, attr_vertex, unif_scale, unif_offset, unif_tex, unif_centre; 
// mandelbrot attribs
   GLuint attr_vertex2, unif_scale2, unif_offset2, unif_centre2;
};

struct object_linked_list {
	struct object *object;
	struct object_linked_list *next;
};

struct object_container {
	struct object_linked_list *object_list_begin;
	struct object_linked_list *object_list_end;
	size_t num_of_objects;
	GLfloat *vertex_coordinates;
	size_t vertex_coordinates_size; /* size of vertex_coordinates array */
	GLfloat *color_coordinates;
	size_t color_coordinates_size;
	GLfloat *texture_coordinates;
	size_t texture_coordinates_size;
	char is_prepared_to_draw;
	char is_rotating;
	GLfloat angle_x;
	GLfloat angle_y;
	GLfloat angle_z;
	GLfloat translate_x;
	GLfloat translate_y;
	GLfloat translate_z;
	GLuint *texture_objects;
	size_t num_of_textures;
	struct shaders_state shaders_state;
};


struct object *object_new(size_t num_of_vertices, GLfloat *vertex_coordinates,
 GLfloat *color_coordinates, GLfloat *texture_coordinates, char *texture_buffer,
                                 GLsizei texture_width, GLsizei texture_height);

void object_delete(struct object *object);

struct object_container *object_container_new();

void object_container_delete_objects(struct object_container *object_container);

void object_container_delete(struct object_container *object_container);

void object_container_set_translation(struct object_container *object_container,
		 GLfloat translate_x, GLfloat translate_y, GLfloat translate_z);

void object_container_set_rotation_angle(struct object_container
	  *object_container, GLfloat angle_x, GLfloat angle_y, GLfloat angle_z);

void object_container_add_object(struct object_container *object_container,
							 struct object *object);

void object_container_prepare_to_draw(struct object_container
							     *object_container);

void object_container_draw_objects(struct object_container *object_container);
/* FIXME: private ones? : */

size_t object_get_count_for_glDrawArrays(struct object *object);

size_t object_get_first_for_glDrawArrays(struct object *object);

GLfloat *object_container_get_pointer_for_glVertexPointer(struct
					    object_container *object_container);

GLfloat *object_container_get_pointer_for_glColorPointer(struct
                                            object_container *object_container);


GLfloat *object_container_get_pointer_for_glTexCoordPointer(struct
					    object_container *object_container);

/* FIXME: deprecate? */

char object_container_is_prepared_to_draw(struct
					    object_container *object_container);

/* FIXME : shouldn't there be set_rotating_step(x,y,z) and
   set_translating_step(x,y,z)? (step at the end of draw_objects()) */
void object_container_set_rotating_true(struct object_container
							     *object_container);

void object_container_set_rotating_false(struct object_container
							     *object_container);

#endif // __OBJECT_CONTAINER__H__
