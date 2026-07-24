// Copyright (C) 2016-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//


/* In this file the single precision sine and cosine functions are implemented
   following the HOTBM method published by
   Jeremie Detrey and Florent de Dinechin, "Floating-point Trigonometric Functions for FPGAs" FPL 2007.
*/

#include "kf_sincos.c"

#ifndef TEST_PREFIX
#define TEST_PREFIX
#endif

float TEST_PREFIX(tanf)(float x)
{
   float sinx;
   float cosx;
   float _Complex res = __cexpif(x);
   sinx = __imag__ res;
   cosx = __real__ res;
   return sinx / cosx;
}
