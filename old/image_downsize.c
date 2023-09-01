#include <stdint.h>

void shrink(uint8_t *luma, int w, int h) {
	for (int i=0; i<(w*h/4); i++) {
		int ox = i % (w/2) * 2;
		int oy = i / (w/2) * 2;
		luma[i] = ( luma[oy * w + ox]
			  + luma[oy * w + ox + 1]
			  + luma[oy * w + ox + w]
			  + luma[oy * w + ox + w + 1]) >> 2;
	}
}

void threshold(uint8_t * luma, int len, uint8_t mid) {
	for (int i=0; i<(len/8); i++) {
		uint8_t bit = 0b10000000;
		for (int i=0; i<8; ++i) {
			luma[i] |= luma[i*8+i] < mid ? bit >> i : 0;
		}
	}
}

