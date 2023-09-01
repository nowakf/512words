#include <stdint.h>
#include <string.h>

uint8_t * crop(uint8_t * luma, int prev_w, int w, int h) {
	for (int y=0; y<h; y++) {
		memmove(luma + y*w, luma + y * prev_w, w);
	}
	return luma;
}

int next_lesser_power_of_2(unsigned int n) {
	return n & (0x80000000 >> __builtin_clz(n));
}

void quarter(uint8_t * luma, int w, int h) {
	for (int y=0; y<(h/2); y++) {
		int sy = y * 2;
		for (int x=0; x<(w/2); x++) {
			int sx = x * 2;
			luma[y*(w/2)+x] = ( luma[sy * w + sx]
					  + luma[sy * w + sx + 1]
					  + luma[sy * w + sx + w]
					  + luma[sy * w + sx + w + 1] ) >> 2;
		}
	}
}

void rgba2luma(uint8_t *luma, int len) {
	for (int i=0; i<len/4; i++) {
		luma[i] = (luma[i*4] + luma[i*4+1] + luma[i*4+2]) / 3;
	}
}

void threshold(uint8_t * luma, int len, uint8_t mid) {
	for (int i=0; i<(len/8); i++) {
		luma[i] = (luma[i*8]   > mid ? 0b10000000 : 0)
			| (luma[i*8+1] > mid ? 0b01000000 : 0)
			| (luma[i*8+2] > mid ? 0b00100000 : 0)
			| (luma[i*8+3] > mid ? 0b00010000 : 0)
			| (luma[i*8+4] > mid ? 0b00001000 : 0)
			| (luma[i*8+5] > mid ? 0b00000100 : 0)
			| (luma[i*8+6] > mid ? 0b00000010 : 0)
			| (luma[i*8+7] > mid ? 0b00000001 : 0);
	}
}

