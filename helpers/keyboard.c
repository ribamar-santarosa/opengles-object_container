#include "keyboard.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct keyboard {
	int fd;
	char device_name[256];
};

keyboard_t *keyboard_open(const char *device)
{
	keyboard_t *keyboard = malloc(sizeof(keyboard_t));
	keyboard->fd = open(device, O_RDONLY | O_NONBLOCK);

	if (keyboard->fd == -1) {
		perror("keyboard_open");
		return NULL;
	}

	return keyboard;
}

void keyboard_print_name(const keyboard_t *keyboard)
{
	char name[256];
	ioctl(keyboard->fd, EVIOCGNAME(sizeof(name)), name);
	printf("Device name: %s\n", name);
}

int keyboard_read(const keyboard_t *keyboard)
{
	struct input_event ev[64];
	int read_count;
	int events_count;

	read_count = read(keyboard->fd, &ev, sizeof(ev));
	if (read_count == -1) {
		/* If EAGAIN is not an error, we expose a non-blocking
		   API, so will just return. */
		if (errno != EAGAIN) {
			perror("keyboard_read");
			return -1;
		}
        return KEY_NONE;
	}

    events_count = read_count / sizeof(struct input_event);
    if (events_count > 1 && ev[0].value != ' ' && ev[1].type == EV_KEY && ev[1].value == 1)
		return ev[1].code;
	return KEY_NONE;
}

int keyboard_close(keyboard_t *keyboard)
{
	int fd = keyboard->fd;
	free(keyboard);

	if (close(fd)) {
		perror("keyboard_close");
		return -1;
	}

	return 0;
}
