/**
 * Test: CSE merge of two predicated loads with the same base variable but
 * different addresses (mutually exclusive predicates).
 *
 * Uses a 2-element array: load arr[0] when cond is true, arr[1] otherwise.
 * simple_code_motion hoists both loads into the predecessor BB as predicated
 * accesses:
 *   if( cond) a = arr[0];
 *   if(!cond) b = arr[1];
 * The new same-base CSE pass merges these into:
 *   tmp_ptr = cond ? &arr[0] : &arr[1];
 *   merged  = *tmp_ptr;
 *   a = cond ? merged : 0;
 *   b = cond ? 0 : merged;
 */
#include <stdlib.h>

int test_cse_same_base_load(int arr[2], _Bool cond) __attribute__((noinline));

int test_cse_same_base_load(int arr[2], _Bool cond)
{
   int a = 0, b = 0;
   if(cond)
      a = arr[0];
   if(!cond)
      b = arr[1];
   return a | b;
}

int main()
{
   int p1[2] = {10, 20};
   int p2[2] = {30, 40};
   if(test_cse_same_base_load(p1, 1) != 10)
   {
      return 1;
   }
   if(test_cse_same_base_load(p1, 0) != 20)
   {
      return 1;
   }
   if(test_cse_same_base_load(p2, 1) != 30)
   {
      return 1;
   }
   if(test_cse_same_base_load(p2, 0) != 40)
   {
      return 1;
   }
   return 0;
}
