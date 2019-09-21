/**
 * read primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * June, 4 2015.
 *
 */
#include <stddef.h>
#include <unistd.h>
signed short int __bambu_readc(int fd);
long long int __bambu_read4c(int fd);

ssize_t read(int fd, void* __restrict buf, size_t count)
{
   signed short int res;
   long long int res_ll_int;
   int index = 0;
   while(index + 4 < count)
   {
      char control_word;
      res_ll_int = __bambu_read4c(fd);
      control_word = (char)((res_ll_int >> 32) & 15);
      if(control_word)
      {
         if(control_word & 1)
            return index;
         ((signed char*)buf)[index] = (signed char)res_ll_int;
         ++index;
         if(control_word & 2)
            return index;
         ((signed char*)buf)[index] = (signed char)(res_ll_int >> 8);
         ++index;
         if(control_word & 4)
            return index;
         ((signed char*)buf)[index] = (signed char)(res_ll_int >> 16);
         ++index;
         return index;
      }
      else
      {
         *((int*)(((char*)buf) + index)) = (int)res_ll_int;
         index += 4;
      }
   }
   while(index < count)
   {
      res = __bambu_readc(fd);
      if(res < 0)
         break;
      ((signed char*)buf)[index] = res;
      ++index;
   }
   return index;
}
