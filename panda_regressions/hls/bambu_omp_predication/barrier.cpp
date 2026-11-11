// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#ifndef THREADS
#define THREADS 4
#endif
extern "C" void omp_predication(const int* a, int* result, int enabled)
{
#pragma omp parallel num_threads(THREADS)
  {
#pragma omp for nowait
    for (int i = 0; i < 32; ++i)
      result[i] = a[i] + 1;
    // All threads observe the same dynamic condition.
    if (enabled) {
#pragma omp barrier
#pragma omp for
      for (int i = 0; i < 32; ++i)
        result[32 + i] = result[(i + 16) % 32] + a[i];
    }
  }
}
