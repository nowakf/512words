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

#define INVALID_SPEC (utf8){0}
#define VARIANT_MAX 3

bool utf8_spec_equal(utf8 a, utf8 b) {
	return 	a.min_start     == b.min_start
		&& a.max_start  == b.max_start
		&& a.min_follow == b.min_follow
		&& a.max_follow == b.max_follow;
}

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

int min_variant(const uint8_t * str, int vlen, uint_t * score) {
}


uint8_t char_distance(uint8_t ch, uint8_t min, uint8_t max) {
	return __builtin_popcount(ch ^ clamp(ch, min, max));
}


uint8_t codepoint_distance(uint8_t *str, int len, int variant) {
	const utf8 spec = valid[len][variant];
	uint8_t distance = char_distance(str[0], spec.min_start, spec.max_start);
	for (int i=1; i<=len; ++i) {
		distance += char_distance(str[i], spec.min_follow, spec.max_follow);
	}
	return distance * 3 / len;
}
int characterize(uint8_t *str, int max_len) {
	int best_score = 0;
	int best_scoring = 0;
	for (int len=1; len<=max_len; ++len) {
		for (int variant=0; variant<VARIANT_MAX; ++variant) {
			if (utf8_spec_equal(valid[len][variant], INVALID_SPEC)) break;
			uint8_t score = codepoint_distance(str, len, variant);
			if (score > best_score) {
				best_score = score;
				best_scoring = score;
			}
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
	stats s = {
		calloc(len * 4, 1),
		calloc(len * 4, sizeof(int)),
		calloc(len * 4, sizeof(int)),
	};
	if (!s.odds || !s.types || s.indexes) {
		return (stats){0};
	}
	for (int i=0; i<(len*4); ++i) {
		s.indexes[i] = i;
	}

	for (int i=0; i<len; ++i) {
		s.odds[i] = PROB_MAX - codepoint_distance(buf+i, L1, 0);
		if (i > (len-1)) continue;
		s.odds[len+i] = PROB_MAX - codepoint_distance(buf+i, L2, 0);
		if (i > (len-2)) continue;
		s.odds[len*2+i] = PROB_MAX - codepoint_distance(buf+i, L3, 0);
		if (i > (len-3)) continue;
		s.odds[len*3+i] = PROB_MAX - codepoint_distance(buf+i, L4, 0);
	}


	int cmp (const void * a, const void * b) {
		int pa = index_to_pvalue(&p, *(int *)a, len);
		int pb = index_to_pvalue(&p, *(int *)b, len);
		return pa - pb;
	};

	qsort(s.indexes, len*4, sizeof(int), &cmp);

	//fill out array with 0xff (invalid utf8);
	//go over indexes inserting in order if there is enough space
	
	uint8_t * out = malloc(len);
	memset(out, 0xff, len);
	int filled = 0;
	int i = 0;
	while (filled < (len * 4) && i < (len * 4)) {
		int index = s.indexes[i];
		int run_len = index / len + 1;
		int start = index % len;
		if (space_available(out, start, run_len)) {
			transform(buf, out, run_len, s.types[i]);
			filled += run_len;
		}
		i++;
	}
	return s;
}

void free_stats(stats *s) {
	free(s->odds);
	free(s->types);
	free(s->indexes);
}

