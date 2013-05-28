#include "keyboard.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	keyboard_t *k = keyboard_open(argv[1]);
	if (!k)
		return 1;

	keyboard_print_name(k);
	printf("Press key A to print output and ESC to leave the program.\n");

	while (1) {
		int key = keyboard_read(k);
		if (key == -1)
			return 1;
		if (key == KEY_ESC)
			break;
		if (key == KEY_A)
			printf("Key A was pressed\n");
	}

	keyboard_close(k);
	printf("\nThanks!\n");
	return 0;
}
