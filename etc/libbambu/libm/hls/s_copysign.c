// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include "kt_copysign.h"
#include <softfloat/softfloat_features.h>

double copysign(double x, double y)
{
   return asdouble(__kernel_copysign(asuint64(x), asuint64(y), IEEE64_SPEC_ARGS));
}
