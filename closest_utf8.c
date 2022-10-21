#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	uint8_t     mask;
	uint8_t     pat;
} utf8_t;


const static utf8_t encodings[] = {
	[0] = (utf8_t) {0b11000000, 0b10000000},
	[1] = (utf8_t) {0b10000000, 0b00000000},
	[2] = (utf8_t) {0b11100000, 0b11000000},
	[3] = (utf8_t) {0b11110000, 0b11100000},
	[4] = (utf8_t) {0b11111000, 0b11110000}
};

static int utf8_len(uint8_t ch) {
	for (int len=1; len<=4; ++len) {
		if ((encodings[len].mask & ch) == encodings[len].pat) {
			return len;
		}
	}
	return 0;
}

static float score_char(uint8_t ch, int len) {
	float max_deviation = __builtin_popcount(encodings[len].mask);
	float deviation = __builtin_popcount(ch & encodings[len].mask ^ encodings[len].pat);

	return max_deviation / deviation;
}

static int score(uint8_t * string, int len) {
	float score = score_char(string[0], len);
	for (int i=1; i<len; ++i) {
		score += score_char(string[i], 0);
	}
	return score;
}

static int guess_utf8_len(uint8_t * string, int max_len) {
	float best_score = -1;
	int best_scoring = max_len;
	for (int i=1; i<=max_len; ++i) {
		float proposal = score(string, i) / i;
		if (proposal > best_score) {
			best_score = proposal;
			best_scoring = i;
		}
	}
	return best_scoring;
}

static int closest_utf8_len(uint8_t * string, int max) {
	int hint_len = utf8_len(*string);
	if (hint_len > max || !hint_len) {
		return guess_utf8_len(string, max);
	}
	for (int i=1; i < max && i < hint_len; ++i) {
		if (encodings[0].mask & string[i] != encodings[0].pat) {
			return guess_utf8_len(string, max);
		}
	}
	return hint_len;
}
enum BYTE_TYPE {
	ONE        = 0x01,
	TWO        = 0x02,
	THREE      = 0x03,
	FOUR       = 0x04,
	INVAILD    = 0x05,
	FOLLOW_8   = 0x10,
	FOLLOW_9   = 0x20,
	FOLLOW_A   = 0x40,
	FOLLOW_B   = 0x80,
	THREE_8_9  = THREE | FOLLOW_8 | FOLLOW_9,
	THREE_A_B  = THREE | FOLLOW_A | FOLLOW_B,
	FOUR_8     = FOUR  | FOLLOW_8,
	FOUR_9_A_B = FOUR  | FOLLOW_9 | FOLLOW_A | FOLLOW_B
};

//characterize all valid bytes
//mark all valid sequences
//guess gaps
uint8_t characterize(uint8_t ch) {
	switch (ch) {
		case 0x00 ... 0x7f: 
			return ONE;
		case 0x80 ... 0x8f:
			return FOLLOW_8;
		case 0x90 ... 0x9f:
			return FOLLOW_9;
		case 0xa0 ... 0xaf:
			return FOLLOW_A;
		case 0xb0 ... 0xbf:
			return FOLLOW_B;
		case 0xc2 ... 0xcf:
			return TWO;
		case 0xe0:
			return THREE_A_B;
		case 0xe1 ... 0xec: case 0xee || 0xef:
			return THREE;
		case 0xed:
			return THREE_8_9;
		case 0xf0:
			return FOUR_9_A_B;
		case 0xf1 ... 0xf3:
			return FOUR;
		case 0xf4:
			return FOUR_8;
		default:
			return INVAILD;
	}
}

void cut_problem_control_chars(uint8_t * string, int len) {
	for (int i=0; i<len; ++i) {
		uint8_t ch = string[i];
		if (ch < 0x20 && ch > 0x09 || ch < 0x09) {
			string[i] = ' ';
		}
	}
}

static void replace_invalid_bytes(uint8_t * ch) {
	uint8_t mask = 0;
	switch (*ch) {
		case 0xc0 ... 0xc2: 
			*ch += 3;
			break;
		case 0xf5 ... 0xff:
			*ch -= 5;
			break;
		case 0xe0:
			mask = 0b10111111;
			break;
		case 0xed:
			mask = 0b10011111;
			break;
		case 0xf0:
			mask = 0b10111111;
			break;
		case 0xf4:
			mask = 0b10001111;
			break;
	}
	if (mask) {
		for (int i=1; i<4; ++i) {
			ch[i] &= mask;
		}
	}
}

static void make_valid(uint8_t ** string, int len) {
	(*string)[0] &= ~encodings[len].mask;
	(*string)[0] |= encodings[len].pat;

	for (int i=1; i<len; ++i) {
		(*string)[i] &= ~encodings[0].mask;
		(*string)[i] |= encodings[0].pat;
	}
	replace_invalid_bytes(*string);

	*string += len;
}

void to_utf8(uint8_t * buf, size_t buf_len) {
	uint8_t * ptr = buf;
	while (buf_len) {
		int seq_len_max = buf_len >= 4 ? 4 : buf_len;
		int len = closest_utf8_len(ptr, seq_len_max);
		make_valid(&ptr, len);
		buf_len -= len;
	}
}

void diff(uint8_t * original, uint8_t * utf8, size_t len) {
	for (int i=0; i<len; ++i) {
		original[i] ^= utf8[i];
	}
}

