#ifndef _DECOMPRESSOR_H
#define _DECOMPRESSOR_H

#include <iostream>
#include <cstring>
#include <stdio.h>

#include "bit_reader.h"
#include "huffman_decoder.h"
#include "adler32.h"
#include "crc32.h"

#define MATCHLEN_PAIR(__base,__dispbits) ((__base) | ((__dispbits) << 16) | 0x8000)
#define OFFSET_PAIR(__base,__dispbits) ((__base) | ((__dispbits) << 16))

/*-- zlib static and dynamic blocks inflater --*/
constexpr auto kCodeLenBits = 3;
constexpr auto kLiteralSyms = 288;
constexpr auto kEODMarkerSym = 256;
constexpr auto kMatchLenSymStart = 257;
constexpr auto kMatchLenSyms = 29;
constexpr auto kOffsetSyms = 32;
constexpr auto kMinMatchSize = 3;

constexpr unsigned int kMatchLenCode[kMatchLenSyms] = {
   MATCHLEN_PAIR(kMinMatchSize + 0, 0), MATCHLEN_PAIR(kMinMatchSize + 1, 0), MATCHLEN_PAIR(kMinMatchSize + 2, 0), MATCHLEN_PAIR(kMinMatchSize + 3, 0), MATCHLEN_PAIR(kMinMatchSize + 4, 0),
   MATCHLEN_PAIR(kMinMatchSize + 5, 0), MATCHLEN_PAIR(kMinMatchSize + 6, 0), MATCHLEN_PAIR(kMinMatchSize + 7, 0), MATCHLEN_PAIR(kMinMatchSize + 8, 1), MATCHLEN_PAIR(kMinMatchSize + 10, 1),
   MATCHLEN_PAIR(kMinMatchSize + 12, 1), MATCHLEN_PAIR(kMinMatchSize + 14, 1), MATCHLEN_PAIR(kMinMatchSize + 16, 2), MATCHLEN_PAIR(kMinMatchSize + 20, 2), MATCHLEN_PAIR(kMinMatchSize + 24, 2),
   MATCHLEN_PAIR(kMinMatchSize + 28, 2), MATCHLEN_PAIR(kMinMatchSize + 32, 3), MATCHLEN_PAIR(kMinMatchSize + 40, 3), MATCHLEN_PAIR(kMinMatchSize + 48, 3), MATCHLEN_PAIR(kMinMatchSize + 56, 3),
   MATCHLEN_PAIR(kMinMatchSize + 64, 4), MATCHLEN_PAIR(kMinMatchSize + 80, 4), MATCHLEN_PAIR(kMinMatchSize + 96, 4), MATCHLEN_PAIR(kMinMatchSize + 112, 4), MATCHLEN_PAIR(kMinMatchSize + 128, 5),
   MATCHLEN_PAIR(kMinMatchSize + 160, 5), MATCHLEN_PAIR(kMinMatchSize + 192, 5), MATCHLEN_PAIR(kMinMatchSize + 224, 5), MATCHLEN_PAIR(kMinMatchSize + 255, 0),
};

constexpr unsigned int kOffsetCode[kOffsetSyms] = {
   OFFSET_PAIR(1, 0), OFFSET_PAIR(2, 0), OFFSET_PAIR(3, 0), OFFSET_PAIR(4, 0), OFFSET_PAIR(5, 1), OFFSET_PAIR(7, 1), OFFSET_PAIR(9, 2), OFFSET_PAIR(13, 2), OFFSET_PAIR(17, 3), OFFSET_PAIR(25, 3),
   OFFSET_PAIR(33, 4), OFFSET_PAIR(49, 4), OFFSET_PAIR(65, 5), OFFSET_PAIR(97, 5), OFFSET_PAIR(129, 6), OFFSET_PAIR(193, 6), OFFSET_PAIR(257, 7), OFFSET_PAIR(385, 7), OFFSET_PAIR(513, 8), OFFSET_PAIR(769, 8),
   OFFSET_PAIR(1025, 9), OFFSET_PAIR(1537, 9), OFFSET_PAIR(2049, 10), OFFSET_PAIR(3073, 10), OFFSET_PAIR(4097, 11), OFFSET_PAIR(6145, 11), OFFSET_PAIR(8193, 12), OFFSET_PAIR(12289, 12), OFFSET_PAIR(16385, 13), OFFSET_PAIR(24577, 13),
};

class Decompressor
{
public:
	Decompressor() {};
	~Decompressor() = default;

	unsigned int Feed(const void *, unsigned int, unsigned char *, unsigned int, bool);
};

#endif /* !_DECOMPRESSOR_H */
