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

#define Type float

constexpr Type null = static_cast<Type>(-INT32_MAX);

constexpr unsigned size = 20;

constexpr Type data1_amount = static_cast<Type>(1);
constexpr Type data2_amount = static_cast<Type>(2);
constexpr Type data3_amount = static_cast<Type>(3);
constexpr Type data4_amount = static_cast<Type>(4);

void fn(Type* data1, Type* data2, Type* data3, Type* data4);

#endif