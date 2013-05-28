#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES/gl.h>

#include "object_container.h"

struct object *object_new(size_t num_of_vertices, GLfloat *vertex_coordinates,
 GLfloat *color_coordinates, GLfloat *texture_coordinates, char *texture_buffer,
                                  GLsizei texture_width, GLsizei texture_height)
{
	struct object *object = (struct object*) malloc(sizeof(struct object));
	assert(object != NULL);
	object->num_of_vertices = num_of_vertices;

	object->vertex_coordinates = vertex_coordinates;
	object->color_coordinates = color_coordinates;
    object->texture_coordinates = texture_coordinates;

	object->texture_buffer = texture_buffer;

	if (texture_buffer == NULL) {
        /* TODO: now texture_object isn't a pointer, is 0 a safe value to
        declare texture_object? isn't 0 the value of a valid texture_object?
        */
    } else {
        glGenTextures(1, &object->texture_object);
        glBindTexture(GL_TEXTURE_2D, object->texture_object);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width,
             texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                                           (GLfloat)GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                                           (GLfloat)GL_NEAREST);
	}
	return object;
}

void object_delete(struct object *object)
{
	assert(object != NULL);
	if (object->texture_buffer != NULL)
		glDeleteTextures(1, &object->texture_object);
	free(object);
}

struct object_container *object_container_new()
{
        struct object_container *object_container = (struct object_container*)
					malloc(sizeof(struct object_container));
	assert(object_container != NULL);
	object_container->object_list_begin = NULL;
	object_container->object_list_end = NULL;
	object_container->num_of_objects = 0;
	object_container->is_prepared_to_draw = 0;
	object_container->translate_x = 0;
	object_container->translate_y = 0;
	object_container->translate_z = 0;
	object_container->is_rotating = 0;
	object_container->angle_x = 0;
	object_container->angle_y = 0;
	object_container->angle_z = 0;
	object_container->vertex_coordinates = NULL;
	object_container->vertex_coordinates_size = 0;
	object_container->color_coordinates = NULL;
	object_container->color_coordinates_size = 0;
	object_container->texture_coordinates = NULL;
	object_container->texture_coordinates_size = 0;
	return object_container;
}

void object_container_delete_objects(struct object_container *object_container)
{
	assert(object_container != NULL);
	struct object_linked_list *object_list =
					    object_container->object_list_begin;
	struct object_linked_list *link;
	while (object_list != NULL) {
		object_delete(object_list->object);
		link = object_list;
		object_list = object_list->next;
		free(link);
	}
	object_container->object_list_end = NULL;
}

void object_container_delete(struct object_container *object_container)
{
	assert(object_container != NULL);
	free(object_container);
}

void object_container_set_translation(struct object_container *object_container,
		  GLfloat translate_x, GLfloat translate_y, GLfloat translate_z)
{
	assert(object_container != NULL);
	object_container->translate_x = translate_x;
	object_container->translate_y = translate_y;
	object_container->translate_z = translate_z;
}

void object_container_set_rotating_true(struct object_container
							      *object_container)
{
	assert(object_container != NULL);
	object_container->is_rotating = 1;
}

void object_container_set_rotating_false(struct object_container
							      *object_container)
{
	assert(object_container != NULL);
	object_container->is_rotating = 0;
}

void object_container_set_rotation_angle(struct object_container
	   *object_container, GLfloat angle_x, GLfloat angle_y, GLfloat angle_z)
{
	assert(object_container != NULL);
	object_container->angle_x = angle_x;
	object_container->angle_y = angle_y;
	object_container->angle_z = angle_z;
}

void object_container_add_object(struct object_container *object_container,
							  struct object *object)
{
	struct object_linked_list *new_link;

	assert(object_container != NULL);
	assert(object_container->is_prepared_to_draw == 0);
	assert(object != NULL);
	object_container->num_of_objects++;
	object_container->vertex_coordinates_size  += object->num_of_vertices *
					       3 /* (x,y,z)*/ * sizeof(GLfloat);
	object_container->color_coordinates_size   += object->num_of_vertices *
					     4 /* (r,g,b,a)*/ * sizeof(GLfloat);

	/* FIXME: when no texture_buffer, no reason to alloc room for it */
	object_container->texture_coordinates_size += object->num_of_vertices *
				   2 /* (x,y) tex is plane */ * sizeof(GLfloat);

	new_link = (struct object_linked_list *)
				    malloc(sizeof(struct object_linked_list *));
	assert(new_link != NULL);
	new_link->object = object;
	new_link->next = NULL;

