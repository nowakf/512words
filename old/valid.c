#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define PROB_MAX 769 
// this is the maximum possible popcount, multiplied by all the different run lengths, so division never results in lost precision
#define UNFILLED_BYTE_MARK 0xff 
//0xff is invalid, so cannot be confused with bytes filled with utf8

typedef struct {
	uint8_t min;
	uint8_t max;
} range;

enum {
	L1  = 0,  L2  =  2, L3A =  4,
	L3B = 6,  L3C =  8, L3D = 10,
	L4A = 12, L4B = 14, L4C = 16,
	INDEX_COUNT = 18,
} range_indexes;

const range ranges[] = {
	[L1]  = {0x00, 0x7f}, {0},

	[L2]  = {0xc2, 0xdf}, {0x80, 0xbf},

	[L3A] = {0xe0, 0xe0}, {0xa0, 0xbf},
	[L3B] = {0xe1, 0xec}, {0x80, 0xbf},
	[L3C] = {0xed, 0xed}, {0x80, 0x9f},
	[L3D] = {0xee, 0xef}, {0x80, 0xbf},

	[L4A] = {0xf0, 0xf0}, {0x90, 0xbf},
	[L4B] = {0xf1, 0xf3}, {0x80, 0xbf},
	[L4C] = {0xf4, 0xf4}, {0x80, 0x8f},
};

const int lengths[] = {
	[L1]  = 1, [L2]  = 2, [L3A] = 3,
	[L3B] = 3, [L3C] = 3, [L3D] = 3,
	[L4A] = 4, [L4B] = 4, [L4C] = 4,
};

uint8_t bitwise_clamp(uint8_t val, uint8_t min, uint8_t max) {
	return (val | min) & max;
}

uint8_t numeric_clamp(uint8_t val, uint8_t min, uint8_t max) {
	val = bitwise_clamp(val, min, max);
	const uint8_t t = val < min ? min : val;
	return t > max ? max : t;
}

void transform(const uint8_t *from, uint8_t *to, int type) {
	to[0] = numeric_clamp(from[0], ranges[type].min, ranges[type].max);
	if (lengths[type] > 1) {
		to[1] =  numeric_clamp(from[1], ranges[type+1].min, ranges[type+1].max);
		for (int i=2; i<lengths[type]; ++i) {
			to[i] = numeric_clamp(from[i], ranges[L2+1].min, ranges[L2+1].max);
		}
	}
}

uint8_t char_distance(uint8_t ch, uint8_t min, uint8_t max) {
	return __builtin_popcount(ch ^ bitwise_clamp(ch, min, max));
}

uint16_t codepoint_distance(const uint8_t *str, int type) {
	range start = ranges[type];
	uint16_t distance = char_distance(str[0], start.min, start.max);
	if (lengths[type] > 1) {
		distance +=  char_distance(str[1], ranges[type+1].min, ranges[type+1].max);
		for (int i=2; i<lengths[type]; ++i) {
			distance += char_distance(str[i], ranges[L2+1].min, ranges[L2+1].max);
		}
	}
	return distance * 12 / lengths[type];
}

void get_type(const uint8_t *str, int len, uint8_t *type, uint16_t *score) {
	*score = 0;
	for (int i=0; i<INDEX_COUNT; i+=2) {
		if  (lengths[i] != len) continue;
		uint16_t new_score = PROB_MAX - codepoint_distance(str, i);
		if (*score < new_score) {
			*score = new_score;
			*type = i;
		}
	}
}


typedef struct {
	uint16_t score;
	uint8_t type;
	// spare space?
	int index;
} candidate;

int cmp_candidate(const void *a, const void *b) {
	int _a = ((candidate *)a)->score;
	int _b = ((candidate *)b)->score;
	return _b - _a;
}

void get_candidate_probabilities(candidate *candidates, const uint8_t *data, int len) {
	memset(candidates, 0, len*4*sizeof(candidates));
	for (int i=0; i<len; ++i) {
		candidates[i].index = i;
		get_type(data+i, 1, &(candidates[i].type), &(candidates[i].score));

		if (i>=(len-2)) continue;
		int j = i+len;
		candidates[j].index   = j;
		get_type(data+i, 2, &(candidates[j].type), &(candidates[j].score));

		if (i>=(len-3)) continue;
		int k = i+len*2;
		candidates[k].index = k;
		get_type(data+i, 3, &(candidates[k].type), &(candidates[k].score));

		if (i>=(len-4)) continue;
		int l = i+len*3;
		candidates[l].index = l;
		get_type(data+i, 4, &(candidates[l].type), &(candidates[l].score));
	}
}

bool has_space(const uint8_t *at, int type) {
	for (int i=0; i<lengths[type]; ++i) {
		if (at[i] != UNFILLED_BYTE_MARK) {
			return false;
		}
	}
	return true;
}

void closest_transformation(candidate *possible, const uint8_t *in, uint8_t *out, int len) {
	memset(out, UNFILLED_BYTE_MARK, len);
	qsort(possible, len * 4, sizeof(candidate), &cmp_candidate);
	int remaining_space = len;
	int i = 0;
	while (remaining_space > 0) {
		candidate c = possible[i++];
		int buf_index = c.index % len;
		if (has_space(out + buf_index, c.type)) {
			transform(in + buf_index, out + buf_index, c.type);
			remaining_space -= lengths[c.type];
		}
	}
}

void remove_nonsense_control_bytes(uint8_t *buf, int len) {
	for (int i=0; i<len; ++i) {
		uint8_t ch = buf[i];
		if (ch < 0x20 && ch > 0x09 || ch < 0x09 || ch == 0x7f) {
			buf[i] = ' ';
		}
	}
}

//for testing
int main(int argc, char ** argv) {
	FILE * stats = NULL;
	if (argc == 2) {
		stats = fopen(argv[1], "w");
	}
	uint8_t * in_buf = malloc(BUFSIZ);
	uint8_t * out_buf = malloc(BUFSIZ);
	int len = BUFSIZ - 1;
	candidate * candidates = malloc(len * 4 * sizeof(candidate));

	int read = 0;
	while ((read = fread(in_buf, 1, len, stdin))) {
		get_candidate_probabilities(candidates, in_buf, read);
		if (stats) {
			fprintf(stats, "score, type, index\n");
			for (int i=0; i<read*4; ++i) {
				fprintf(stats, "%d,%d,%d\n", candidates[i].score, candidates[i].type, candidates[i].index % len);
			}
		}
		closest_transformation(candidates, in_buf, out_buf, read);
		out_buf[read] = '\0';
		fwrite(out_buf, 1, read+1, stdout);
	}
	if (stats) fclose(stats);
	free(in_buf);
	free(out_buf);
	free(candidates);
}



