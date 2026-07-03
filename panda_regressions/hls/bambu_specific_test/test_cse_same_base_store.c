/**
 * Test: CSE merge of two predicated stores with the same base variable but
 * different addresses (mutually exclusive predicates).
 *
 * Uses a 2-element array: store arr[0] when cond is true, arr[1] otherwise.
 * After bambu predication:
 *   if(cond)  arr[0] = v1;
 *   if(!cond) arr[1] = v2;
 * The new same-base CSE pass merges these into:
 *   tmp_ptr = cond ? &arr[0] : &arr[1];
 *   tmp_val = cond ? v1 : v2;
 *   *tmp_ptr = tmp_val;
 */
#include <stdlib.h>

void test_cse_same_base_store(int arr[2], _Bool cond, int v1, int v2) __attribute__((noinline));

void test_cse_same_base_store(int arr[2], _Bool cond, int v1, int v2)
{
   if(cond)
      arr[0] = v1;
   else
      arr[1] = v2;
}

int main()
{
   int p[2] = {0, 0};
   test_cse_same_base_store(p, 1, 42, 99);
   if(p[0] != 42 || p[1] != 0)
   {
      return 1;
   }
   test_cse_same_base_store(p, 0, 42, 99);
   if(p[1] != 99 || p[0] != 42)
   {
      return 1;
   }
   return 0;
}
