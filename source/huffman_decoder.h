#ifndef _HUFFMAN_DECODER_H
#define _HUFFMAN_DECODER_H

#include "bit_reader.h"

constexpr auto kMaxSymbols = 288;
constexpr auto kCodeLenSyms = 19;
constexpr auto kFastSymbolBits = 10;

class HuffmanDecoder
{
public:
	HuffmanDecoder() {};
	~HuffmanDecoder() = default;

	int PrepareTable(unsigned int *, const int, const int, unsigned char *);
	int FinalizeTable(unsigned int *);
	static int ReadRawLengths(const int, const int, const int, unsigned char *, BitReader *);
	int ReadLength(const unsigned int *, const int, const int, unsigned char *, BitReader *);

	unsigned int ReadValue(const unsigned int *, BitReader *);

private:
	unsigned int fast_symbol_[1 << kFastSymbolBits];
	unsigned int start_index_[16];
	unsigned int symbols_;
	int num_sorted_;
	int starting_pos_[16];
};

#endif /* !_HUFFMAN_DECODER_H */
