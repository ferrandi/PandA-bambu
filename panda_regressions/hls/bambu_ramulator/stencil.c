// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0

#define N 20

void ramulator_kernel(int n, int steps, float a[restrict N], float b[restrict N])
{
   int step, i;
   for(step = 0; step < steps; ++step)
   {
      for(i = 1; i < n - 1; ++i)
      {
         b[i] = 0.33333 * (a[i - 1] + a[i] + a[i + 1]);
      }
      for(i = 1; i < n - 1; ++i)
      {
         a[i] = 0.33333 * (b[i - 1] + b[i] + b[i + 1]);
      }
   }
}
