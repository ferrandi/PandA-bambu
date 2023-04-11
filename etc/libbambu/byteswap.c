/**
 * swap primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * October, 3 2014.
 *
 */

#ifndef __llvm__
unsigned int __attribute__((optimize("-O1"))) bswap32(unsigned int x)
{
   return ((x << 24u) | ((x & 0x0000ff00U) << 8) | ((x & 0x00ff0000U) >> 8) | (x >> 24));
}
#endif
