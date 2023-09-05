#include <stdint.h>
#include <string.h>

void rgba2luma(uint8_t *luma, int len) {
	for (int i=0; i<len/4; i++) {
		luma[i] = (luma[i*4] + luma[i*4+1] + luma[i*4+2]) / 3;
	}
}

void composite(uint8_t * rg8, const uint8_t * a, const uint8_t * b, int len) {
	for (int i=0; i<len; i++) {
		int shift = 7 - i % 8;
		uint8_t mask = 0x1 << shift;
		rg8[i*2+0] = (a[i/8] & mask >> shift) * 255;
		rg8[i*2+1] = (b[i/8] & mask >> shift) * 255;
	}
}


void crop(uint8_t * luma, int prev_w, int w, int h) {
	for (int y=0; y<h; y++) {
		memmove(luma + y*w, luma + y * prev_w, w);
	}
}

void threshold(const uint8_t * luma, uint8_t *bits, int len, uint8_t mid) {
	for (int i=0; i<len/8; i++) {
		bits[i] = (luma[i*8]   > mid ? 0b10000000 : 0)
			| (luma[i*8+1] > mid ? 0b01000000 : 0)
			| (luma[i*8+2] > mid ? 0b00100000 : 0)
			| (luma[i*8+3] > mid ? 0b00010000 : 0)
			| (luma[i*8+4] > mid ? 0b00001000 : 0)
			| (luma[i*8+5] > mid ? 0b00000100 : 0)
			| (luma[i*8+6] > mid ? 0b00000010 : 0)
			| (luma[i*8+7] > mid ? 0b00000001 : 0);
	}
}
