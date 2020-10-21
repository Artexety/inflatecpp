#include "huffman_decoder.h"

/**
 * Prepare huffman tables
 *
 * @param rev_symbol_table array of 2 * symbols entries for storing the reverse lookup table
 * @param code_length codeword lengths table
 *
 * @return 0 for success, -1 for failure
 */
int HuffmanDecoder::PrepareTable(unsigned int *rev_symbol_table, const int read_symbols, const int symbols, unsigned char *code_length)
{
	int num_symbols_per_len[16];
	int i;

	if (read_symbols < 0 || read_symbols > kMaxSymbols || symbols < 0 || symbols > kMaxSymbols || read_symbols > symbols)
		return -1;
	this->symbols_ = symbols;


	for (i = 0; i < 16; i++)
		num_symbols_per_len[i] = 0;

	for (i = 0; i < read_symbols; i++)
	{
		if (code_length[i] >= 16) return -1;
		num_symbols_per_len[code_length[i]]++;
	}

	this->starting_pos_[0] = 0;
	this->num_sorted_ = 0;
	for (i = 1; i < 16; i++)
	{
		this->starting_pos_[i] = this->num_sorted_;
		this->num_sorted_ += num_symbols_per_len[i];
	}

	for (i = 0; i < symbols; i++)
		rev_symbol_table[i] = -1;

	for (i = 0; i < read_symbols; i++)
	{
		if (code_length[i])
			rev_symbol_table[this->starting_pos_[code_length[i]]++] = i;
	}

	return 0;
}

/**
 * Finalize huffman codewords for decoding
 *
 * @param rev_symbol_table array of 2 * symbols entries that contains the reverse lookup table
 *
 * @return 0 for success, -1 for failure
 */
int HuffmanDecoder::FinalizeTable(unsigned int *rev_symbol_table)
{
	const int symbols = this->symbols_;
	unsigned int canonical_code_word = 0;
	unsigned int *rev_code_length_table = rev_symbol_table + symbols;
	int canonical_length = 1;
	int i;

	for (i = 0; i < (1 << kFastSymbolBits); i++)
		this->fast_symbol_[i] = 0;
	for (i = 0; i < 16; i++)
		this->start_index_[i] = 0;

	i = 0;
	while (i < this->num_sorted_)
	{
		if (canonical_length >= 16) return -1;
		this->start_index_[canonical_length] = i - canonical_code_word;

		while (i < this->starting_pos_[canonical_length])
		{

			if (i >= symbols) return -1;
			rev_code_length_table[i] = canonical_length;

			if (canonical_code_word >= (1U << canonical_length)) return -1;

			if (canonical_length <= kFastSymbolBits)
			{
				unsigned int rev_word;

				/* Get upside down codeword (branchless method by Eric Biggers) */
				rev_word = ((canonical_code_word & 0x5555) << 1) | ((canonical_code_word & 0xaaaa) >> 1);
				rev_word = ((rev_word & 0x3333) << 2) | ((rev_word & 0xcccc) >> 2);
				rev_word = ((rev_word & 0x0f0f) << 4) | ((rev_word & 0xf0f0) >> 4);
				rev_word = ((rev_word & 0x00ff) << 8) | ((rev_word & 0xff00) >> 8);
				rev_word = rev_word >> (16 - canonical_length);

				int slots = 1 << (kFastSymbolBits - canonical_length);
				while (slots)
				{
					this->fast_symbol_[rev_word] = (rev_symbol_table[i] & 0xffffff) | (canonical_length << 24);
					rev_word += (1 << canonical_length);
					slots--;
				}
			}

			i++;
			canonical_code_word++;
		}
		canonical_length++;
		canonical_code_word <<= 1;
	}

	while (i < symbols)
	{
		rev_symbol_table[i] = -1;
		rev_code_length_table[i++] = 0;
	}

	return 0;
}

