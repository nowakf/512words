#include <stdio.h>
#include <stdint.h>

#include "512_words.h"

int main(int argc, char ** argv) {
	char buf[1000] = {0};
	int read = 0;
	while (read = fread(buf, 1, 1000, stdin)) {
		to_utf8(buf, read);
		cut_problem_control_chars(buf, read);
		buf[1000] = '\0';
		printf("%s\n", buf);
	}
}
