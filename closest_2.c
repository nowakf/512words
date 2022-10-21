#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PROB_MAX 97
// 32 * 3 + 1

#define MIN(a, b, c) ({                                   \
	 __typeof__(a) _a = (a);                          \
	 __typeof__(b) _b = (b);                          \
	 __typeof__(c) _c = (c);                          \
	 _a < _b ? _a < _c ? _a : _c : _b < _c ? _b : _c; \
	 })                                               \

typedef struct {
	uint8_t min;
	uint8_t max;
} range;

const range leader_ranges[][] = {
	[L1] = {{0x00, 0x7f}},
	[L2] = {{0xc2, 0xdf}},
	[L3] = {{0xe0, 0xe0}, {0xe1, 0xec}, {0xed, 0xed}},
	[L4] = {{0xf0, 0xf0}, {0xf1, 0xf3}, {0xf4, 0xf4}},
};
const range[] valid_follow_ranges[] = {
	[L1] = NULL,
	[L2] = {{0x80, 0xbf}},
	[L3] = {{0xa0, 0xbf}, {0x80, 0xbf}, {0x80, 0x9f}},
	[L4] = {{0x90, 0xbf}, {0x80, 0xbf}, {0x80, 0x8f}},
}


typedef struct {
	uint8_t min;
	uint8_t max;
	int len;
	uint8_t min_continuation;
	uint8_t max_continuation;
} utf_range;

enum {
	F8,
	F9,
	FA,
	FB,
	VARIANT_COUNT,
}

enum { 
	L1,
	L2,
	L3,
	L4 = L3 + VARIANT_COUNT,
	VALID_RANGE_COUNT;
};


const utf_range ranges[] = {
        [L1]      = { 0x00, 0x7f, 1, 0,    0    },
	//1 byte character
        [L2]      = { 0xc2, 0xdf, 2, 0x80, 0xbf },
	//2 byte character, valid continuation range: 80..bf
        [L3_AB]   = { 0xe0, 0xe0, 3, 0xa0, 0xbf },
	//3 byte character, valid continuation range: a0..bf
        [L3_8B]   = { 0xe1, 0xec, 3, 0x80, 0xbf },
	//3 byte character, valid continuation range: 80..bf
        [L3_89]   = { 0xed, 0xed, 3, 0x80, 0x9f },
	//3 byte character, valid continuation range: 80..9f
        [L4_9B]   = { 0xf0, 0xf0, 4, 0x90, 0xbf },
	//4 byte character, valid continuation range: 90..bf
        [L4_8B]   = { 0xf1, 0xf3, 4, 0x80, 0xbf },
	//4 byte character, valid continuation range: 80..bf
        [L4_8]    = { 0xf4, 0xf4, 4, 0x80, 0x8f },
	//4 byte character, valid continuation range: 80..8f
	[INVALID] = { 0,    0,    0, 0,    0    }
	//invalid character
};

#define UTF_TYPE_COUNT INVALID

uint8_t clamp(uint8_t val, uint8_t min, uint8_t max) {
	return val | min & max;
}

void transform(const uint8_t * from, uint8_t *to, const utf_range * r) {
	to[0] = clamp(from[0], r->min, r->max);
	for (int i=1; i<r->len; ++i) {
		to[i] = clamp(from[i], r->min_continuation, r->max_continuation);
	}
}

int min_variant(const uint8_t * str, int vlen, uint_t * score) {
	int best = 0;
	uint8_t best_score = 0;
	for (int i=0; i<3; ++i) {
		uint8_t score = codepoint_distance(ranges[vlen+i] );
		if 
	}
}


uint8_t char_distance(uint8_t ch, uint8_t min, uint8_t max) {
	return __builtin_popcount(ch ^ clamp(ch, min, max));
}


uint8_t codepoint_distance(uint8_t *str, const utf_range * r) {
	uint8_t distance = char_distance(str[0], r->min, r->max);
	for (int i=1; i<=r->len; ++i) {
		distance += char_distance(str[i], r->min_continuation, r->max_continuation);
	}
	return distance * 3 / r->len;
}

int characterize(uint8_t *str, int max_len) {
	int best_score = 0;
	int best_scoring = 0;
	for (int i=0; i<UTF_TYPE_COUNT; ++i) {
		uint8_t score = codepoint_distance(str, &ranges[i]);
		if (score > best_score) {
			best_score = score;
			best_scoring = score;
		}
	}
	return best_scoring;
}

typedef struct {
	uint8_t * odds;
	int     * types;
	int     * indexes;
} stats;

bool space_available(uint8_t * buf, int start, int len) {
	for (int i=start; i<(start+len); ++i) {
		if (buf[i] == 0xff) {
			return false;
		}
	}
	return true;
}

stats new_stats(uint8_t * buf, int len) {
	probabilities p = {
		calloc(len * 4, 1),
		calloc(len * 4, sizeof(int)),
		calloc(len * 4, sizeof(int)),
	};
	if (!p.odds || !p.types || p.indexes) {
		return (probabilities){0};
	}
	for (int i=0; i<(len*4); ++i) {
		p.indexes[i] = i;
	}

	for (int i=0; i<len; ++i) {
		p.odds[i] = PROB_MAX - codepoint_distance(buf+i, L1);
		if (i > (len-1)) continue;
		p.odds[len+i] = PROB_MAX - codepoint_distance(buf+i, L2);
		if (i > (len-2)) continue;
		p.odds[len*2+i] = PROB_MAX - codepoint_distance(buf+i, L3);
		if (i > (len-3)) continue;
		p.odds[len*3+i] = PROB_MAX - codepoint_distance(buf+i, L4);
	}

	for (int i=0; i<len; ++i) {
		p.one_byte[i]   = PROB_MAX - codepoint_distance(buf+i, &ranges[L1]);
		if (i > (len-2)) continue;
		p.two_byte[i]   = PROB_MAX - codepoint_distance(buf+i, &ranges[L2]);
		if (i > (len-3)) continue;
		p.three_byte[i] = PROB_MAX - MIN(codepoint_distance(buf+i, &ranges[L3_AB]),
                                                 codepoint_distance(buf+i, &ranges[L3_89]),
                                                 codepoint_distance(buf+i, &ranges[L3_8B]));
		if (i > (len-4)) continue;
		p.four_byte[i]  = PROB_MAX - MIN(codepoint_distance(buf+i, &ranges[L4_9B]),
                                                codepoint_distance(buf+i, &ranges[L4_8B]),
                                                codepoint_distance(buf+i, &ranges[L4_8]));
	}

	int cmp (const void * a, const void * b) {
		int pa = index_to_pvalue(&p, *(int *)a, len);
		int pb = index_to_pvalue(&p, *(int *)b, len);
		return pa - pb;
	};

	qsort(p.indexes, len*4, sizeof(int), &cmp);

	//fill out array with 0xff (invalid utf8);
	//go over indexes inserting in order if there is enough space
	
	uint8_t * out = malloc(len);
	memset(out, 0xff, len);
	int filled = 0;
	int i = 0;
	while (filled < (len * 4) && i < (len * 4)) {
		int index = p.indexes[i++];
		int run_len = index / len + 1;
		int start = index % len;
		if (space_available(out, start, run_len)) {
			transform(buf, out, &ranges[L4_8]);
			filled += run_len;
		}
	}
	return p;
}

void free_probabilities(probabilities *p) {
	free(p->one_byte);
	free(p->two_byte);
	free(p->three_byte);
	free(p->four_byte);
	free(p->indexes);
}

int main() {
}
