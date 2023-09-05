#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define PROB_MAX 193
// 32 * 6 + 1

typedef struct {
	uint8_t min_start;
	uint8_t max_start;
	uint8_t min_follow;
	uint8_t max_follow;
} utf8;

enum {L1 = 1, L2, L3, L4};

#define VARIANT_MAX 4

const utf8 valid[][VARIANT_MAX] = {
//		variant 1		   variant 2		    variant 3
//	len	mn_s   mx_s  mn_f  mx_f    mn_s   mx_s  mn_f  mx_f   mn_s   mx_s  mn_f  mx_f
	[L1] = {{0x00, 0x7f  /*x    x*/} /* x     x     x     x       x     x     x     x */},
	[L2] = {{0xc2, 0xdf, 0x80, 0xbf} /* x     x     x     x       x     x     x     x */},
	[L3] = {{0xe0, 0xe0, 0xa0, 0xbf}, {0xe1, 0xec, 0x80, 0xbf}, {0xed, 0xed, 0x80, 0x9f}, {0xee, 0xef, 0x80, 0xbf}},
	[L4] = {{0xf0, 0xf0, 0x90, 0xbf}, {0xf1, 0xf3, 0x80, 0xbf}, {0xf4, 0xf4, 0x80, 0x8f}},
};


bool utf8_spec_equal(utf8 a, utf8 b) {
	return 	a.min_start     == b.min_start
		&& a.max_start  == b.max_start
		&& a.min_follow == b.min_follow
		&& a.max_follow == b.max_follow;
}

#define invalid_spec(a) (utf8_spec_equal((a), (utf8){0}))

uint8_t clamp(uint8_t val, uint8_t min, uint8_t max) {
	return val > max ? max : val < min ? min : val; 
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
	for (int i=1; i<len; ++i) {
		distance += char_distance(str[i], spec.min_follow, spec.max_follow);
	}
	return distance * 6 / len;
}

int min_variant(const uint8_t * str, int vlen, uint8_t * score) {
	int variant = 0;
	for (int i=0; i<VARIANT_MAX; ++i) {
		utf8 spec = valid[vlen][i];
		if (invalid_spec(spec)) break;
		uint8_t new_score = codepoint_distance(str, vlen, i);
		if (*score > new_score) {
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
		if (buf[i] != 0xff) {
			return false;
		}
	}
	return true;
}

void get_occupant_probability(occupant *occupants, const uint8_t *buf, int len){
	uint8_t p1 = 0;
	int v = 0;
	for (int i=0; i<(len*4); ++i) {
		occupants[i].index = i;
		occupants[i].variant = 0;
		occupants[i].likelyhood = 0;
	}

	for (int i=0; i<len; ++i) {
		occupants[i].likelyhood = PROB_MAX - codepoint_distance(buf+i, L1, 0);
		if (i >= (len-2)) continue;
		occupants[i+len].likelyhood = PROB_MAX - codepoint_distance(buf+i, L2, 0);
		if (i >= (len-3)) continue;
		v = min_variant(buf+i, 3, &p1);
		occupants[i+len*2].likelyhood = PROB_MAX - p1;
		occupants[i+len*2].variant = v;
		if (i >= (len-4)) continue;
		v = min_variant(buf+i, 4, &p1);
		occupants[i+len*3].likelyhood = PROB_MAX - p1;
		occupants[i+len*3].variant = v;
	}
}
int cmp (const void * a, const void * b) {
	int _a = ((occupant *)a)->likelyhood;
	int _b = ((occupant *)b)->likelyhood;
	return _b - _a;
};

void clear_nonsense_control_bytes(uint8_t *buf, int len) {
	for (int i=0; i<len; ++i) {
		uint8_t ch = buf[i];
		if (ch < 0x20 && ch > 0x09 || ch < 0x09) {
			buf[i] = ' ';
		}
	}
}

void closest_transformation(occupant *probable_occupants, const uint8_t *in, uint8_t *out, int len) {
	qsort(probable_occupants, len*4, sizeof(occupant), &cmp);
	memset(out, 0xff, len);
	int remaining_space = len;
	int i = 0;
	while ((remaining_space > 0) && (i < (len*4))) {
		occupant o = probable_occupants[i++];
		int run_len = o.index / len + 1;
		int buf_index = o.index % len;
		if (space_available(out, buf_index, run_len)) {
			transform(in+buf_index, out+buf_index, run_len, o.variant);
			remaining_space -= run_len;
		}
	}
}
void print_occupants(occupant * occupants, int len) {
	for (int i=0; i<len; ++i) {
		int run_len = occupants[i].index / (len/4) + 1;
		int buf_index = occupants[i].index % (len/4);
		printf("%d, %d, %d, %d, %d\n", occupants[i].index, occupants[i].likelyhood, occupants[i].variant, run_len, buf_index);
	}
}

uint8_t * to_utf8(const uint8_t *in, size_t len) {
	uint8_t * out = malloc(len + 1);
	occupant * occupants = malloc(len * 4 * sizeof(occupant));
	get_occupant_probability(occupants, in, len);
	closest_transformation(occupants, in, out, len);
	clear_nonsense_control_bytes(out, len);
	out[len] = '\0';
	free(occupants);
	return out;
}

uint8_t * diff(const uint8_t * original, const uint8_t * utf8, size_t len) {
	uint8_t * out = malloc(len);
	for (int i=0; i<len; ++i) {
		out[i] = original[i] ^ utf8[i];
	}
	return out;
}



int main(int argc, char ** argv) {
	uint8_t * in = (uint8_t *)argv[1];
	int len = strlen(in);
	uint8_t * out = to_utf8(in, len);
	printf("%s\n", out);
	free(out);
	return 0;
}

