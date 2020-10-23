#include "decompressor.h"

unsigned int CopyStored(BitReader *bit_reader, unsigned char *out, unsigned int out_offset, unsigned int block_size_max)
{
	if (bit_reader->ByteAllign() < 0)
		return -1;

	if ((bit_reader->GetInBlock() + 4) > bit_reader->GetInBlockEnd())
		return -1;

	unsigned short stored_length = ((unsigned short)bit_reader->GetInBlock()[0]) | (((unsigned short)bit_reader->GetInBlock()[0]) << 8);
	bit_reader->ModifyInBlock(2);

	unsigned short neg_stored_length = ((unsigned short)bit_reader->GetInBlock()[0]) | (((unsigned short)bit_reader->GetInBlock()[1]) << 8);
	bit_reader->ModifyInBlock(2);

	if (stored_length != ((~neg_stored_length) & 0xffff))
		return -1;

	if (stored_length > block_size_max)
		return -1;

	std::memcpy(out + out_offset, bit_reader->GetInBlock(), stored_length);
	bit_reader->ModifyInBlock(stored_length);

	return (unsigned int)stored_length;
}

unsigned int DecompressBlock(BitReader *bit_reader, int dynamic_block, unsigned char *out, unsigned int out_offset, unsigned int block_size_max)
{
	HuffmanDecoder literals_decoder;
	HuffmanDecoder offset_decoder;
	unsigned int literals_rev_sym_table[kLiteralSyms * 2];
	unsigned int offset_rev_sym_table[kLiteralSyms * 2];
	int i;

	if (dynamic_block)
	{
		HuffmanDecoder tables_decoder;
		unsigned char code_length[kLiteralSyms + kOffsetSyms];
		unsigned int tables_rev_sym_table[kCodeLenSyms * 2];

		unsigned int literal_syms = bit_reader->GetBits(5);
		if (literal_syms == -1)
			return -1;
		literal_syms += 257;
		if (literal_syms > kLiteralSyms)
			return -1;

		unsigned int offset_syms = bit_reader->GetBits(5);
		if (offset_syms == -1)
			return -1;
		offset_syms += 1;
		if (offset_syms > kOffsetSyms)
			return -1;

		unsigned int code_len_syms = bit_reader->GetBits(4);
		if (code_len_syms == -1)
			return -1;
		code_len_syms += 4;
		if (code_len_syms > kCodeLenSyms)
			return -1;

		if (HuffmanDecoder::ReadRawLengths(kCodeLenBits, code_len_syms, kCodeLenSyms, code_length, bit_reader) < 0)
			return -1;
		if (tables_decoder.PrepareTable(tables_rev_sym_table, kCodeLenSyms, kCodeLenSyms, code_length) < 0)
			return -1;
		if (tables_decoder.FinalizeTable(tables_rev_sym_table) < 0)
			return -1;

		if (tables_decoder.ReadLength(tables_rev_sym_table, literal_syms + offset_syms, kLiteralSyms + kOffsetSyms, code_length, bit_reader) < 0)
			return -1;
		if (literals_decoder.PrepareTable(literals_rev_sym_table, literal_syms, kLiteralSyms, code_length) < 0)
			return -1;
		if (offset_decoder.PrepareTable(offset_rev_sym_table, offset_syms, kOffsetSyms, code_length + literal_syms) < 0)
			return -1;
	}
	else
	{
		unsigned char fixed_literal_code_len[kLiteralSyms];
		unsigned char fixed_offset_code_len[kOffsetSyms];

		for (i = 0; i < 144; i++)
			fixed_literal_code_len[i] = 8;
		for (; i < 256; i++)
			fixed_literal_code_len[i] = 9;
		for (; i < 280; i++)
			fixed_literal_code_len[i] = 7;
		for (; i < kLiteralSyms; i++)
			fixed_literal_code_len[i] = 8;

		for (i = 0; i < kOffsetSyms; i++)
			fixed_offset_code_len[i] = 5;

		if (literals_decoder.PrepareTable(literals_rev_sym_table, kLiteralSyms, kLiteralSyms, fixed_literal_code_len) < 0)
			return -1;
		if (offset_decoder.PrepareTable(offset_rev_sym_table, kOffsetSyms, kOffsetSyms, fixed_offset_code_len) < 0)
			return -1;
	}

	for (i = 0; i < kOffsetSyms; i++) 
	{
		unsigned int n = offset_rev_sym_table[i];
		if (n < kOffsetSyms) 
		{
			offset_rev_sym_table[i] = kOffsetCode[n];
		}
	}

	for (i = 0; i < kLiteralSyms; i++) 
	{
		unsigned int n = literals_rev_sym_table[i];
		if (n >= kMatchLenSymStart && n < kLiteralSyms) 
		{
			literals_rev_sym_table[i] = kMatchLenCode[n - kMatchLenSymStart];
		}
	}

	if (literals_decoder.FinalizeTable(literals_rev_sym_table) < 0)
		return -1;
	if (offset_decoder.FinalizeTable(offset_rev_sym_table) < 0)
		return -1;

	unsigned char *current_out = out + out_offset;
	const unsigned char *out_end = current_out + block_size_max;
	const unsigned char *out_fast_end = out_end - 15;

	while (1) 
	{
		bit_reader->Refill32();

		unsigned int literals_code_word = literals_decoder.ReadValue(literals_rev_sym_table, bit_reader);
		if (literals_code_word < 256) 
		{
			
			if (current_out < out_end)
				*current_out++ = literals_code_word;
			else
				return -1;
		}
		else 
		{
			if (literals_code_word == kEODMarkerSym) 
				break;  
			if (literals_code_word == -1) 
				return -1;

			unsigned int match_length = bit_reader->GetBits((literals_code_word >> 16) & 15);
			if (match_length == -1) 
				return -1;

			match_length += (literals_code_word & 0x7fff);

			unsigned int offset_code_word = offset_decoder.ReadValue(offset_rev_sym_table, bit_reader);
			if (offset_code_word == -1) 
				return -1;

			unsigned int match_offset = bit_reader->GetBits((offset_code_word >> 16) & 15);
			if (match_offset == -1) 
				return -1;

			match_offset += (offset_code_word & 0x7fff);

			const unsigned char *src = current_out - match_offset;
			if (src >= out) 
			{
				if (match_offset >= 16 && (current_out + match_length) <= out_fast_end) 
				{
					const unsigned char *copy_src = src;
					unsigned char *copy_dst = current_out;
					const unsigned char *copy_end_dst = current_out + match_length;

					do 
					{
						std::memcpy(copy_dst, copy_src, 16);
						copy_src += 16;
						copy_dst += 16;
					} 
					while (copy_dst < copy_end_dst);

					current_out += match_length;
				}
				else 
				{
					if ((current_out + match_length) > out_end) 
						return -1;

					while 
						(match_length--) {
						*current_out++ = *src++;
					}
				}
			}
			else
				return -1;
		}
	}

	return (unsigned int)(current_out - (out + out_offset));
}

