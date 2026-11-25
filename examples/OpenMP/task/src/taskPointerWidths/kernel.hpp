/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#ifndef TASK_POINTER_WIDTHS_HPP
#define TASK_POINTER_WIDTHS_HPP
#include <cstdint>
#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif
constexpr unsigned size = 8;
void fn(std::uint64_t* input, std::uint64_t* output);
#endif
