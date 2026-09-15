/**
 * strcmp primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * October, 22 2024.
 *
 */
/* Public domain.  */

int strcmp(const char* p1, const char* p2)
{
   while(*p1 && (*p1 == *p2))
   {
      p1++;
      p2++;
   }
   return *(unsigned char*)p1 - *(unsigned char*)p2;
}
