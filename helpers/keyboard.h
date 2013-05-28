#include <linux/input.h>

/* Read keys from the keyboard in a raw mode, keyboard_read() is a
   non-blocking call and returns the character codes of type KEY_xxx
   (defined in linux/input.h), or KEY_NONE in case no key was pressed. */

#define KEY_NONE -2

typedef struct keyboard keyboard_t;

keyboard_t *keyboard_open(const char *device);
void keyboard_print_name(const keyboard_t *keyboard);
int keyboard_read(const keyboard_t *keyboard);
int keyboard_close(keyboard_t *keyboard);
