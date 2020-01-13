int memory(int base, int i)
{

   #pragma HLS_pipeline
   
   static char str[] = {'a','b','c','d'};
   int flag = 0;
   flag += str[i] - base;
   i++;
   flag += str[i] - base - i;
   i++;
   flag += str[i] - base - i;
   i++;
   flag += str[i] - base - i;
   i++;
   
   return flag;
}
