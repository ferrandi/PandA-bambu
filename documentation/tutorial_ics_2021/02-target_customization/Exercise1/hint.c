void max(int input[10], int * max)
{
   int local_max = input[0];
   int i = 0;
   for(i = 0; i < 10; i++)
   {
      if(input[i] > local_max)
      {
         local_max = input[i];
      }
   }
   *max = local_max;
}
