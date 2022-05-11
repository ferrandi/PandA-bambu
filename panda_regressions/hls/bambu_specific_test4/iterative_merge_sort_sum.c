// code adapted from https://www.geeksforgeeks.org/iterative-merge-sort/
// Thursday, August 13, 2020
/* Iterative C program for merge sort */
#include <stdio.h>
#include <stdlib.h>

/* Function to merge the two haves arr[l..m] and arr[m+1..r] of array arr[] */
__attribute__((noinline)) void merge(int arr[], int l, int m, int r);

// Utility function to find minimum of two integers
int min(int x, int y)
{
   return (x < y) ? x : y;
}

/* Iterative mergesort function to sort arr[0...n-1] */
/// the 8 is irrelevant from the compiler perspective but it would be useful to test array interface passed to nested functions
int mergeSort(int arr[8], int n)
{
   int curr_size;  // For current size of subarrays to be merged
                   // curr_size varies from 1 to n/2
   int left_start; // For picking starting index of left subarray
                   // to be merged
   int sum_of_elements = 0;
   int index;

   for(index = 0; index < n; ++index)
      sum_of_elements += arr[index];

   // Merge subarrays in bottom up manner. First merge subarrays of
   // size 1 to create sorted subarrays of size 2, then merge subarrays
   // of size 2 to create sorted subarrays of size 4, and so on.
   for(curr_size = 1; curr_size <= n - 1; curr_size = 2 * curr_size)
   {
      // Pick starting point of different subarrays of current size
      for(left_start = 0; left_start < n - 1; left_start += 2 * curr_size)
      {
         // Find ending point of left subarray. mid+1 is starting
         // point of right
         int mid = min(left_start + curr_size - 1, n - 1);

         int right_end = min(left_start + 2 * curr_size - 1, n - 1);

         // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end]
         merge(arr, left_start, mid, right_end);
      }
   }
   return sum_of_elements;
}

/* Function to merge the two haves arr[l..m] and arr[m+1..r] of array arr[] */
void merge(int arr[], int l, int m, int r)
{
   int i, j, k;
   int n1 = m - l + 1;
   int n2 = r - m;

   /* create temp arrays */
   // int L[n1], R[n2];
   /// this removes dynamic allocation
   int L[8], R[8];

   /* Copy data to temp arrays L[] and R[] */
   for(i = 0; i < n1; i++)
      L[i] = arr[l + i];
   for(j = 0; j < n2; j++)
      R[j] = arr[m + 1 + j];

   /* Merge the temp arrays back into arr[l..r]*/
   i = 0;
   j = 0;
   k = l;
   while(i < n1 && j < n2)
   {
      if(L[i] <= R[j])
      {
         arr[k] = L[i];
         i++;
      }
      else
      {
         arr[k] = R[j];
         j++;
      }
      k++;
   }

   /* Copy the remaining elements of L[], if there are any */
   while(i < n1)
   {
      arr[k] = L[i];
      i++;
      k++;
   }

   /* Copy the remaining elements of R[], if there are any */
   while(j < n2)
   {
      arr[k] = R[j];
      j++;
      k++;
   }
}
