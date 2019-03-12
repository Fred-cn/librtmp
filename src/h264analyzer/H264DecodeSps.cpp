#include "stdafx.h"
#include "H264DecodeSps.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t default_scaling4[2][16] = {
    { 6, 13, 20, 28, 13, 20, 28, 32,
    20, 28, 32, 37, 28, 32, 37, 42 },
    { 10, 14, 20, 24, 14, 20, 24, 27,
    20, 24, 27, 30, 24, 27, 30, 34 }
};

static const uint8_t default_scaling8[2][64] = {
    { 6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42 },
    { 9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35 }
};

static const int level_max_dpb_mbs[][2] = {
    { 10, 396 },
    { 11, 900 },
    { 12, 2376 },
    { 13, 2376 },
    { 20, 2376 },
    { 21, 4752 },
    { 22, 8100 },
    { 30, 8100 },
    { 31, 18000 },
    { 32, 20480 },
    { 40, 32768 },
    { 41, 32768 },
    { 42, 34816 },
    { 50, 110400 },
    { 51, 184320 },
    { 52, 184320 },
};

const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

const uint8_t ff_zigzag_scan[16 + 1] = {
    0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
    1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
    1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
    3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};
const int8_t ff_se_golomb_vlc_code[512] = {
    17, 17, 17, 17, 17, 17, 17, 17, 16, 17, 17, 17, 17, 17, 17, 17,  8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
    4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};


