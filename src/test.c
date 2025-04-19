#include <stdio.h>

int main() {
	// On Saturday, the 19th of April 2025.
	printf(
		"\x1b[?1049h\x1b[H"
		"\x1b[3mThe excitement of creating something new and hopefully useful flows down your veins\n"
		"as you prepare for the very first commit.\x1b[23m\n\n\n\n"
		"\x1b[1;33m		You are filled with determination.\x1b[m");
	getchar();
	printf("\x1b[?1049l");
}

