# inflate
"Artexety/inflate" is a single function call (optional), memory-to-memory decompressor for the zlib and deflate bitstream formats:

[RFC 1950: ZLIB specification](https://www.ietf.org/rfc/rfc1950.txt)

[RFC 1951: DEFLATE specification](https://www.ietf.org/rfc/rfc1951.txt)

In the future, I will probably add gzip format support according to: [RFC 1952: GZIP specification](https://www.ietf.org/rfc/rfc1952.txt)

"Artexety/inflate" is less than 900 lines of C++ (excluding aproximatly 150 lines of the adler checksum implementation), and decompresses faster than the original zlib function.

Decompressing large raw texture file (zlib bitstream, 60,132,846 bytes compressed to 25,614,357):

    Decompressor                              Time (microseconds), core i7
    zlib inflate                              134,245 (100%)
    "Artexety/inflate" (with checksum)        118,399 (91%)
    "Artexety/inflate" (without checksum)     110,238 (85%)

# How to use it
First, you need to add all the src files of this repository to your project. Then include "decompressor.h" in your code and you are done. 
