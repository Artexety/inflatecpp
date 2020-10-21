#include <iostream>
#include <fstream>
#include <stdio.h>

#include "source\decompressor.h"

int main(int argc, const char *argv[])
{
	if (argc < 2) 
	{
		std::cout << "no file specified!" << std::endl;
		return 0;
	}

	Decompressor example_decompressor = Decompressor();
	std::ifstream file = std::ifstream(argv[1], std::ios_base::binary);
	
	if(!file.is_open())
	{
		std::cout << "could not open the specified file!" << std::endl;
		return 0;
	}

	file.seekg(0, std::ios::end);
	unsigned int compressed_data_size = file.tellg();
	file.seekg(0, std::ios::beg);

	unsigned int max_decompressed_data_size = 200000000; // As an example
	unsigned char *compressed_data = new unsigned char[compressed_data_size];
	unsigned char *decompressed_data = new unsigned char[max_decompressed_data_size];

	file.read(reinterpret_cast<char *>(&compressed_data[0]), compressed_data_size);
	file.close();

	unsigned int decompressed_data_size = example_decompressor.Feed(compressed_data,
		compressed_data_size, decompressed_data, max_decompressed_data_size, true);

	if (decompressed_data_size == -1)
	{
		std::cout << "decompression error!" << std::endl;
		delete[] compressed_data;
		delete[] decompressed_data;

		return 0;
	}

	std::cout << "decompressed " << decompressed_data_size << " bytes";

	delete[] compressed_data;
	delete[] decompressed_data;

	return 0;
}