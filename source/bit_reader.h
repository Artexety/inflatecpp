#ifndef _BIT_READER_H
#define _BIT_READER_H

#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)
#define X64BIT_SHIFTER
#endif /* defined(_M_X64) */

#ifdef X64BIT_SHIFTER
typedef unsigned long long shifter_t;
#else
typedef unsigned int shifter_t;
#endif /* X64BIT_SHIFTER */

class BitReader
{
public:
	BitReader();
	~BitReader() = default;

	void Init(unsigned char *, unsigned char *);
	void ConsumeBits(const int);
	void ModifyInBlock(const int);
	void Refill32();

	unsigned int GetBits(const int);
	unsigned int PeekBits();

	int ByteAllign();

	unsigned char *GetInBlock() { return this->in_block_; };
	unsigned char *GetInBlockEnd() { return this->in_block_end_; };
	unsigned char *GetInBlockStart() { return this->in_block_start_; };

private:
	int shifter_bit_count_;
	shifter_t shifter_data_;
	unsigned char *in_block_;
	unsigned char *in_block_end_;
	unsigned char *in_block_start_;
};

#endif /* !_BIT_READER_H */