/**
 * Read fixed bit size code lengths
 *
 * @param len_bits number of bits per code length
 * @param read_symbols number of symbols actually read
 * @param symbols number of symbols to build codes for
 * @param code_length output code lengths table
 * @param bit_reader bit reader context
 *
 * @return 0 for success, -1 for failure
 */
int HuffmanDecoder::ReadRawLengths(const int len_bits, const int read_symbols, const int symbols, unsigned char *code_length, BitReader *bit_reader)
{
	const unsigned char code_len_syms[kCodeLenSyms] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	int i;

	if (read_symbols < 0 || read_symbols > kMaxSymbols || symbols < 0 || symbols > kMaxSymbols || read_symbols > symbols)
		return -1;

	i = 0;
	while (i < read_symbols)
	{
		unsigned int length = bit_reader->GetBits(len_bits);
		if (length == -1)
			return -1;

		code_length[code_len_syms[i++]] = length;
	}

	while (i < symbols)
	{
		code_length[code_len_syms[i++]] = 0;
	}

	return 0;
}

/**
 * Read huffman-encoded code lengths
 *
 * @param tables_rev_symbol_table reverse lookup table for code lengths
 * @param read_symbols number of symbols actually read
 * @param symbols number of symbols to build codes for
 * @param code_length output code lengths table
 * @param bit_reader bit reader context
 *
 * @return 0 for success, -1 for failure
 */
int HuffmanDecoder::ReadLength(const unsigned int *tables_rev_symbol_table, const int read_symbols, const int symbols, unsigned char *code_length, BitReader *bit_reader)
{
	int i;
	if (read_symbols < 0 || symbols < 0 || read_symbols > symbols)
		return -1;

	i = 0;
	unsigned int previous_length = 0;

	while (i < read_symbols)
	{
		unsigned int length = this->ReadValue(tables_rev_symbol_table, bit_reader);
		if (length == -1)
			return -1;

		if (length < 16)
		{
			previous_length = length;
			code_length[i++] = previous_length;
		}
		else
		{
			unsigned int run_length = 0;

			if (length == 16)
			{
				int extra_run_length = bit_reader->GetBits(2);
				if (extra_run_length == -1)
					return -1;
				run_length = 3 + extra_run_length;
			}
			else if (length == 17)
			{
				int extra_run_length = bit_reader->GetBits(3);
				if (extra_run_length == -1)
					return -1;
				previous_length = 0;
				run_length = 3 + extra_run_length;
			}
			else if (length == 18)
			{
				int extra_run_length = bit_reader->GetBits(7);
				if (extra_run_length == -1)
					return -1;
				previous_length = 0;
				run_length = 11 + extra_run_length;
			}

			while (run_length && i < read_symbols)
			{
				code_length[i++] = previous_length;
				run_length--;
			}
		}
	}

	while (i < symbols)
		code_length[i++] = 0;
	return 0;
}

/**
 * Decode next symbol
 *
 * @param rev_symbol_table reverse lookup table
 * @param bit_reader bit reader context
 *
 * @return symbol, or -1 for error
 */
unsigned int HuffmanDecoder::ReadValue(const unsigned int *rev_symbol_table, BitReader *bit_reader)
{
	unsigned int stream = bit_reader->PeekBits();
	unsigned int fast_sym_bits = this->fast_symbol_[stream & ((1 << kFastSymbolBits) - 1)];
	if (fast_sym_bits)
	{
		bit_reader->ConsumeBits(fast_sym_bits >> 24);
		return fast_sym_bits & 0xffffff;
	}
	const unsigned int *rev_code_length_table = rev_symbol_table + this->symbols_;
	unsigned int code_word = 0;
	int bits = 1;

	do
	{
		code_word |= (stream & 1);

		unsigned int table_index = this->start_index_[bits] + code_word;
		if (table_index < this->symbols_)
		{
			if (bits == rev_code_length_table[table_index])
			{
				bit_reader->ConsumeBits(bits);
				return rev_symbol_table[table_index];
			}
		}
		code_word <<= 1;
		stream >>= 1;
		bits++;
	} while (bits < 16);

	return -1;
}