	if (object_container->object_list_begin == NULL) {
		/* first object to add: place it on begin */
		assert(object_container->object_list_end == NULL);
		object_container->object_list_begin = new_link;
	} else {
		/* link the end of the list to this new link: */
		assert(object_container->object_list_end != NULL);
		object_container->object_list_end->next = new_link;
	}

	/* added object is always placed on the end of the list: */
	object_container->object_list_end = new_link;
	assert(object_container->object_list_end->next == NULL);
}

void object_container_prepare_to_draw(struct object_container *object_container)
{
	struct object_linked_list *object_list;
	GLfloat *dest_buffer, *src_buffer;
	size_t i, n, dest_vertex_pos, dest_color_pos, dest_texture_pos,
							     vertex_accumulator;

	assert(object_container != NULL);
	assert(object_container->is_prepared_to_draw == 0);

	object_container->vertex_coordinates = (GLfloat *)
			      malloc(object_container->vertex_coordinates_size);
	assert(object_container->vertex_coordinates);

	object_container->color_coordinates = (GLfloat *)
			       malloc(object_container->color_coordinates_size);
	assert(object_container->color_coordinates);

	/* FIXME: when no texture_buffer, no reason to alloc room for it */
	object_container->texture_coordinates = (GLfloat *)
			     malloc(object_container->texture_coordinates_size);
	assert(object_container->texture_coordinates);

	object_list = object_container->object_list_begin;

	dest_vertex_pos = 0;
	dest_color_pos = 0;
	dest_texture_pos = 0;
	vertex_accumulator = 0;
	while (object_list != NULL) {
		/* copy data from object arrays to container arrays ... */
		assert(object_list->object != NULL);

		printf("\n---\nobject_list->object->num_of_vertices=%d\n",
					  object_list->object->num_of_vertices);
		fflush(stdout);

		/* 1. copy of vertex_coordinates */
		printf("vertex copy\n");
		fflush(stdout);
		assert(dest_vertex_pos <
				     object_container->vertex_coordinates_size);

		n = sizeof(GLfloat)
		       * 3 /* (x,y,z) */ * object_list->object->num_of_vertices;
		src_buffer = object_list->object->vertex_coordinates;
		dest_buffer =
			 &object_container->vertex_coordinates[dest_vertex_pos];
		memcpy(dest_buffer, src_buffer, n);

		printf("*** vertex_acc=%d\n", vertex_accumulator);
		printf("*** n=%d (ie n=%d)\n", n, (int) (n/sizeof(GLfloat)));
		printf("*** dest_vertex_pos=%d\n", dest_vertex_pos);
		fflush(stdout);

		/* deprecate pos_on_container? TODO */
		object_list->object->vertex_pos_on_container = dest_vertex_pos;
		dest_vertex_pos += (size_t) n/sizeof(GLfloat);

		object_list->object->vertex_pos_on_container = vertex_accumulator;
		vertex_accumulator += object_list->object->num_of_vertices;

		/* 2. copy of color_coordinates */
		printf("color copy\n");
		fflush(stdout);
		assert(dest_color_pos <
				      object_container->color_coordinates_size);

		n = sizeof(GLfloat)
		     * 4 /* (r,g,b,a) */ * object_list->object->num_of_vertices;
		src_buffer = object_list->object->color_coordinates;
		dest_buffer =
			 &object_container->color_coordinates[dest_color_pos];
		memcpy(dest_buffer, src_buffer, n);

		/* deprecate pos_on_container? TODO */
		object_list->object->color_pos_on_container = dest_color_pos;
		dest_color_pos += (size_t) n/sizeof(GLfloat);


		/* 3. copy of texture_coordinates */
		printf("tex coord copy\n");
		fflush(stdout);
		assert(dest_texture_pos <
				    object_container->texture_coordinates_size);

		printf("3.1 n...\n");
		fflush(stdout);
		n = sizeof(GLfloat)
			 * 2 /* (x,y) */ * object_list->object->num_of_vertices;
		printf("3.2 source buf...\n");
		fflush(stdout);
		src_buffer = object_list->object->texture_coordinates;
		printf("3.3 dest buf...\n");
		fflush(stdout);
		dest_buffer =
			&object_container->texture_coordinates[dest_texture_pos];
		printf("3.4 memcpy...\n");
		fflush(stdout);
		memcpy(dest_buffer, src_buffer, n);

		printf("3.4 dest_texture_pos finish...\n");
		fflush(stdout);
		/* deprecate pos_on_container? TODO */
		object_list->object->texture_pos_on_container = dest_texture_pos;
		dest_texture_pos += (size_t) n/sizeof(GLfloat);


		/* ... and step */
		object_list = object_list->next;
	}
	object_container->is_prepared_to_draw = 1;

	printf("----\n");
	printf("4.1 vertex_coordinates:");
	for (i = 0;  i< object_container->vertex_coordinates_size/sizeof(GLfloat);
								       i += 1) {

		if (i % 3 == 0 ) {
			printf("\n");
		}

		printf("%2.5f \t", object_container->vertex_coordinates[i]);
		fflush(stdout);
	}
	printf("\n");

	printf("4.2 color_coordinates:");
	for (i = 0;  i< object_container->color_coordinates_size/sizeof(GLfloat); i += 1) {

		if (i % 4 == 0) {
			printf("\n");
		}

		printf("%2.5f \t", object_container->color_coordinates[i]);
		fflush(stdout);
	}
	printf("\n");

	/* send arrays to GPU - TODO maybe making this block a separate function
	will make this lib more flexible for optmizations */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0,
			object_container_get_pointer_for_glVertexPointer(object_container));
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0,
			 object_container_get_pointer_for_glColorPointer(object_container));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0,
		  object_container_get_pointer_for_glTexCoordPointer(object_container));

}

