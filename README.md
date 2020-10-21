# inflate++
[![GitHub license](https://img.shields.io/github/license/mashape/apistatus.svg?style=flat-square)](http://goldsborough.mit-license.org)

"Artexety/inflate" is a single function call (optional), memory-to-memory decompressor for the zlib and deflate bitstream formats:

[RFC 1950: ZLIB specification](https://www.ietf.org/rfc/rfc1950.txt)

[RFC 1951: DEFLATE specification](https://www.ietf.org/rfc/rfc1951.txt)

In the future, I will probably add gzip format support according to: [RFC 1952: GZIP specification](https://www.ietf.org/rfc/rfc1952.txt)

"Artexety/inflate" is less than 900 lines of C++ (excluding aproximatly 100 lines of the adler checksum implementation), and decompresses faster than the original zlib method.

Decompressing large raw texture file (zlib bitstream, 60,132,846 bytes compressed to 25,614,357):

    Decompressor                              Time (microseconds), core i7
    zlib inflate 1.2.11                       134,245 (100%)
    "Artexety/inflate" (with checksum)        118,399 (91%)
    "Artexety/inflate" (without checksum)     110,238 (85%)

This project is developed by Artexety inspired by Mark Adlers zlib decompression. The adler checksum compution by Mark Adler. All code is licensed under the [permissive free software license (MIT)](https://mit-license.org). All mentions are included in the source code.
## Usage
First, you need to include all source files given in this repository to your project. Then include "decompressor.h" in your code and you are done. 

You can use the "decompressor.h" as follows:

```C++

#include <iostream>
#include <stdio.h>

#include "<dir>\decompressor.h"

int main(int argc, const char *argv[])
{
	Decompressor example_decompressor = Decompressor();

	unsigned int compressed_data_size = ...
	unsigned int max_decompressed_data_size = 200000000; // As an example
	unsigned char *compressed_data = new unsigned char[compressed_data_size];
	unsigned char *decompressed_data = new unsigned char[max_decompressed_data_size];

	// ...
	// assign values to "compressed_data"  
	// ...

	unsigned int decompressed_data_size = example_decompressor.Feed(compressed_data_size,
		compressed_data_size, decompressed_data, max_decompressed_data_size, true);
	
	if(decompressed_data_size == -1)
	{
		std::cout << "decompression error!" << std::endl;
		delete [] compressed_data;
		delete [] decompressed_data;

		return 0;
	}

	std::cout << "decompressed " << decompressed_data_size << " bytes";

	// ...
	// use the decompressed data stored in "decompressed_data"
	// ...

	return 0;
}

```
