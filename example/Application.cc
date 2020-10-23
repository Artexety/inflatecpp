#include <iostream>
#include <fstream>
#include <stdio.h>

#include "decompressor.h"

int main(int argc, const char *argv[])
{
	if (argc < 2) 
	{
		std::cout << "usage: " << argv[0] << " [infile] [outfile]" << std::endl;
		return 0;
	}

	Decompressor example_decompressor = Decompressor();
	std::ifstream input_file = std::ifstream(argv[1], std::ios::binary | std::ios::app | std::ios::in);
	std::ofstream output_file;

	if(!input_file.is_open())
	{
		std::cout << "could not open the specified input file!" << std::endl;
		return 0;
	}

	input_file.seekg(0, std::ios::end);
	unsigned int compressed_data_size = input_file.tellg();
	input_file.seekg(0, std::ios::beg);

	unsigned int max_decompressed_data_size = 200000000; // As an example
	unsigned char *compressed_data = new unsigned char[compressed_data_size];
	unsigned char *decompressed_data = new unsigned char[max_decompressed_data_size];

	input_file.read(reinterpret_cast<char *>(&compressed_data[0]), compressed_data_size);
	input_file.close();

	unsigned int decompressed_data_size = example_decompressor.Feed(compressed_data,
		compressed_data_size, decompressed_data, max_decompressed_data_size, false);

	if (decompressed_data_size == -1)
	{
		std::cout << "decompression error!" << std::endl;
		delete[] compressed_data;
		delete[] decompressed_data;

		return 0;
	}

	std::cout << "decompressed " << decompressed_data_size << " bytes" << std::endl;

	output_file.open(argv[2], std::ios::binary | std::ios::app | std::ios::out);

	if (!output_file.is_open())
	{
		std::cout << "could not open the specified output file!" << std::endl;
		delete[] compressed_data;
		delete[] decompressed_data;
		
		return 0;
	}

	output_file.write(reinterpret_cast<char *>(&decompressed_data[0]), decompressed_data_size);
	output_file.close();

	delete[] compressed_data;
	delete[] decompressed_data;

	return 0;
}