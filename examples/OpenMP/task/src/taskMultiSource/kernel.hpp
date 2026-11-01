/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#ifndef TASK_MULTI_SOURCE_HPP
#define TASK_MULTI_SOURCE_HPP
#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif
constexpr unsigned size = 8;
void fn(int* data);
void task_worker(int* data);
#endif