const uint8_t ff_ue_golomb_vlc_code[512] = {
    32,32,32,32,32,32,32,32,31,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
    7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t ff_golomb_vlc_len[512] = {
    19,17,15,15,13,13,13,13,11,11,11,11,11,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};


const uint8_t ff_log2_tab[256] = {
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static const AVRational ff_h264_pixel_aspect[17] = {
    { 0,  1 },
    { 1,  1 },
    { 12, 11 },
    { 10, 11 },
    { 16, 11 },
    { 40, 33 },
    { 24, 11 },
    { 20, 11 },
    { 32, 11 },
    { 80, 33 },
    { 18, 11 },
    { 15, 11 },
    { 64, 33 },
    { 160, 99 },
    { 4,  3 },
    { 3,  2 },
    { 2,  1 }
};

const uint8_t ff_interleaved_golomb_vlc_len[256] = {
	 9,9,7,7,9,9,7,7,5,5,5,5,5,5,5,5,
	 9,9,7,7,9,9,7,7,5,5,5,5,5,5,5,5,
	 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	 9,9,7,7,9,9,7,7,5,5,5,5,5,5,5,5,
	 9,9,7,7,9,9,7,7,5,5,5,5,5,5,5,5,
	 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

const int8_t ff_interleaved_se_golomb_vlc_code[256] = {
	   8, -8,  4,  4,  9, -9, -4, -4,  2,  2,  2,  2,  2,  2,  2,  2,
	  10,-10,  5,  5, 11,-11, -5, -5, -2, -2, -2, -2, -2, -2, -2, -2,
	   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	  12,-12,  6,  6, 13,-13, -6, -6,  3,  3,  3,  3,  3,  3,  3,  3,
	  14,-14,  7,  7, 15,-15, -7, -7, -3, -3, -3, -3, -3, -3, -3, -3,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

const uint8_t ff_interleaved_ue_golomb_vlc_code[256] = {
	  15,16,7, 7, 17,18,8, 8, 3, 3, 3, 3, 3, 3, 3, 3,
	  19,20,9, 9, 21,22,10,10,4, 4, 4, 4, 4, 4, 4, 4,
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	  23,24,11,11,25,26,12,12,5, 5, 5, 5, 5, 5, 5, 5,
	  27,28,13,13,29,30,14,14,6, 6, 6, 6, 6, 6, 6, 6,
	  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

const uint8_t ff_interleaved_dirac_golomb_vlc_code[256] = {
	 0, 1, 0, 0, 2, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	 4, 5, 2, 2, 6, 7, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 8, 9, 4, 4, 10,11,5, 5, 2, 2, 2, 2, 2, 2, 2, 2,
	 12,13,6, 6, 14,15,7, 7, 3, 3, 3, 3, 3, 3, 3, 3,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

#ifndef ff_log2
#define ff_log2             ff_log2_c
static inline const int ff_log2_c(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
#endif // !ff_log2_c

#ifndef ff_log2_16bit
#define ff_log2_16bit       ff_log2_16bit_c
static const int ff_log2_16bit_c(unsigned int v)
{
    int n = 0;
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
#endif // !ff_log2_16bit_c

#ifndef zero_extend
static inline const unsigned zero_extend(unsigned val, unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}
#endif
#ifndef sign_extend
static inline int sign_extend(int val, unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned)val << shift };
    return v.s >> shift;
}
#endif

#define av_log2             ff_log2
#define av_log2_16bit       ff_log2_16bit
#define get_bits_count(s)   ((int)(s->index))
#define get_bitsz(s, n)     ((int)(n ? get_bits(s, n) : 0))
#define show_bits1(s)       ((unsigned int)show_bits(s, 1))
#define skip_bits1(s)       (skip_bits(s, 1))

static inline void skip_bits_long(GetBitContext *s, int n)
{
#if UNCHECKED_BITSTREAM_READER
    s->index += n;
#else
    s->index += av_clip(n, -s->index, s->size_in_bits_plus8 - s->index);
#endif
}

static inline int get_xbits(GetBitContext *s, int n)
{
    register int sign;
    register int32_t cache;
    OPEN_READER(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE(re, s);
    cache = GET_CACHE(re, s);
    sign = ~cache >> 31;
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return (NEG_USR32(sign ^ cache, n) ^ sign) - sign;
}

static inline int get_xbits_le(GetBitContext *s, int n)
{
    register int sign;
    register int32_t cache;
    OPEN_READER(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE_LE(re, s);
    cache = GET_CACHE(re, s);
    sign = sign_extend(~cache, n) >> 31;
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return (zero_extend(sign ^ cache, n) ^ sign) - sign;
}

static inline int get_sbits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE(re, s);
    tmp = SHOW_SBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

static inline unsigned int get_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

static inline unsigned int get_bits_le(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE_LE(re, s);
    tmp = SHOW_UBITS_LE(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

static inline unsigned int show_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER_NOSIZE(re, s);
    //av_assert2(n > 0 && n <= 25);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    return tmp;
}

static inline void skip_bits(GetBitContext *s, int n)
{
    OPEN_READER(re, s);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
}

static inline unsigned int get_bits1(GetBitContext *s)
{
    unsigned int index = s->index;
    uint8_t result = s->buffer[index >> 3];
#ifdef BITSTREAM_READER_LE
    result >>= index & 7;
    result &= 1;
#else
    result <<= index & 7;
    result >>= 8 - 1;
#endif
#if !UNCHECKED_BITSTREAM_READER
    if (s->index < s->size_in_bits_plus8)
#endif
        index++;
    s->index = index;

    return result;
}

static inline unsigned int get_bits_long(GetBitContext *s, int n)
{
    //av_assert2(n >= 0 && n <= 32);
    if (!n) {
        return 0;
    }
    else if (n <= MIN_CACHE_BITS) {
        return get_bits(s, n);
    }
    else {
#ifdef BITSTREAM_READER_LE
        unsigned ret = get_bits(s, 16);
        return ret | (get_bits(s, n - 16) << 16);
#else
        unsigned ret = get_bits(s, 16) << (n - 16);
        return ret | get_bits(s, n - 16);
#endif
    }
}

static inline uint64_t get_bits64(GetBitContext *s, int n)
{
    if (n <= 32) {
        return get_bits_long(s, n);
    }
    else {
#ifdef BITSTREAM_READER_LE
        uint64_t ret = get_bits_long(s, 32);
        return ret | (uint64_t)get_bits_long(s, n - 32) << 32;
#else
        uint64_t ret = (uint64_t)get_bits_long(s, n - 32) << 32;
        return ret | get_bits_long(s, 32);
#endif
    }
}

static inline unsigned int show_bits_long(GetBitContext *s, int n)
{
    if (n <= MIN_CACHE_BITS) {
        return show_bits(s, n);
    }
    else {
        GetBitContext gb = *s;
        return get_bits_long(&gb, n);
    }
}

static inline int init_get_bits(GetBitContext *s, const uint8_t *buffer,
    int bit_size)
{
    int buffer_size;
    int ret = 0;

    if (bit_size >= INT_MAX - FFMAX(7, AV_INPUT_BUFFER_PADDING_SIZE * 8) || bit_size < 0 || !buffer) {
        bit_size = 0;
        buffer = NULL;
        ret = -1;
    }

    buffer_size = (bit_size + 7) >> 3;

    s->buffer = buffer;
    s->size_in_bits = bit_size;
    s->size_in_bits_plus8 = bit_size + 8;
    s->buffer_end = buffer + buffer_size;
    s->index = 0;

    return ret;
}

static int init_get_bits8(GetBitContext *s, const uint8_t *buffer,
    int byte_size)
{
    if (byte_size > INT_MAX / 8 || byte_size < 0)
        byte_size = -1;
    return init_get_bits(s, buffer, byte_size * 8);
}

static inline const uint8_t *align_get_bits(GetBitContext *s)
{
    int n = -get_bits_count(s) & 7;
    if (n)
        skip_bits(s, n);
    return s->buffer + (s->index >> 3);
}
#if 0
#define VLC_TYPE int16_t
typedef struct VLC {
    int bits;
    VLC_TYPE(*table)[2]; ///< code, bits
    int table_size, table_allocated;
} VLC;

typedef struct RL_VLC_ELEM {
    int16_t level;
    int8_t len;
    uint8_t run;
} RL_VLC_ELEM;
#define GET_VLC(code, name, gb, table, bits, max_depth)         \
    do {                                                        \
        int n, nb_bits;                                         \
        unsigned int index;                                     \
                                                                \
        index = SHOW_UBITS(name, gb, bits);                     \
        code  = table[index][0];                                \
        n     = table[index][1];                                \
                                                                \
        if (max_depth > 1 && n < 0) {                           \
            LAST_SKIP_BITS(name, gb, bits);                     \
            UPDATE_CACHE(name, gb);                             \
                                                                \
            nb_bits = -n;                                       \
                                                                \
            index = SHOW_UBITS(name, gb, nb_bits) + code;       \
            code  = table[index][0];                            \
            n     = table[index][1];                            \
            if (max_depth > 2 && n < 0) {                       \
                LAST_SKIP_BITS(name, gb, nb_bits);              \
                UPDATE_CACHE(name, gb);                         \
                                                                \
                nb_bits = -n;                                   \
                                                                \
                index = SHOW_UBITS(name, gb, nb_bits) + code;   \
                code  = table[index][0];                        \
                n     = table[index][1];                        \
            }                                                   \
        }                                                       \
        SKIP_BITS(name, gb, n);                                 \
    } while (0)

#define GET_RL_VLC(level, run, name, gb, table, bits,  \
                   max_depth, need_update)                      \
    do {                                                        \
        int n, nb_bits;                                         \
        unsigned int index;                                     \
                                                                \
        index = SHOW_UBITS(name, gb, bits);                     \
        level = table[index].level;                             \
        n     = table[index].len;                               \
                                                                \
        if (max_depth > 1 && n < 0) {                           \
            SKIP_BITS(name, gb, bits);                          \
            if (need_update) {                                  \
                UPDATE_CACHE(name, gb);                         \
            }                                                   \
                                                                \
            nb_bits = -n;                                       \
                                                                \
            index = SHOW_UBITS(name, gb, nb_bits) + level;      \
            level = table[index].level;                         \
            n     = table[index].len;                           \
            if (max_depth > 2 && n < 0) {                       \
                LAST_SKIP_BITS(name, gb, nb_bits);              \
                if (need_update) {                              \
                    UPDATE_CACHE(name, gb);                     \
                }                                               \
                nb_bits = -n;                                   \
                                                                \
                index = SHOW_UBITS(name, gb, nb_bits) + level;  \
                level = table[index].level;                     \
                n     = table[index].len;                       \
            }                                                   \
        }                                                       \
        run = table[index].run;                                 \
        SKIP_BITS(name, gb, n);                                 \
    } while (0)


static inline int get_vlc2(GetBitContext *s, VLC_TYPE(*table)[2],
    int bits, int max_depth)
{
    int code;

    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);

    GET_VLC(code, re, s, table, bits, max_depth);

    CLOSE_READER(re, s);

    return code;
}
#endif

static inline int decode012(GetBitContext *gb)
{
    int n;
    n = get_bits1(gb);
    if (n == 0)
        return 0;
    else
        return get_bits1(gb) + 1;
}

static inline int decode210(GetBitContext *gb)
{
    if (get_bits1(gb))
        return 0;
    else
        return 2 - get_bits1(gb);
}

static inline int get_bits_left(GetBitContext *gb)
{
    return gb->size_in_bits - get_bits_count(gb);
}

static inline int skip_1stop_8data_bits(GetBitContext *gb)
{
    if (get_bits_left(gb) <= 0)
        return -1;

    while (get_bits1(gb)) {
        skip_bits(gb, 8);
        if (get_bits_left(gb) <= 0)
            return -1;
    }

    return 0;
}

static inline int get_se_golomb(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);

        return ff_se_golomb_vlc_code[buf];
    }
    else {
        int log = av_log2(buf), sign;
        LAST_SKIP_BITS(re, gb, 31 - log);
        UPDATE_CACHE(re, gb);
        buf = GET_CACHE(re, gb);

        buf >>= log;

        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);

        sign = 0 - (buf & 1);
        buf = ((buf >> 1) ^ sign) - sign;

        return buf;
    }
}

static inline int get_ue_golomb_31(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    buf >>= 32 - 9;
    LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
    CLOSE_READER(re, gb);

    return ff_ue_golomb_vlc_code[buf];
}

static inline unsigned get_ue_golomb_long(GetBitContext *gb)
{
    unsigned buf, log;

    buf = show_bits_long(gb, 32);
    log = 31 - av_log2(buf);
    skip_bits_long(gb, log);

    return get_bits_long(gb, log + 1) - 1;
}

static inline int get_ue_golomb(GetBitContext *gb)
{
    unsigned int buf;

    OPEN_READER(re, gb);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
        CLOSE_READER(re, gb);

        return ff_ue_golomb_vlc_code[buf];
    }
    else {
        int log = 2 * av_log2(buf) - 31;
        LAST_SKIP_BITS(re, gb, 32 - log);
        CLOSE_READER(re, gb);
        if (log < 7) {
            //av_log(NULL, AV_LOG_ERROR, "Invalid UE golomb code\n");
            return -1;
        }
        buf >>= log;
        buf--;

        return buf;
    }
}

static inline int get_sbits_long(GetBitContext *s, int n)
{
    // sign_extend(x, 0) is undefined
    if (!n)
        return 0;

    return sign_extend(get_bits_long(s, n), n);
}
static inline int check_marker(void *logctx, GetBitContext *s, const char *msg)
{
    int bit = get_bits1(s);
    //if (!bit)
    //     av_log(logctx, AV_LOG_INFO, "Marker bit missing at %d of %d %s\n",
    //         get_bits_count(s) - 1, s->size_in_bits, msg);

    return bit;
}


static int decode_scaling_list(
    GetBitContext *gb,
    uint8_t *factors,
    int size,
    const uint8_t *jvt_list,
    const uint8_t *fallback_list)
{
    int i, last = 8, next = 8;
    const uint8_t *scan = size == 16 ? ff_zigzag_scan : ff_zigzag_direct;
    if (!get_bits1(gb)) /* matrix not written, we use the predicted one */
        ::memcpy(factors, fallback_list, size * sizeof(uint8_t));
    else
        for (i = 0; i < size; i++) {
            if (next) {
                int v = get_se_golomb(gb);
                if (v < -128 || v > 127) {
                    //av_log(NULL, AV_LOG_ERROR, "delta scale %d is invalid\n", v);
                    return -1;
                }
                next = (last + v) & 0xff;
            }
            if (!i && !next) { /* matrix not written, we use the preset one */
                memcpy(factors, jvt_list, size * sizeof(uint8_t));
                break;
            }
            last = factors[scan[i]] = next ? next : last;
        }
    return 0;
}

static int decode_scaling_matrices(GetBitContext *gb, const SPS *sps, int is_sps,
    uint8_t(*scaling_matrix4)[16],
    uint8_t(*scaling_matrix8)[64])
{
    int fallback_sps = !is_sps && sps->scaling_matrix_present;
    const uint8_t *fallback[4] = {
        fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
        fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
        fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
        fallback_sps ? sps->scaling_matrix8[3] : default_scaling8[1]
    };
    int ret = 0;
    if (get_bits1(gb)) {
        ret |= decode_scaling_list(gb, scaling_matrix4[0], 16, default_scaling4[0], fallback[0]);        // Intra, Y
        ret |= decode_scaling_list(gb, scaling_matrix4[1], 16, default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
        ret |= decode_scaling_list(gb, scaling_matrix4[2], 16, default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
        ret |= decode_scaling_list(gb, scaling_matrix4[3], 16, default_scaling4[1], fallback[1]);        // Inter, Y
        ret |= decode_scaling_list(gb, scaling_matrix4[4], 16, default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
        ret |= decode_scaling_list(gb, scaling_matrix4[5], 16, default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
        if (is_sps) {
            ret |= decode_scaling_list(gb, scaling_matrix8[0], 64, default_scaling8[0], fallback[2]); // Intra, Y
            ret |= decode_scaling_list(gb, scaling_matrix8[3], 64, default_scaling8[1], fallback[3]); // Inter, Y
            if (sps->chroma_format_idc == 3) {
                ret |= decode_scaling_list(gb, scaling_matrix8[1], 64, default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
                ret |= decode_scaling_list(gb, scaling_matrix8[4], 64, default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
                ret |= decode_scaling_list(gb, scaling_matrix8[2], 64, default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
                ret |= decode_scaling_list(gb, scaling_matrix8[5], 64, default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
            }
        }
        if (!ret)
            ret = is_sps;
    }

    return ret;
}

static inline int decode_hrd_parameters(GetBitContext *gb, SPS *sps)
{
    int cpb_count, i;
    cpb_count = get_ue_golomb_31(gb) + 1;

    if (cpb_count > 32U) {
        //av_log(avctx, AV_LOG_ERROR, "cpb_count %d invalid\n", cpb_count);
        return -1;
    }

    get_bits(gb, 4); /* bit_rate_scale */
    get_bits(gb, 4); /* cpb_size_scale */
    for (i = 0; i < cpb_count; i++) {
        get_ue_golomb_long(gb); /* bit_rate_value_minus1 */
        get_ue_golomb_long(gb); /* cpb_size_value_minus1 */
        get_bits1(gb);          /* cbr_flag */
    }
    sps->initial_cpb_removal_delay_length = get_bits(gb, 5) + 1;
    sps->cpb_removal_delay_length = get_bits(gb, 5) + 1;
    sps->dpb_output_delay_length = get_bits(gb, 5) + 1;
    sps->time_offset_length = get_bits(gb, 5);
    sps->cpb_cnt = cpb_count;
    return 0;
}

static inline int decode_vui_parameters(GetBitContext *gb, SPS *sps)
{
    int aspect_ratio_info_present_flag;
    unsigned int aspect_ratio_idc;

    aspect_ratio_info_present_flag = get_bits1(gb);

    if (aspect_ratio_info_present_flag) {
        aspect_ratio_idc = get_bits(gb, 8);
        if (aspect_ratio_idc == EXTENDED_SAR) {
            sps->sar.num = get_bits(gb, 16);
            sps->sar.den = get_bits(gb, 16);
        }
        else if (aspect_ratio_idc < FF_ARRAY_ELEMS(ff_h264_pixel_aspect)) {
            sps->sar = ff_h264_pixel_aspect[aspect_ratio_idc];
        }
        else {
            //av_log(avctx, AV_LOG_ERROR, "illegal aspect ratio\n");
            return -1;
        }
    }
    else {
        sps->sar.num =
            sps->sar.den = 0;
    }

    if (get_bits1(gb))      /* overscan_info_present_flag */
        get_bits1(gb);      /* overscan_appropriate_flag */

    sps->video_signal_type_present_flag = get_bits1(gb);
    if (sps->video_signal_type_present_flag) {
        get_bits(gb, 3);                 /* video_format */
        sps->full_range = get_bits1(gb); /* video_full_range_flag */

        sps->colour_description_present_flag = get_bits1(gb);
        if (sps->colour_description_present_flag) {
            sps->color_primaries = (AVColorPrimaries)get_bits(gb, 8); /* colour_primaries */
            sps->color_trc = (AVColorTransferCharacteristic)get_bits(gb, 8); /* transfer_characteristics */
            sps->colorspace = (AVColorSpace)get_bits(gb, 8); /* matrix_coefficients */
        }
    }

    /* chroma_location_info_present_flag */
    if (get_bits1(gb)) {
        /* chroma_sample_location_type_top_field */
        get_ue_golomb(gb);
        get_ue_golomb(gb);  /* chroma_sample_location_type_bottom_field */
    }

    if (show_bits1(gb) && get_bits_left(gb) < 10) {
        //av_log(avctx, AV_LOG_WARNING, "Truncated VUI\n");
        return 0;
    }

    sps->timing_info_present_flag = get_bits1(gb);
    if (sps->timing_info_present_flag) {
        unsigned num_units_in_tick = get_bits_long(gb, 32);
        unsigned time_scale = get_bits_long(gb, 32);
        if (!num_units_in_tick || !time_scale) {
            //av_log(avctx, AV_LOG_ERROR,
            //    "time_scale/num_units_in_tick invalid or unsupported (%u/%u)\n",
            //    time_scale, num_units_in_tick);
            sps->timing_info_present_flag = 0;
        }
        else {
            sps->num_units_in_tick = num_units_in_tick;
            sps->time_scale = time_scale;
        }
        sps->fixed_frame_rate_flag = get_bits1(gb);
    }

    sps->nal_hrd_parameters_present_flag = get_bits1(gb);
    if (sps->nal_hrd_parameters_present_flag)
        if (decode_hrd_parameters(gb, sps) < 0)
            return -1;
    sps->vcl_hrd_parameters_present_flag = get_bits1(gb);
    if (sps->vcl_hrd_parameters_present_flag)
        if (decode_hrd_parameters(gb, sps) < 0)
            return -1;
    if (sps->nal_hrd_parameters_present_flag ||
        sps->vcl_hrd_parameters_present_flag)
        get_bits1(gb);     /* low_delay_hrd_flag */
    sps->pic_struct_present_flag = get_bits1(gb);
    if (!get_bits_left(gb))
        return 0;
    sps->bitstream_restriction_flag = get_bits1(gb);
    if (sps->bitstream_restriction_flag) {
        get_bits1(gb);     /* motion_vectors_over_pic_boundaries_flag */
        get_ue_golomb(gb); /* max_bytes_per_pic_denom */
        get_ue_golomb(gb); /* max_bits_per_mb_denom */
        get_ue_golomb(gb); /* log2_max_mv_length_horizontal */
        get_ue_golomb(gb); /* log2_max_mv_length_vertical */
        sps->num_reorder_frames = get_ue_golomb(gb);
        get_ue_golomb(gb); /*max_dec_frame_buffering*/

        if (get_bits_left(gb) < 0) {
            sps->num_reorder_frames = 0;
            sps->bitstream_restriction_flag = 0;
        }

        if (sps->num_reorder_frames > 16U
            /* max_dec_frame_buffering || max_dec_frame_buffering > 16 */) {
            // av_log(avctx, AV_LOG_ERROR,
            //    "Clipping illegal num_reorder_frames %d\n",
            //     sps->num_reorder_frames);
            sps->num_reorder_frames = 16;
            return -1;
        }
    }

    return 0;
}

int ff_h264_decode_seq_parameter_set(GetBitContext *gb, SPS *sps, int ignore_truncation)
{
    int profile_idc, level_idc, constraint_set_flags = 0;
    unsigned int sps_id;
    int i, log2_max_frame_num_minus4;
    int ret;

    sps->data_size = gb->buffer_end - gb->buffer;
    if (sps->data_size > sizeof(sps->data)) {
        //av_log(avctx, AV_LOG_DEBUG, "Truncating likely oversized SPS\n");
        sps->data_size = sizeof(sps->data);
    }
    memcpy(sps->data, gb->buffer, sps->data_size);

    profile_idc = get_bits(gb, 8);
    constraint_set_flags |= get_bits1(gb) << 0;   // constraint_set0_flag
    constraint_set_flags |= get_bits1(gb) << 1;   // constraint_set1_flag
    constraint_set_flags |= get_bits1(gb) << 2;   // constraint_set2_flag
    constraint_set_flags |= get_bits1(gb) << 3;   // constraint_set3_flag
    constraint_set_flags |= get_bits1(gb) << 4;   // constraint_set4_flag
    constraint_set_flags |= get_bits1(gb) << 5;   // constraint_set5_flag
    skip_bits(gb, 2);                             // reserved_zero_2bits
    level_idc = get_bits(gb, 8);
    sps_id = get_ue_golomb_31(gb);

    if (sps_id >= MAX_SPS_COUNT) {
        //av_log(avctx, AV_LOG_ERROR, "sps_id %u out of range\n", sps_id);
        return -1;
    }

    sps->sps_id = sps_id;
    sps->time_offset_length = 24;
    sps->profile_idc = profile_idc;
    sps->constraint_set_flags = constraint_set_flags;
    sps->level_idc = level_idc;
    sps->full_range = -1;

    memset(sps->scaling_matrix4, 16, sizeof(sps->scaling_matrix4));
    memset(sps->scaling_matrix8, 16, sizeof(sps->scaling_matrix8));
    sps->scaling_matrix_present = 0;
    sps->colorspace = (AVColorSpace)2; //AVCOL_SPC_UNSPECIFIED
    if (sps->profile_idc == 100 ||  // High profile
        sps->profile_idc == 110 ||  // High10 profile
        sps->profile_idc == 122 ||  // High422 profile
        sps->profile_idc == 244 ||  // High444 Predictive profile
        sps->profile_idc == 44 ||  // Cavlc444 profile
        sps->profile_idc == 83 ||  // Scalable Constrained High profile (SVC)
        sps->profile_idc == 86 ||  // Scalable High Intra profile (SVC)
        sps->profile_idc == 118 ||  // Stereo High profile (MVC)
        sps->profile_idc == 128 ||  // Multiview High profile (MVC)
        sps->profile_idc == 138 ||  // Multiview Depth High profile (MVCD)
        sps->profile_idc == 144) {  // old High444 profile
        sps->chroma_format_idc = get_ue_golomb_31(gb);
        if (sps->chroma_format_idc > 3U) {
            //avpriv_request_sample(avctx, "chroma_format_idc %u",
            //    sps->chroma_format_idc);
            return -1;
        }
        else if (sps->chroma_format_idc == 3) {
            sps->residual_color_transform_flag = get_bits1(gb);
            if (sps->residual_color_transform_flag) {
                //av_log(avctx, AV_LOG_ERROR, "separate color planes are not supported\n");
                //    goto fail;
            }
        }
        sps->bit_depth_luma = get_ue_golomb(gb) + 8;
        sps->bit_depth_chroma = get_ue_golomb(gb) + 8;
        if (sps->bit_depth_chroma != sps->bit_depth_luma) {
            //avpriv_request_sample(avctx,
            //   "Different chroma and luma bit depth");
            return -1;
        }
        if (sps->bit_depth_luma < 8 || sps->bit_depth_luma   > 14 ||
            sps->bit_depth_chroma < 8 || sps->bit_depth_chroma > 14) {
            //av_log(avctx, AV_LOG_ERROR, "illegal bit depth value (%d, %d)\n",
            //    sps->bit_depth_luma, sps->bit_depth_chroma);
            return -1;
        }
        sps->transform_bypass = get_bits1(gb);
        ret = decode_scaling_matrices(gb, sps, 1,
            sps->scaling_matrix4, sps->scaling_matrix8);
        if (ret < 0)
            return -1;
        sps->scaling_matrix_present |= ret;
    }
    else {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma = 8;
        sps->bit_depth_chroma = 8;
    }

    log2_max_frame_num_minus4 = get_ue_golomb(gb);
    if (log2_max_frame_num_minus4 < MIN_LOG2_MAX_FRAME_NUM - 4 ||
        log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM - 4) {
        //av_log(avctx, AV_LOG_ERROR,
        //    "log2_max_frame_num_minus4 out of range (0-12): %d\n",
        //    log2_max_frame_num_minus4);
        return -1;
    }
    sps->log2_max_frame_num = log2_max_frame_num_minus4 + 4;

    sps->poc_type = get_ue_golomb_31(gb);

    if (sps->poc_type == 0) { // FIXME #define
        unsigned t = get_ue_golomb(gb);
        if (t > 12) {
            //av_log(avctx, AV_LOG_ERROR, "log2_max_poc_lsb (%d) is out of range\n", t);
            return -1;
        }
        sps->log2_max_poc_lsb = t + 4;
    }
    else if (sps->poc_type == 1) { // FIXME #define
        sps->delta_pic_order_always_zero_flag = get_bits1(gb);
        sps->offset_for_non_ref_pic = get_se_golomb(gb);
        sps->offset_for_top_to_bottom_field = get_se_golomb(gb);
        sps->poc_cycle_length = get_ue_golomb(gb);

        if ((unsigned)sps->poc_cycle_length >=
            FF_ARRAY_ELEMS(sps->offset_for_ref_frame)) {
            //av_log(avctx, AV_LOG_ERROR,
            //    "poc_cycle_length overflow %d\n", sps->poc_cycle_length);
            return -1;
        }

        for (i = 0; i < sps->poc_cycle_length; i++)
            sps->offset_for_ref_frame[i] = get_se_golomb(gb);
    }
    else if (sps->poc_type != 2) {
        //av_log(avctx, AV_LOG_ERROR, "illegal POC type %d\n", sps->poc_type);
        return -1;
    }

    sps->ref_frame_count = get_ue_golomb_31(gb);
    sps->gaps_in_frame_num_allowed_flag = get_bits1(gb);
    sps->mb_width = get_ue_golomb(gb) + 1;
    sps->mb_height = get_ue_golomb(gb) + 1;

    //int nWidth = 16 * sps->mb_width;
    //int nHeight = 16 * sps->mb_height;

    sps->frame_mbs_only_flag = get_bits1(gb);

    if (sps->mb_height >= INT_MAX / 2U) {
        //av_log(avctx, AV_LOG_ERROR, "height overflow\n");
        return -1;
    }

    sps->mb_height *= 2 - sps->frame_mbs_only_flag;

    if (!sps->frame_mbs_only_flag)
        sps->mb_aff = get_bits1(gb);
    else
        sps->mb_aff = 0;


    sps->direct_8x8_inference_flag = get_bits1(gb);

    sps->crop = get_bits1(gb);
    if (sps->crop) {
        unsigned int crop_left = get_ue_golomb(gb);
        unsigned int crop_right = get_ue_golomb(gb);
        unsigned int crop_top = get_ue_golomb(gb);
        unsigned int crop_bottom = get_ue_golomb(gb);
        int width = 16 * sps->mb_width;
        int height = 16 * sps->mb_height;
    }
    else {
        sps->crop_left =
            sps->crop_right =
            sps->crop_top =
            sps->crop_bottom =
            sps->crop = 0;
    }

    sps->vui_parameters_present_flag = get_bits1(gb);
    if (sps->vui_parameters_present_flag) {
        int ret = decode_vui_parameters(gb, sps);
        if (ret < 0)
            return -1;
    }

    if (get_bits_left(gb) < 0) {
        //av_log(avctx, ignore_truncation ? AV_LOG_WARNING : AV_LOG_ERROR,
        //    "Overread %s by %d bits\n", sps->vui_parameters_present_flag ? "VUI" : "SPS", -get_bits_left(gb));
        if (!ignore_truncation)
            return -1;
    }
    return 0;
}


int get_h264_sps_info(uint8_t* buff, int nLen, int& nWidth, int& nHeight, int& nFrameRate)
{
    GetBitContext gb;
    SPS sps = { 0 };
    nWidth = 0;
    nHeight = 0;
    nFrameRate = 0;
    if (init_get_bits8(&gb, buff, sizeof(nLen)) < 0)
        return -1;
    if (ff_h264_decode_seq_parameter_set(&gb, &sps, 1) < 0)
        return -1;

    nWidth = sps.mb_width * 16;
    nHeight = sps.mb_height * 16;
    if (sps.timing_info_present_flag)
        nFrameRate = sps.time_scale / sps.num_units_in_tick;
    else
        nFrameRate = 25;
    return 0;
}

void av_free(void *ptr)
{
#if HAVE_ALIGNED_MALLOC
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

void av_freep(void *arg)
{
	void *val;

	memcpy(&val, arg, sizeof(val));
	memset(arg, 0, sizeof(val));
	//memcpy(arg, &(void *){ NULL }, sizeof(val));//c99
	av_free(val);
}

void *av_realloc(void *ptr, size_t size)
{
    /* let's disallow possibly ambiguous cases */
    if (size > (INT_MAX - 32))
        return NULL;

#if HAVE_ALIGNED_MALLOC
    return _aligned_realloc(ptr, size + !size, ALIGN);
#else
    return realloc(ptr, size + !size);
#endif
}

static inline int av_size_mult(size_t a, size_t b, size_t *r)
{
    size_t t = a * b;
    /* Hack inspired from glibc: don't try the division if nelem and elsize
     * are both less than sqrt(SIZE_MAX). */
    if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
        return -1;
	* r = t;
    return 0;
}

void *av_realloc_f(void *ptr, size_t nelem, size_t elsize)
{
    size_t size;
    void *r;

    if (av_size_mult(elsize, nelem, &size)) {
        av_free(ptr);
        return NULL;
	}
    r = av_realloc(ptr, size);
    if (!r)
        av_free(ptr);
    return r;
}

int av_reallocp_array(void *ptr, size_t nmemb, size_t size)
{
    void *val;

    memcpy(&val, ptr, sizeof(val));
    val = av_realloc_f(val, nmemb, size);
    memcpy(ptr, &val, sizeof(val));
    if (!val && nmemb && size)
        return -1;

    return 0;
}

static void hvcc_update_ptl(HEVCDecoderConfigurationRecord *hvcc,
                            HVCCProfileTierLevel *ptl)
{
     /*
      * The value of general_profile_space in all the parameter sets must be
      * identical.
      */
     hvcc->general_profile_space = ptl->profile_space;

     /*
      * The level indication general_level_idc must indicate a level of
      * capability equal to or greater than the highest level indicated for the
      * highest tier in all the parameter sets.
      */
     if (hvcc->general_tier_flag < ptl->tier_flag)
         hvcc->general_level_idc = ptl->level_idc;
     else
         hvcc->general_level_idc = FFMAX(hvcc->general_level_idc, ptl->level_idc);

     /*
      * The tier indication general_tier_flag must indicate a tier equal to or
      * greater than the highest tier indicated in all the parameter sets.
      */
     hvcc->general_tier_flag = FFMAX(hvcc->general_tier_flag, ptl->tier_flag);

     /*
      * The profile indication general_profile_idc must indicate a profile to
      * which the stream associated with this configuration record conforms.
      *
      * If the sequence parameter sets are marked with different profiles, then
      * the stream may need examination to determine which profile, if any, the
      * entire stream conforms to. If the entire stream is not examined, or the
      * examination reveals that there is no profile to which the entire stream
      * conforms, then the entire stream must be split into two or more
      * sub-streams with separate configuration records in which these rules can
      * be met.
      *
      * Note: set the profile to the highest value for the sake of simplicity.
      */
     hvcc->general_profile_idc = FFMAX(hvcc->general_profile_idc, ptl->profile_idc);

     /*
      * Each bit in general_profile_compatibility_flags may only be set if all
      * the parameter sets set that bit.
      */
     hvcc->general_profile_compatibility_flags &= ptl->profile_compatibility_flags;

     /*
      * Each bit in general_constraint_indicator_flags may only be set if all
      * the parameter sets set that bit.
      */
     hvcc->general_constraint_indicator_flags &= ptl->constraint_indicator_flags;
}


static void hvcc_parse_ptl(GetBitContext *gb,
                            HEVCDecoderConfigurationRecord *hvcc,
                            unsigned int max_sub_layers_minus1)
{
    unsigned int i;
    HVCCProfileTierLevel general_ptl;
    uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];

    general_ptl.profile_space = get_bits(gb, 2);
    general_ptl.tier_flag = get_bits1(gb);
    general_ptl.profile_idc = get_bits(gb, 5);
    general_ptl.profile_compatibility_flags = get_bits_long(gb, 32);
    general_ptl.constraint_indicator_flags = get_bits64(gb, 48);
    general_ptl.level_idc = get_bits(gb, 8);
    hvcc_update_ptl(hvcc, &general_ptl);

    for (i = 0; i < max_sub_layers_minus1; i++) {
         sub_layer_profile_present_flag[i] = get_bits1(gb);
         sub_layer_level_present_flag[i] = get_bits1(gb);

	}

    if (max_sub_layers_minus1 > 0)
         for (i = max_sub_layers_minus1; i < 8; i++)
             skip_bits(gb, 2); // reserved_zero_2bits[i]

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
             /*
              * sub_layer_profile_space[i]                     u(2)
              * sub_layer_tier_flag[i]                         u(1)
              * sub_layer_profile_idc[i]                       u(5)
              * sub_layer_profile_compatibility_flag[i][0..31] u(32)
              * sub_layer_progressive_source_flag[i]           u(1)
              * sub_layer_interlaced_source_flag[i]            u(1)
              * sub_layer_non_packed_constraint_flag[i]        u(1)
              * sub_layer_frame_only_constraint_flag[i]        u(1)
              * sub_layer_reserved_zero_44bits[i]              u(44)
              */
             skip_bits_long(gb, 32);
             skip_bits_long(gb, 32);
             skip_bits(gb, 24);
		}

         if (sub_layer_level_present_flag[i])
             skip_bits(gb, 8);

	}
}

static void skip_sub_layer_hrd_parameters(GetBitContext *gb,
                                           unsigned int cpb_cnt_minus1,
                                           uint8_t sub_pic_hrd_params_present_flag)
{
    unsigned int i;

    for (i = 0; i <= cpb_cnt_minus1; i++) {
        get_ue_golomb_long(gb); // bit_rate_value_minus1
        get_ue_golomb_long(gb); // cpb_size_value_minus1

        if (sub_pic_hrd_params_present_flag) {
             get_ue_golomb_long(gb); // cpb_size_du_value_minus1
             get_ue_golomb_long(gb); // bit_rate_du_value_minus1
		}

        skip_bits1(gb); // cbr_flag

	}
}

static int skip_hrd_parameters(GetBitContext *gb, uint8_t cprms_present_flag,
                                 unsigned int max_sub_layers_minus1)
{
     unsigned int i;
     uint8_t sub_pic_hrd_params_present_flag = 0;
     uint8_t nal_hrd_parameters_present_flag = 0;
     uint8_t vcl_hrd_parameters_present_flag = 0;

    if (cprms_present_flag) {
         nal_hrd_parameters_present_flag = get_bits1(gb);
         vcl_hrd_parameters_present_flag = get_bits1(gb);

        if (nal_hrd_parameters_present_flag ||
             vcl_hrd_parameters_present_flag) {
             sub_pic_hrd_params_present_flag = get_bits1(gb);

             if (sub_pic_hrd_params_present_flag)
                 /*
                  * tick_divisor_minus2                          u(8)
                  * du_cpb_removal_delay_increment_length_minus1 u(5)
                  * sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
                  * dpb_output_delay_du_length_minus1            u(5)
                  */
                 skip_bits(gb, 19);

             /*
              * bit_rate_scale u(4)
              * cpb_size_scale u(4)
              */
             skip_bits(gb, 8);

             if (sub_pic_hrd_params_present_flag)
                 skip_bits(gb, 4); // cpb_size_du_scale

             /*
              * initial_cpb_removal_delay_length_minus1 u(5)
              * au_cpb_removal_delay_length_minus1      u(5)
              * dpb_output_delay_length_minus1          u(5)
              */
             skip_bits(gb, 15);
		}

	}

    for (i = 0; i <= max_sub_layers_minus1; i++) {
         unsigned int cpb_cnt_minus1 = 0;
         uint8_t low_delay_hrd_flag = 0;
         uint8_t fixed_pic_rate_within_cvs_flag = 0;
         uint8_t fixed_pic_rate_general_flag = get_bits1(gb);

         if (!fixed_pic_rate_general_flag)
             fixed_pic_rate_within_cvs_flag = get_bits1(gb);

        if (fixed_pic_rate_within_cvs_flag)
             get_ue_golomb_long(gb); // elemental_duration_in_tc_minus1
        else
             low_delay_hrd_flag = get_bits1(gb);

        if (!low_delay_hrd_flag) {
             cpb_cnt_minus1 = get_ue_golomb_long(gb);
             if (cpb_cnt_minus1 > 31)
                 return -1;
		}

        if (nal_hrd_parameters_present_flag)
             skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                           sub_pic_hrd_params_present_flag);

        if (vcl_hrd_parameters_present_flag)
             skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                           sub_pic_hrd_params_present_flag);
	}

    return 0;
}

static void skip_timing_info(GetBitContext *gb)
{
     skip_bits_long(gb, 32); // num_units_in_tick
     skip_bits_long(gb, 32); // time_scale

     if (get_bits1(gb))          // poc_proportional_to_timing_flag
         get_ue_golomb_long(gb); // num_ticks_poc_diff_one_minus1
}

static void hvcc_parse_vui(GetBitContext *gb,
                            HEVCDecoderConfigurationRecord *hvcc,
                            unsigned int max_sub_layers_minus1)
{
     unsigned int min_spatial_segmentation_idc;

     if (get_bits1(gb))              // aspect_ratio_info_present_flag
         if (get_bits(gb, 8) == 255) // aspect_ratio_idc
             skip_bits_long(gb, 32); // sar_width u(16), sar_height u(16)

    if (get_bits1(gb))  // overscan_info_present_flag
         skip_bits1(gb); // overscan_appropriate_flag

    if (get_bits1(gb)) {  // video_signal_type_present_flag
         skip_bits(gb, 4); // video_format u(3), video_full_range_flag u(1)

         if (get_bits1(gb)) // colour_description_present_flag
             /*
              * colour_primaries         u(8)
              * transfer_characteristics u(8)
              * matrix_coeffs            u(8)
              */
             skip_bits(gb, 24);
	}

    if (get_bits1(gb)) {        // chroma_loc_info_present_flag
         get_ue_golomb_long(gb); // chroma_sample_loc_type_top_field
         get_ue_golomb_long(gb); // chroma_sample_loc_type_bottom_field
	}

     /*
      * neutral_chroma_indication_flag u(1)
      * field_seq_flag                 u(1)
      * frame_field_info_present_flag  u(1)
      */
     skip_bits(gb, 3);

    if (get_bits1(gb)) {        // default_display_window_flag
         get_ue_golomb_long(gb); // def_disp_win_left_offset
         get_ue_golomb_long(gb); // def_disp_win_right_offset
         get_ue_golomb_long(gb); // def_disp_win_top_offset
         get_ue_golomb_long(gb); // def_disp_win_bottom_offset
	}

    if (get_bits1(gb)) { // vui_timing_info_present_flag
         skip_timing_info(gb);

         if (get_bits1(gb)) // vui_hrd_parameters_present_flag
             skip_hrd_parameters(gb, 1, max_sub_layers_minus1);
	}

    if (get_bits1(gb)) { // bitstream_restriction_flag
         /*
          * tiles_fixed_structure_flag              u(1)
          * motion_vectors_over_pic_boundaries_flag u(1)
          * restricted_ref_pic_lists_flag           u(1)
          */
         skip_bits(gb, 3);

         min_spatial_segmentation_idc = get_ue_golomb_long(gb);

         /*
          * unsigned int(12) min_spatial_segmentation_idc;
          *
          * The min_spatial_segmentation_idc indication must indicate a level of
          * spatial segmentation equal to or less than the lowest level of
          * spatial segmentation indicated in all the parameter sets.
          */
         hvcc->min_spatial_segmentation_idc = FFMIN(hvcc->min_spatial_segmentation_idc,
                                                    min_spatial_segmentation_idc);

         get_ue_golomb_long(gb); // max_bytes_per_pic_denom
         get_ue_golomb_long(gb); // max_bits_per_min_cu_denom
         get_ue_golomb_long(gb); // log2_max_mv_length_horizontal
         get_ue_golomb_long(gb); // log2_max_mv_length_vertical
	}
}

static void skip_sub_layer_ordering_info(GetBitContext *gb)
{
     get_ue_golomb_long(gb); // max_dec_pic_buffering_minus1
     get_ue_golomb_long(gb); // max_num_reorder_pics
     get_ue_golomb_long(gb); // max_latency_increase_plus1
}

static int hvcc_parse_vps(GetBitContext *gb,
                           HEVCDecoderConfigurationRecord *hvcc)
{
     unsigned int vps_max_sub_layers_minus1;

     /*
      * vps_video_parameter_set_id u(4)
      * vps_reserved_three_2bits   u(2)
      * vps_max_layers_minus1      u(6)
      */
     skip_bits(gb, 12);

     vps_max_sub_layers_minus1 = get_bits(gb, 3);

     /*
      * numTemporalLayers greater than 1 indicates that the stream to which this
      * configuration record applies is temporally scalable and the contained
      * number of temporal layers (also referred to as temporal sub-layer or
      * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
      * indicates that the stream is not temporally scalable. Value 0 indicates
      * that it is unknown whether the stream is temporally scalable.
      */
     hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers,
                                     vps_max_sub_layers_minus1 + 1);

     /*
      * vps_temporal_id_nesting_flag u(1)
      * vps_reserved_0xffff_16bits   u(16)
      */
     skip_bits(gb, 17);

     hvcc_parse_ptl(gb, hvcc, vps_max_sub_layers_minus1);

     /* nothing useful for hvcC past this point */
     return 0;
}

static inline int get_se_golomb_long(GetBitContext *gb)
{
     unsigned int buf = get_ue_golomb_long(gb);
     int sign = (buf & 1) - 1;
     return ((buf >> 1) ^ sign) + 1;
}

static void skip_scaling_list_data(GetBitContext *gb)
{
     int i, j, k, num_coeffs;

     for (i = 0; i < 4; i++)
         for (j = 0; j < (i == 3 ? 2 : 6); j++)
             if (!get_bits1(gb))         // scaling_list_pred_mode_flag[i][j]
                 get_ue_golomb_long(gb); // scaling_list_pred_matrix_id_delta[i][j]
             else {
                 num_coeffs = FFMIN(64, 1 << (4 + (i << 1)));

                 if (i > 1)
                     get_se_golomb_long(gb); // scaling_list_dc_coef_minus8[i-2][j]

                 for (k = 0; k < num_coeffs; k++)
                     get_se_golomb_long(gb); // scaling_list_delta_coef
	}
}

static int parse_rps(GetBitContext *gb, unsigned int rps_idx,
                     unsigned int num_rps,
                     unsigned int num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS])
{
    unsigned int i;

    if (rps_idx && get_bits1(gb)) { // inter_ref_pic_set_prediction_flag
        /* this should only happen for slice headers, and this isn't one */
        if (rps_idx >= num_rps)
            return -1;

        skip_bits1(gb); // delta_rps_sign
        get_ue_golomb_long(gb); // abs_delta_rps_minus1

        num_delta_pocs[rps_idx] = 0;

        /*
         * From libavcodec/hevc_ps.c:
         *
         * if (is_slice_header) {
         *    //foo
         * } else
         *     rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];
         *
         * where:
         * rps:             &sps->st_rps[rps_idx]
         * sps->st_rps:     &sps->st_rps[0]
         * is_slice_header: rps_idx == num_rps
         *
         * thus:
         * if (num_rps != rps_idx)
         *     rps_ridx = &sps->st_rps[rps_idx - 1];
         *
         * NumDeltaPocs[RefRpsIdx]: num_delta_pocs[rps_idx - 1]
         */
        for (i = 0; i <= num_delta_pocs[rps_idx - 1]; i++) {
            uint8_t use_delta_flag = 0;
            uint8_t used_by_curr_pic_flag = get_bits1(gb);
            if (!used_by_curr_pic_flag)
                use_delta_flag = get_bits1(gb);

            if (used_by_curr_pic_flag || use_delta_flag)
                num_delta_pocs[rps_idx]++;
		}

	}
	else {
        unsigned int num_negative_pics = get_ue_golomb_long(gb);
        unsigned int num_positive_pics = get_ue_golomb_long(gb);

        if ((num_positive_pics + (uint64_t)num_negative_pics) * 2 > get_bits_left(gb))
            return -1;

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            get_ue_golomb_long(gb); // delta_poc_s0_minus1[rps_idx]
            skip_bits1(gb); // used_by_curr_pic_s0_flag[rps_idx]
		}

        for (i = 0; i < num_positive_pics; i++) {
            get_ue_golomb_long(gb); // delta_poc_s1_minus1[rps_idx]
            skip_bits1(gb); // used_by_curr_pic_s1_flag[rps_idx]
		}
	}
    return 0;
}


static int hvcc_parse_sps(GetBitContext *gb,
                           HEVCDecoderConfigurationRecord *hvcc)
{
    unsigned int i, sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
    unsigned int num_short_term_ref_pic_sets, num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];

    skip_bits(gb, 4); // sps_video_parameter_set_id

    sps_max_sub_layers_minus1 = get_bits(gb, 3);

    /*
     * numTemporalLayers greater than 1 indicates that the stream to which this
     * configuration record applies is temporally scalable and the contained
     * number of temporal layers (also referred to as temporal sub-layer or
     * sub-layer in ISO/IEC 23008-2) is equal to numTemporalLayers. Value 1
     * indicates that the stream is not temporally scalable. Value 0 indicates
     * that it is unknown whether the stream is temporally scalable.
     */
    hvcc->numTemporalLayers = FFMAX(hvcc->numTemporalLayers,
                                    sps_max_sub_layers_minus1 + 1);

    hvcc->temporalIdNested = get_bits1(gb);

    hvcc_parse_ptl(gb, hvcc, sps_max_sub_layers_minus1);

    get_ue_golomb_long(gb); // sps_seq_parameter_set_id

    hvcc->chromaFormat = get_ue_golomb_long(gb);

    if (hvcc->chromaFormat == 3)
        skip_bits1(gb); // separate_colour_plane_flag

    get_ue_golomb_long(gb); // pic_width_in_luma_samples
    get_ue_golomb_long(gb); // pic_height_in_luma_samples

    if (get_bits1(gb)) {        // conformance_window_flag
        get_ue_golomb_long(gb); // conf_win_left_offset
        get_ue_golomb_long(gb); // conf_win_right_offset
        get_ue_golomb_long(gb); // conf_win_top_offset
        get_ue_golomb_long(gb); // conf_win_bottom_offset
	}

    hvcc->bitDepthLumaMinus8 = get_ue_golomb_long(gb);
    hvcc->bitDepthChromaMinus8 = get_ue_golomb_long(gb);
    log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb_long(gb);

    /* sps_sub_layer_ordering_info_present_flag */
    i = get_bits1(gb) ? 0 : sps_max_sub_layers_minus1;
    for (; i <= sps_max_sub_layers_minus1; i++)
        skip_sub_layer_ordering_info(gb);

    get_ue_golomb_long(gb); // log2_min_luma_coding_block_size_minus3
    get_ue_golomb_long(gb); // log2_diff_max_min_luma_coding_block_size
    get_ue_golomb_long(gb); // log2_min_transform_block_size_minus2
    get_ue_golomb_long(gb); // log2_diff_max_min_transform_block_size
    get_ue_golomb_long(gb); // max_transform_hierarchy_depth_inter
    get_ue_golomb_long(gb); // max_transform_hierarchy_depth_intra

    if (get_bits1(gb) && // scaling_list_enabled_flag
        get_bits1(gb))   // sps_scaling_list_data_present_flag
        skip_scaling_list_data(gb);

    skip_bits1(gb); // amp_enabled_flag
    skip_bits1(gb); // sample_adaptive_offset_enabled_flag

    if (get_bits1(gb)) {           // pcm_enabled_flag
        skip_bits(gb, 4); // pcm_sample_bit_depth_luma_minus1
        skip_bits(gb, 4); // pcm_sample_bit_depth_chroma_minus1
        get_ue_golomb_long(gb);    // log2_min_pcm_luma_coding_block_size_minus3
        get_ue_golomb_long(gb);    // log2_diff_max_min_pcm_luma_coding_block_size
        skip_bits1(gb);    // pcm_loop_filter_disabled_flag
	}

    num_short_term_ref_pic_sets = get_ue_golomb_long(gb);
    if (num_short_term_ref_pic_sets > HEVC_MAX_SHORT_TERM_REF_PIC_SETS)
        return -1;

    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        int ret = parse_rps(gb, i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (ret < 0)
            return ret;
	}

    if (get_bits1(gb)) {                               // long_term_ref_pics_present_flag
        unsigned num_long_term_ref_pics_sps = get_ue_golomb_long(gb);
        if (num_long_term_ref_pics_sps > 31U)
            return -1;
        for (i = 0; i < num_long_term_ref_pics_sps; i++) { // num_long_term_ref_pics_sps
            int len = FFMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
            skip_bits(gb, len); // lt_ref_pic_poc_lsb_sps[i]
            skip_bits1(gb);      // used_by_curr_pic_lt_sps_flag[i]
		}
	}

    skip_bits1(gb); // sps_temporal_mvp_enabled_flag
    skip_bits1(gb); // strong_intra_smoothing_enabled_flag

    if (get_bits1(gb)) // vui_parameters_present_flag
        hvcc_parse_vui(gb, hvcc, sps_max_sub_layers_minus1);

    /* nothing useful for hvcC past this point */
    return 0;
}

static int hvcc_parse_pps(GetBitContext *gb,
                          HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t tiles_enabled_flag, entropy_coding_sync_enabled_flag;

    get_ue_golomb_long(gb); // pps_pic_parameter_set_id
    get_ue_golomb_long(gb); // pps_seq_parameter_set_id

    /*
     * dependent_slice_segments_enabled_flag u(1)
     * output_flag_present_flag              u(1)
     * num_extra_slice_header_bits           u(3)
     * sign_data_hiding_enabled_flag         u(1)
     * cabac_init_present_flag               u(1)
     */
    skip_bits(gb, 7);

    get_ue_golomb_long(gb); // num_ref_idx_l0_default_active_minus1
    get_ue_golomb_long(gb); // num_ref_idx_l1_default_active_minus1
    get_se_golomb_long(gb); // init_qp_minus26

    /*
     * constrained_intra_pred_flag u(1)
     * transform_skip_enabled_flag u(1)
     */
    skip_bits(gb, 2);

    if (get_bits1(gb))          // cu_qp_delta_enabled_flag
        get_ue_golomb_long(gb); // diff_cu_qp_delta_depth

    get_se_golomb_long(gb); // pps_cb_qp_offset
    get_se_golomb_long(gb); // pps_cr_qp_offset

    /*
     * pps_slice_chroma_qp_offsets_present_flag u(1)
     * weighted_pred_flag               u(1)
     * weighted_bipred_flag             u(1)
     * transquant_bypass_enabled_flag   u(1)
     */
    skip_bits(gb, 4);

    tiles_enabled_flag = get_bits1(gb);
    entropy_coding_sync_enabled_flag = get_bits1(gb);

    if (entropy_coding_sync_enabled_flag && tiles_enabled_flag)
        hvcc->parallelismType = 0; // mixed-type parallel decoding
    else if (entropy_coding_sync_enabled_flag)
        hvcc->parallelismType = 3; // wavefront-based parallel decoding
    else if (tiles_enabled_flag)
        hvcc->parallelismType = 2; // tile-based parallel decoding
    else
        hvcc->parallelismType = 1; // slice-based parallel decoding

    /* nothing useful for hvcC past this point */
    return 0;
}

static uint8_t *nal_unit_extract_rbsp(const uint8_t *src, uint32_t src_len,
                                      uint32_t *dst_len)
{
    uint8_t *dst;
    uint32_t i, len;

    dst = (uint8_t *)malloc(src_len + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!dst)
        return NULL;

    /* NAL unit header (2 bytes) */
    i = len = 0;
    while (i < 2 && i < src_len)
        dst[len++] = src[i++];

    while (i + 2 < src_len)
        if (!src[i] && !src[i + 1] && src[i + 2] == 3) {
            dst[len++] = src[i++];
            dst[len++] = src[i++];
            i++; // remove emulation_prevention_three_byte
	}
	else
        dst[len++] = src[i++];

    while (i < src_len)
        dst[len++] = src[i++];

    memset(dst + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	 * dst_len = len;
    return dst;
}

static void nal_unit_parse_header(GetBitContext *gb, uint8_t *nal_type)
{
    skip_bits1(gb); // forbidden_zero_bit
	 * nal_type = get_bits(gb, 6);

     /*
      * nuh_layer_id          u(6)
      * nuh_temporal_id_plus1 u(3)
      */
     skip_bits(gb, 9);
}

static int hvcc_array_add_nal_unit(uint8_t *nal_buf, uint32_t nal_size,
                                   uint8_t nal_type, int ps_array_completeness,
                                   HEVCDecoderConfigurationRecord *hvcc)
{
    int ret;
    uint8_t index;
    uint16_t numNalus;
    HVCCNALUnitArray *array;

    for (index = 0; index < hvcc->numOfArrays; index++)
        if (hvcc->array[index].NAL_unit_type == nal_type)
            break;

    if (index >= hvcc->numOfArrays) {
        uint8_t i;
        ret = av_reallocp_array(&hvcc->array, index + 1, sizeof(HVCCNALUnitArray));
        if (ret < 0)
            return ret;

        for (i = hvcc->numOfArrays; i <= index; i++)
            memset(&hvcc->array[i], 0, sizeof(HVCCNALUnitArray));
        hvcc->numOfArrays = index + 1;
	}

    array = &hvcc->array[index];
    numNalus = array->numNalus;

    ret = av_reallocp_array(&array->nalUnit, numNalus + 1, sizeof(uint8_t*));
    if (ret < 0)
        return ret;

    ret = av_reallocp_array(&array->nalUnitLength, numNalus + 1, sizeof(uint16_t));
    if (ret < 0)
        return ret;

    array->nalUnit[numNalus] = nal_buf;
    array->nalUnitLength[numNalus] = nal_size;
    array->NAL_unit_type = nal_type;
    array->numNalus++;

    /*
     * When the sample entry name is ‘hvc1’, the default and mandatory value of
     * array_completeness is 1 for arrays of all types of parameter sets, and 0
     * for all other arrays. When the sample entry name is ‘hev1’, the default
     * value of array_completeness is 0 for all arrays.
     */
    if (nal_type == HEVC_NAL_VPS || nal_type == HEVC_NAL_SPS || nal_type == HEVC_NAL_PPS)
        array->array_completeness = ps_array_completeness;

    return 0;
}

static int hvcc_add_nal_unit(uint8_t *nal_buf, uint32_t nal_size,
                             int ps_array_completeness,
                             HEVCDecoderConfigurationRecord *hvcc)
{
    int ret = 0;
    GetBitContext gbc;
    uint8_t nal_type;
    uint8_t *rbsp_buf;
    uint32_t rbsp_size;

    rbsp_buf = nal_unit_extract_rbsp(nal_buf, nal_size, &rbsp_size);
    if (!rbsp_buf) {
        ret = -1;
        goto end;
	}

    ret = init_get_bits8(&gbc, rbsp_buf, rbsp_size);
    if (ret < 0)
        goto end;

    nal_unit_parse_header(&gbc, &nal_type);

    /*
     * Note: only 'declarative' SEI messages are allowed in
     * hvcC. Perhaps the SEI playload type should be checked
     * and non-declarative SEI messages discarded?
     */
    switch (nal_type) {
    case HEVC_NAL_VPS:
    case HEVC_NAL_SPS:
    case HEVC_NAL_PPS:
    case HEVC_NAL_SEI_PREFIX:
    case HEVC_NAL_SEI_SUFFIX:
        ret = hvcc_array_add_nal_unit(nal_buf, nal_size, nal_type,
                                      ps_array_completeness, hvcc);
        if (ret < 0)
            goto end;
        else if (nal_type == HEVC_NAL_VPS)
            ret = hvcc_parse_vps(&gbc, hvcc);
        else if (nal_type == HEVC_NAL_SPS)
            ret = hvcc_parse_sps(&gbc, hvcc);
        else if (nal_type == HEVC_NAL_PPS)
            ret = hvcc_parse_pps(&gbc, hvcc);
        if (ret < 0)
            goto end;
        break;
    default:
        ret = -1;
        goto end;

	}

end:
    free(rbsp_buf);
    return ret;
}

static void hvcc_init(HEVCDecoderConfigurationRecord *hvcc)
{
    memset(hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
    hvcc->configurationVersion = 1;
    hvcc->lengthSizeMinusOne = 3; // 4 bytes

    /*
     * The following fields have all their valid bits set by default,
     * the ProfileTierLevel parsing code will unset them when needed.
     */
    hvcc->general_profile_compatibility_flags = 0xffffffff;
    hvcc->general_constraint_indicator_flags = 0xffffffffffff;

    /*
     * Initialize this field with an invalid value which can be used to detect
     * whether we didn't see any VUI (in which case it should be reset to zero).
     */
    hvcc->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;
}

static void hvcc_close(HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t i;

    for (i = 0; i < hvcc->numOfArrays; i++) {
        hvcc->array[i].numNalus = 0;
        av_freep(&hvcc->array[i].nalUnit);
        av_freep(&hvcc->array[i].nalUnitLength);
	}
    hvcc->numOfArrays = 0;
    av_freep(&hvcc->array);
}

static int hvcc_write(unsigned char* data, HEVCDecoderConfigurationRecord *hvcc)
{
    uint8_t i;
    uint16_t j, vps_count = 0, sps_count = 0, pps_count = 0;

    /*
     * We only support writing HEVCDecoderConfigurationRecord version 1.
     */
    hvcc->configurationVersion = 1;

    /*
     * If min_spatial_segmentation_idc is invalid, reset to 0 (unspecified).
     */
    if (hvcc->min_spatial_segmentation_idc > MAX_SPATIAL_SEGMENTATION)
        hvcc->min_spatial_segmentation_idc = 0;

    /*
     * parallelismType indicates the type of parallelism that is used to meet
     * the restrictions imposed by min_spatial_segmentation_idc when the value
     * of min_spatial_segmentation_idc is greater than 0.
     */
    if (!hvcc->min_spatial_segmentation_idc)
        hvcc->parallelismType = 0;

    /*
     * It's unclear how to properly compute these fields, so
     * let's always set them to values meaning 'unspecified'.
     */
    hvcc->avgFrameRate = 0;
    hvcc->constantFrameRate = 0;

    /*
     * We need at least one of each: VPS, SPS and PPS.
     */
    for (i = 0; i < hvcc->numOfArrays; i++)
        switch (hvcc->array[i].NAL_unit_type) {
        case HEVC_NAL_VPS:
            vps_count += hvcc->array[i].numNalus;
            break;
        case HEVC_NAL_SPS:
            sps_count += hvcc->array[i].numNalus;
            break;
        case HEVC_NAL_PPS:
            pps_count += hvcc->array[i].numNalus;
            break;
        default:
            break;
	}
    if (!vps_count || vps_count > HEVC_MAX_VPS_COUNT ||
        !sps_count || sps_count > HEVC_MAX_SPS_COUNT ||
        !pps_count || pps_count > HEVC_MAX_PPS_COUNT)
        return -1;

	int index = 0;
    /* unsigned int(8) configurationVersion = 1; */
	data[index++] = hvcc->configurationVersion;

    /*
     * unsigned int(2) general_profile_space;
     * unsigned int(1) general_tier_flag;
     * unsigned int(5) general_profile_idc;
     */
	data[index++] = (hvcc->general_profile_space << 6 |
					hvcc->general_tier_flag << 5 |
					hvcc->general_profile_idc);

    /* unsigned int(32) general_profile_compatibility_flags; */
	data[index++] = (uint8_t)((hvcc->general_profile_compatibility_flags >> 24) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_profile_compatibility_flags >> 16) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_profile_compatibility_flags >> 8) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_profile_compatibility_flags >> 0) & 0xFF);

    /* unsigned int(48) general_constraint_indicator_flags; */
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 40) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 32) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 24) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 16) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 8) & 0xFF);
	data[index++] = (uint8_t)((hvcc->general_constraint_indicator_flags >> 0) & 0xFF);

    /* unsigned int(8) general_level_idc; */
	data[index++] = hvcc->general_level_idc;

    /*
     * bit(4) reserved = ‘1111’b;
     * unsigned int(12) min_spatial_segmentation_idc;
     */
	uint16_t temp_seg_idc = hvcc->min_spatial_segmentation_idc | 0xf000;
	data[index++] = (uint8_t)((temp_seg_idc >> 8) & 0xFF);
	data[index++] = (uint8_t)((temp_seg_idc >> 0) & 0xFF);

    /*
     * bit(6) reserved = ‘111111’b;
     * unsigned int(2) parallelismType;
     */
	data[index++] = hvcc->parallelismType | 0xfc;

    /*
     * bit(6) reserved = ‘111111’b;
     * unsigned int(2) chromaFormat;
     */
	data[index++] = hvcc->chromaFormat | 0xfc;

    /*
     * bit(5) reserved = ‘11111’b;
     * unsigned int(3) bitDepthLumaMinus8;
     */
	data[index++] = hvcc->bitDepthLumaMinus8 | 0xf8;

    /*
     * bit(5) reserved = ‘11111’b;
     * unsigned int(3) bitDepthChromaMinus8;
     */
	data[index++] = hvcc->bitDepthChromaMinus8 | 0xf8;

    /* bit(16) avgFrameRate; */
	data[index++] = (uint8_t)((hvcc->avgFrameRate >> 8) & 0xFF);
	data[index++] = (uint8_t)((hvcc->avgFrameRate >> 0) & 0xFF);

    /*
     * bit(2) constantFrameRate;
     * bit(3) numTemporalLayers;
     * bit(1) temporalIdNested;
     * unsigned int(2) lengthSizeMinusOne;
     */
	data[index++] = hvcc->constantFrameRate << 6 |
		hvcc->numTemporalLayers << 3 |
		hvcc->temporalIdNested << 2 |
		hvcc->lengthSizeMinusOne;

    /* unsigned int(8) numOfArrays; */
	data[index++] = hvcc->numOfArrays;

    for (i = 0; i < hvcc->numOfArrays; i++) {
        /*
         * bit(1) array_completeness;
         * unsigned int(1) reserved = 0;
         * unsigned int(6) NAL_unit_type;
         */
		data[index++] = hvcc->array[i].array_completeness << 7 |
			hvcc->array[i].NAL_unit_type & 0x3f;

        /* unsigned int(16) numNalus; */
		data[index++] = (uint8_t)((hvcc->array[i].numNalus >> 8) & 0xFF);
		data[index++] = (uint8_t)((hvcc->array[i].numNalus >> 0) & 0xFF);

        for (j = 0; j < hvcc->array[i].numNalus; j++) {
            /* unsigned int(16) nalUnitLength; */
			data[index++] = (uint8_t)((hvcc->array[i].nalUnitLength[j] >> 8) & 0xFF);
			data[index++] = (uint8_t)((hvcc->array[i].nalUnitLength[j] >> 0) & 0xFF);

            /* bit(8*nalUnitLength) nalUnit; */
			size_t nal_len = hvcc->array[i].nalUnitLength[j];
			memcpy(&data[index], hvcc->array[i].nalUnit[j], nal_len);
			index += nal_len;
		}
	}
    return index;
}


int get_sequence_header(
	unsigned char *out_data, 
	unsigned char *pps, 
	unsigned short pps_len, 
	unsigned char * sps, 
	unsigned short sps_len, 
	unsigned char * vps, 
	unsigned short vps_len)
{
	HEVCDecoderConfigurationRecord hvcc;
	hvcc_init(&hvcc);

	do {
		if (hvcc_add_nal_unit(pps, pps_len, 0, &hvcc) < 0)
			break;

		if (hvcc_add_nal_unit(sps, sps_len, 0, &hvcc) < 0)
			break;

		if (hvcc_add_nal_unit(vps, vps_len, 0, &hvcc) < 0)
			break;

		int len = hvcc_write(out_data, &hvcc);
		if (len > 0)
		{
			hvcc_close(&hvcc);
			return len;
		}
		
	} while (0);

	hvcc_close(&hvcc);
	return -1;
}
