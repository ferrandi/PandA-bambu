//// from Hacker's Delight 2nd Edition
//// by Henry S Warren, Jr.

unsigned char test_bit_reverse8(register unsigned char x)
{
   x = (x & 0x55555555) <<  1 | (x & 0xAAAAAAAA) >>  1;
   x = (x & 0x33333333) <<  2 | (x & 0xCCCCCCCC) >>  2;
   x = (x & 0x0F0F0F0F) <<  4 | (x & 0xF0F0F0F0) >>  4;
   return x;
}

unsigned short test_bit_reverse16(unsigned short x)
{
   x = (x & 0x55555555) <<  1 | (x & 0xAAAAAAAA) >>  1;
   x = (x & 0x33333333) <<  2 | (x & 0xCCCCCCCC) >>  2;
   x = (x & 0x0F0F0F0F) <<  4 | (x & 0xF0F0F0F0) >>  4;
   x = (x & 0x00FF00FF) <<  8 | (x & 0xFF00FF00) >>  8;
   return x;
}

unsigned int test_bit_reverse32(unsigned int x)
{
   x = (x & 0x55555555) <<  1 | (x & 0xAAAAAAAA) >>  1;
   x = (x & 0x33333333) <<  2 | (x & 0xCCCCCCCC) >>  2;
   x = (x & 0x0F0F0F0F) <<  4 | (x & 0xF0F0F0F0) >>  4;
   x = (x & 0x00FF00FF) <<  8 | (x & 0xFF00FF00) >>  8;
   x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
   return x;
}

unsigned long long test_bit_reverse64(unsigned long long x)
{
   x = (x & 0x5555555555555555) <<  1 | (x & 0xAAAAAAAAAAAAAAAA) >>  1;
   x = (x & 0x3333333333333333) <<  2 | (x & 0xCCCCCCCCCCCCCCCC) >>  2;
   x = (x & 0x0F0F0F0F0F0F0F0F) <<  4 | (x & 0xF0F0F0F0F0F0F0F0) >>  4;
   x = (x & 0x00FF00FF00FF00FF) <<  8 | (x & 0xFF00FF00FF00FF00) >>  8;
   x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
   x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
   return x;
}


