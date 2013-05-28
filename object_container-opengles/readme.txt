opengles-object_container
=========================

library that simplifies the OpenGL/ES programming, tested using EGL in the Raspbian/Raspberry Pi board.

0. Introduction

object_container is a implementation of a class encapsulating coordinates
manipulation in OpenGL/ES, tested in the Raspberry PI Board, having
nix-rpi-image-2012-12-04.img image installed (a complete output for dpkg -l is
provided in file output-dpkg-l-2013.05.07.txt).

the basic idea is that, instead of aligning the vertex, color and texture
coordinate arrays for all the scene, the user will create objects for polygons
in the scene (calling object_new()), add them (calling
object_container_add_object()) to a structure that describes the whole scene for
a frame called object_container (previously created with
object_container_new()), and, when there is no more objects to be added, the
user tells the container that no more objects will be added to that
scene (calling object_container_prepare_to_draw()). at this point, the
object_container can supposedly do the eventual optimizations and then the
coordinates are sent to the GPU, but the scene is not drawn yet. the scene will
be actually drawn by a subsequent call to object_container_draw_objects().

notice that multiple scenes can be prepared in parallel using multiple
object_container, what leaves room for the user to optimize points he/she
thinks can be done. but a call to object_container_draw_objects() must be a
consequent call of object_container_prepare_to_draw() for the same
object_container, since the GPU is already commited to the drawing of that
scene.

still, note that object_container_draw_objects() draws to the buffer, so,
multiple scenes can be drawn to the same buffer/frame (reusing a
object_container or using multiple ones).  the frame/buffer is actually drawn
when eglSwapBuffers() is called, by the object_container user.

a point (that may be ignored by beginners but must be considered for them as
soon as possible) is that, after object_container_prepare_to_draw() or
object_container_draw_objects() the coordinate matrices are on GPU, so
transformations like rotation or translation or any other may be applied, and
then one may proceed to a direct call to object_container_draw_objects() without
a new call to object_container_prepare_to_draw() being necessary.  For while,
object_container doesn't provide no facilities for doing such transformations,
so it must be done manually by object_container's users.


1. Getting opengles-object_container

git clone https://github.com/ribamar-santarosa/opengles-object_container.git
cd opengles-object_container/


2. Running object_container examples

RPI_USER=root
RPI_IP=10.60.4.117
scp -r object_container-opengles ${RPI_USER}@${RPI_IP}:
scp -r helpers ${RPI_USER}@${RPI_IP}:
ssh ${RPI_USER}@${RPI_IP}

cd object_container-opengles
# the most basic example, not having even textures
make clean ; make first_basic_example-no_textures ; ./object_container-first_basic_example-no_textures.bin
# the most basic example having textures
make clean ; make first_basic_example ; ./object_container-first_basic_example.bin
# the example's head, example_object_container.c
make clean ; make head ; ./example_object_container.bin

exit # from rpi

3. Developping with object_container

knowledge about opengles, is, by the way, required, to properly use
object_container. the suggestion is trying to read the code in the order
first_basic_example-no_textures.c -> first_basic_example.c ->
example_object_container.c



