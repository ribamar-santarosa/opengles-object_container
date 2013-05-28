CFLAGS=-Wall
LDFLAGS=-L/opt/vc/lib/ -lGLESv2  -lEGL  -lbcm_host -lvcos -lvchiq_arm
INCLUDES=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads \
-I../helpers -I../object_container-opengles
TARGETS=first_basic_example first_basic_example-no_textures # first_basic_example

head: example_object_container.c ../helpers/keyboard.h ../helpers/keyboard.c ../object_container-opengles/object_container.h ../object_container-opengles/object_container.c
	gcc $(CFLAGS) $(INCLUDES) $(LDFLAGS) $^ -o example_object_container.bin
$(TARGETS):  ../helpers/keyboard.h ../helpers/keyboard.c ../object_container-opengles/object_container.h ../object_container-opengles/object_container.c
	gcc $(CFLAGS) $(INCLUDES) $(LDFLAGS) -x c $^ $@.c -o object_container-$@.bin
clean:
	rm *object_container*.bin
all: head $(TARGETS)