enum ChecksumType { kNone = 0, kGZIP = 1, kZLIB = 2 };

/**
 * Inflate zlib data
 *
 * @param compressed_data pointer to start of zlib data
 * @param compressed_data_size size of zlib data, in bytes
 * @param out pointer to start of decompression buffer
 * @param out_size_max maximum size of decompression buffer, in bytes
 * @param checksum defines if the decompressor should use a specific checksum
 *
 * @return number of bytes decompressed, or -1 in case of an error
 */
unsigned int Decompressor::Feed(const void *compressed_data, unsigned int compressed_data_size, unsigned char *out, unsigned int out_size_max, bool checksum)
{
	unsigned char *current_compressed_data = (unsigned char *)compressed_data;
	unsigned char *end_compressed_data = current_compressed_data + compressed_data_size;
	unsigned int final_block;
	unsigned int current_out_offset;
	unsigned long check_sum = 0;

	ChecksumType checksum_type = ChecksumType::kNone;
	BitReader bit_reader;

	if ((current_compressed_data + 2) > end_compressed_data) 
		return -1;

	if(current_compressed_data[0] == 0x1f && current_compressed_data[1] == 0x8b)
	{
		current_compressed_data += 2;
      	if ((current_compressed_data + 8) > end_compressed_data || current_compressed_data[0] != 0x08)
			return -1;
      	
		current_compressed_data++;
		
      	unsigned char flags = *current_compressed_data++;
      	current_compressed_data += 6;

      	if (flags & 0x02) 
		{      
         	if ((current_compressed_data + 2) > end_compressed_data) 
			 	return -1;
         	
			current_compressed_data += 2;
      	}

      	if (flags & 0x04) 
		{     
         	if ((current_compressed_data + 2) > end_compressed_data) 
			 	return -1;
			
         	unsigned short extra_field_len = ((unsigned short)current_compressed_data[0]) | (((unsigned short)current_compressed_data[1]) << 8);
         	current_compressed_data += 2;

         	if ((current_compressed_data + extra_field_len) > end_compressed_data) 
			 	return -1;
         	
			current_compressed_data += extra_field_len;
      	}

      	if (flags & 0x08) 
	  	{       
        	do 
		 	{
            	if (current_compressed_data >= end_compressed_data) 
					return -1;
            	
				current_compressed_data++;
         	} 
			while (current_compressed_data[-1]);
      	}

      	if (flags & 0x10) 
		{      
         	do 
			{
            	if (current_compressed_data >= end_compressed_data) 
					return -1;
            	
				current_compressed_data++;
         	} 
			while (current_compressed_data[-1]);
      	}

      	if (flags & 0x20) 
        	return -1;

      	checksum_type = ChecksumType::kGZIP;
	}
	else if ((current_compressed_data[0] & 0x0f) == 0x08) 
	{
		unsigned char CMF = current_compressed_data[0];
		unsigned char FLG = current_compressed_data[1];
		unsigned short check = FLG | (((unsigned short)CMF) << 8);

		if ((CMF >> 4) <= 7 && (check % 31) == 0) 
		{
			current_compressed_data += 2;
			if (FLG & 0x20) 
			{
				if ((current_compressed_data + 4) > end_compressed_data) 
					return -1;
				current_compressed_data += 4;
			}
		}

		checksum_type = ChecksumType::kZLIB;
	}

	if (checksum && checksum_type == ChecksumType::kZLIB)
		check_sum = adler32_z(0, nullptr, 0);

	bit_reader.Init(current_compressed_data, end_compressed_data);
	current_out_offset = 0;

	do 
	{
		unsigned int block_type;
		unsigned int block_result;

		final_block = bit_reader.GetBits(1);
		block_type = bit_reader.GetBits(2);

		switch (block_type) 
		{
		case 0:  
			block_result = CopyStored(&bit_reader, out, current_out_offset, out_size_max - current_out_offset);
			break;

		case 1:  
			block_result = DecompressBlock(&bit_reader, 0, out, current_out_offset, out_size_max - current_out_offset);
			break;

		case 2: 
			block_result = DecompressBlock(&bit_reader, 1, out, current_out_offset, out_size_max - current_out_offset);
			break;

		case 3:  
			return -1;
		}

		if (block_result == -1) 
			return -1;

		if(checksum)
		{
			switch (checksum_type)
			{
			case ChecksumType::kGZIP:
				check_sum = crc32_4bytes(out + current_out_offset, block_result, check_sum);
				break;
			
			case ChecksumType::kZLIB:
				check_sum = adler32_z(check_sum, out + current_out_offset, block_result);
				break;
			}
		}

		current_out_offset += block_result;
	} 
	while (!final_block);

	bit_reader.ByteAllign();
	current_compressed_data = bit_reader.GetInBlock();
	
	if (checksum)
	{	
		unsigned int stored_check_sum;

		switch (checksum_type)
		{
		case ChecksumType::kGZIP:
				if ((current_compressed_data + 4) > end_compressed_data) 
					return -1;
				
      			stored_check_sum = ((unsigned int)current_compressed_data[0]);
      			stored_check_sum |= ((unsigned int)current_compressed_data[1]) << 8;
      			stored_check_sum |= ((unsigned int)current_compressed_data[2]) << 16;
      			stored_check_sum |= ((unsigned int)current_compressed_data[3]) << 24;
      
	  			if (stored_check_sum != check_sum) 
				  	return -1;
      			
				current_compressed_data += 4;
				break;
			
			case ChecksumType::kZLIB:
				if ((current_compressed_data + 4) > end_compressed_data)
					return -1;

				stored_check_sum = ((unsigned int)current_compressed_data[0]) << 24;
				stored_check_sum |= ((unsigned int)current_compressed_data[1]) << 16;
				stored_check_sum |= ((unsigned int)current_compressed_data[2]) << 8;
				stored_check_sum |= ((unsigned int)current_compressed_data[3]);

				if (stored_check_sum != check_sum)
					return -1;

				current_compressed_data += 4;
				break;
		}
	}

	return current_out_offset;
}