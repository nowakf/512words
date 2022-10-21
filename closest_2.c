#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PROB_MAX 97
// 32 * 3 + 1

typedef struct {
	uint8_t min_start;
	uint8_t max_start;
	uint8_t min_follow;
	uint8_t max_follow;
} utf8;
enum {L1 = 1, L2, L3, L4};

const utf8 valid[][3] = {
//	len	min_s  max_s min_f  max_f
	[L1] = {{0x00, 0x7f            }           				            },
	[L2] = {{0xc2, 0xdf, 0x80, 0xbf}           				            },
	[L3] = {{0xe0, 0xe0, 0xa0, 0xbf}, {0xe1, 0xec, 0x80, 0xbf}, {0xed, 0xed, 0x80, 0x9f}},
	[L4] = {{0xf0, 0xf0, 0x90, 0xbf}, {0xf1, 0xf3, 0x80, 0xbf}, {0xf4, 0xf4, 0x80, 0x8f}},
};

#define VARIANT_MAX 4

bool utf8_spec_equal(utf8 a, utf8 b) {
	return 	a.min_start     == b.min_start
		&& a.max_start  == b.max_start
		&& a.min_follow == b.min_follow
		&& a.max_follow == b.max_follow;
}

#define invalid_spec(a) (utf8_spec_equal((a), (utf8){0}))

uint8_t clamp(uint8_t val, uint8_t min, uint8_t max) {
	return val | min & max;
}

void transform(const uint8_t * from, uint8_t *to, int len, int variant) {
	const utf8 spec = valid[len][variant];
	to[0] = clamp(from[0], spec.min_start, spec.max_start);
	for (int i=1; i<len; ++i) {
		to[i] = clamp(from[i], spec.min_follow, spec.max_follow);
	}
}

uint8_t char_distance(uint8_t ch, uint8_t min, uint8_t max) {
	return __builtin_popcount(ch ^ clamp(ch, min, max));
}


uint8_t codepoint_distance(const uint8_t *str, int len, int variant) {
	const utf8 spec = valid[len][variant];
	uint8_t distance = char_distance(str[0], spec.min_start, spec.max_start);
	for (int i=1; i<=len; ++i) {
		distance += char_distance(str[i], spec.min_follow, spec.max_follow);
	}
	return distance * 3 / len;
}

int min_variant(const uint8_t * str, int vlen, uint8_t * score) {
	int variant = 0;
	for (int i=0; i<VARIANT_MAX; ++i) {
		utf8 spec = valid[vlen][i];
		uint8_t new_score = codepoint_distance(str, vlen, i);
		if (*score < new_score) {
			*score = new_score;
			variant = i;
		}
	}
	return variant;
}

typedef struct {
	uint8_t likelyhood;
	int8_t variant;
	int index;
} occupant;


bool space_available(uint8_t * buf, int start, int len) {
	for (int i=start; i<(start+len); ++i) {
		if (buf[i] == 0xff) {
			return false;
		}
	}
	return true;
}

void get_occupant_probability(occupant *occupants, const uint8_t *buf, int len){
	uint8_t p1;
	int v;
	for (int i=0; i<len; ++i) {
		occupants[i] = (occupant){
			.likelyhood = PROB_MAX - codepoint_distance(buf+i, L1, 0),
			.variant = 0,
			.index = i,
		};
		if (i > (len-1)) continue;
		occupants[i+len] = (occupant){
			.likelyhood = PROB_MAX - codepoint_distance(buf+i, L2, 0),
			.variant = 0,
			.index = i+len,
		};
		if (i > (len-2)) continue;
		v = min_variant(buf+i, 3, &p1);
		occupants[i+len*2] = (occupant){
			.likelyhood = PROB_MAX - p1,
			.variant = v,
			.index = i+len*2,
		};
		if (i > (len-3)) continue;
		v = min_variant(buf+i, 4, &p1);
		occupants[i+len*3] = (occupant){
			.likelyhood = PROB_MAX - p1,
			.variant = v,
			.index = i+len*3,
		};
	}
}
int cmp (const void * a, const void * b) {
	int _a = ((occupant *)a)->likelyhood;
	int _b = ((occupant *)b)->likelyhood;
	return _a - _b;
};

void closest_transformation(occupant *probable_occupants, const uint8_t *in, uint8_t *out, int len) {
	qsort(probable_occupants, len*4, sizeof(int), &cmp);
	memset(out, 0xff, len);
	int filled = 0;
	int i = 0;
	while (filled < (len * 4) && i < (len * 4)) {
		occupant o = probable_occupants[i++];
		int run_len = o.index / len + 1;
		int buf_index = o.index % len;
		if (space_available(out, buf_index, run_len)) {
			transform(in+buf_index, out+buf_index, run_len, o.variant);
			filled += run_len;
		}
	}
}

