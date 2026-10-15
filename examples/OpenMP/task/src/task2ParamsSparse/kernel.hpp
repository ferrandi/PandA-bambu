/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#ifndef KERNEL_HPP
#define KERNEL_HPP

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

constexpr int null = -INT32_MAX;
constexpr unsigned size = 40;

constexpr int data1_amount = 1;
constexpr int data2_amount = 2;

void fn(int* data1, int* data2);

#endif