__attribute__((noinline)) static int helper_no_dae(int a, int b, int c)
{
   return a + b + c;
}

int param_tracking_no_dae(int x, int y, int z)
{
   return helper_no_dae(x, y, z);
}
