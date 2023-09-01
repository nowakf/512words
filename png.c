#include <stdint.h>
#include <stdio.h>

#include "lodepng/lodepng.h"

int encode_png(uint8_t * png_buf, size_t *png_size, uint8_t *bitmap, int w, int h) {
	unsigned error = lodepng_encode_memory(&png_buf, png_size, bitmap, w, h, LCT_GREY, 1);

	if (error) {
		fprintf(stderr, lodepng_error_text(error));
		return -1;
	}

	return 0;
}
