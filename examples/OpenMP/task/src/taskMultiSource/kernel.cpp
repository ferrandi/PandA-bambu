/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "kernel.hpp"
#pragma HLS interface port = data mode = bus
void fn(int* data)
{
   task_worker(data);
}
