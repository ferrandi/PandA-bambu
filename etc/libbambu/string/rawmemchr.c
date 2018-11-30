/**
 * rawmemchr primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */

void* rawmemchr(const void* s, int c)
{
   const unsigned char* r = s;
   while(*r != ((unsigned char)c))
      ++r;
   return (void*)r;
}
