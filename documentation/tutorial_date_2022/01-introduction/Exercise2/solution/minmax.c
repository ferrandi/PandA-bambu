void min_max(int * input, int num_elements, int * max, int * min)
{
   int local_max = input[0];
   int local_min = input[0];
   int i = 0;
   for(i = 0; i < num_elements; i++)
   {
      if(input[i] > local_max)
      {
         local_max = input[i];
      }
      else if(input[i] < local_min)
      {
         local_min = input[i];
      }
   }
   *min = local_min;
   *max = local_max;
}
