#include <stdint.h>

#include "512_words.h"

int main(int argc, char ** argv) {
	if (argc != 5) {
		fprintf(stderr, "Usage: cat in.yuv | <bin> <w> <h> <threshold> <out.png> > utf8.txt");
		return -1;
	}
	FILE *fp        = freopen(NULL, "rb", stdin);
	int w           = atol(argv[1]);
	int h           = atol(argv[2]);
	int threshold   = atol(argv[3]);
	char * filename = argv[4];

	int size = w * h;

	uint8_t * orig = malloc(size);

	int read;
	while ((read = fread(orig, 1, size, fp))) {
		int square = next_lesser_power_of_2(w < h ? w : h);
		orig = crop(orig, square, square);
		while (square > 512) {
			quarter(orig, square, square);
			square /= 2;
		}
		threshold(orig, square*square, threshold);
		uint8_t * utf8 = to_utf8(orig, square*square/8);
		uint8_t * diff = diff(orig, utf8, square*square/8);
	}
}