char object_container_is_prepared_to_draw(struct
                                             object_container *object_container)
{
	assert(object_container != NULL);
	return object_container->is_prepared_to_draw;
}


size_t object_get_count_for_glDrawArrays(struct object *object)
{
	assert(object != NULL);
	return object->num_of_vertices;
}

size_t object_get_first_for_glDrawArrays(struct object *object)
{
	assert(object != NULL);
	return object->vertex_pos_on_container;
}

GLfloat *object_container_get_pointer_for_glVertexPointer(struct
					     object_container *object_container)
{
	assert(object_container != NULL);
	assert(object_container->is_prepared_to_draw);
	return object_container->vertex_coordinates;
}

GLfloat *object_container_get_pointer_for_glColorPointer(struct
                                             object_container *object_container)
{
	assert(object_container != NULL);
	assert(object_container->is_prepared_to_draw);
	return object_container->color_coordinates;
}

GLfloat *object_container_get_pointer_for_glTexCoordPointer(struct
					     object_container *object_container)
{
	assert(object_container != NULL);
	assert(object_container->is_prepared_to_draw);
	return object_container->texture_coordinates;
}

void object_container_draw_objects(struct object_container *object_container)
{
	GLuint *texture_object;
	struct object_linked_list *object_list;
	static char debug_next_time = 0;

	assert(object_container != NULL);
	assert(object_container_is_prepared_to_draw(object_container));

	/* 1. apply transformations */
	glLoadIdentity();

	glTranslatef(object_container->translate_x,
		  object_container->translate_y, object_container->translate_z);

	if (object_container->angle_x)
		glRotatef(object_container->angle_x, 1.f, 0.f, 0.f);
	if (object_container->angle_y)
		glRotatef(object_container->angle_y, 0.f, 1.f, 0.f);
	if (object_container->angle_z)
		glRotatef(object_container->angle_z, 0.f, 0.f, 1.f);

	object_list = object_container->object_list_begin;

	glClear( GL_DEPTH_BUFFER_BIT );
	glClear( GL_COLOR_BUFFER_BIT );

	while (object_list != NULL) {
		/* TODO: after textures are working, try to leave onle glBind
		   inside this loop (and step 4, off course). i believe enable
		   and disable texture are required only once. */
		assert(object_list->object != NULL);
		texture_object = &object_list->object->texture_object;

		if (debug_next_time) {
			printf("\n---\ndraw_objects(): ");
            printf("object_list->object->num_of_vertices=%d\n",
                                          object_list->object->num_of_vertices);
			fflush(stdout);
		}

		/* 2. bind texture */
		if (texture_object != NULL) {
			glEnable(GL_TEXTURE_2D);
			glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBindTexture(GL_TEXTURE_2D, *texture_object);
		}

		/* 3. draw arrays */
		if (debug_next_time) {
			printf("first_for_glDA()=%d\n",
                       object_get_first_for_glDrawArrays(object_list->object) );
			printf("count_for_glDA()=%d\n",
                       object_get_count_for_glDrawArrays(object_list->object) );
			fflush(stdout);
		}

		glDrawArrays(
			GL_TRIANGLE_FAN,
                         object_get_first_for_glDrawArrays(object_list->object),
                        object_get_count_for_glDrawArrays(object_list->object));

		/* 4. disable texture */
		if (texture_object != NULL) {
			glDisable(GL_TEXTURE_2D);
		}

		object_list = object_list->next;
	}

	debug_next_time = 0;


}

