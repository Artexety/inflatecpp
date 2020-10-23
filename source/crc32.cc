#include "crc32.h"

inline unsigned int crc32_swap(unsigned int x) {
#if defined(__GNUC__) || defined(__clang__)
   return __builtin_bswap32(x);
#else
   return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24);
#endif
}

unsigned int crc32_4bytes(const void* data, unsigned int length, unsigned int previousCrc32) {
    unsigned int  crc = ~previousCrc32;
    const unsigned int* current = (const unsigned int*)data;

    while (length >= 4)
    {
    unsigned int one = *current++ ^ crc;
    crc = kCrc32Lookup[0][(one >> 24) & 0xFF] ^ kCrc32Lookup[1][(one >> 16) & 0xFF] ^
         kCrc32Lookup[2][(one >> 8) & 0xFF] ^ kCrc32Lookup[3][one & 0xFF];

    length -= 4;
    }

   const unsigned char* currentChar = (const unsigned char*)current;

   while (length-- != 0)
      crc = (crc >> 8) ^ kCrc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

   return ~crc;
}