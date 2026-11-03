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

#include <cassert>
#include <cfloat>
#include <cstdio>
#include <cstdlib>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef Real
#define Real float
constexpr Real null = FLT_MAX;
#endif

#define EPSILON 1.0E-6

#if defined(MAT_SIZE) && defined(BLOCK_SIZE)
constexpr int mat_size = MAT_SIZE;
constexpr int block_size = BLOCK_SIZE;
#else
constexpr int mat_size = 6;
constexpr int block_size = 10;
#endif

#define block_offset block_size* block_size

#define FALSE 0
#define TRUE 1

void original_init(Real*** pBENCH, char const* pass);
void init(Real* mat, char const* name);
void print_structure(char const* name, Real* M[]);
void print_structure(char const* name, Real* M);
int check(Real* T, Real* M);
void fn(Real* mat);
void fn_vanilla(Real* mat);
void original_fn(Real** N);

#endif
