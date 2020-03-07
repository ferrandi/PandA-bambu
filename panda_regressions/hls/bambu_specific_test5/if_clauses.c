int if_clauses(int a, int b, int c, int d)
{

   #pragma HLS_simple_pipeline
   
   return (a < 13) ? b + c * d : b * c + d;
}
