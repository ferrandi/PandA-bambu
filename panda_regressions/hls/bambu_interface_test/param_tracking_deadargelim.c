__attribute__((noinline)) static int helper_deadargelim(int a, int dead, int c)
{
   return a + c;
}

int param_tracking_deadargelim(int x, int y)
{
   return helper_deadargelim(x, 0, y);
}
