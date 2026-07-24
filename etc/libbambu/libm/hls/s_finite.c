// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <math.h>

int finite(double d)
{
   return isfinite(d);
}
