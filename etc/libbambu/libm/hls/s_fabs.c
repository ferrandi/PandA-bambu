// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/softfloat_features.h>

#include "kt_fabs.h"

double fabs(double x)
{
   return asdouble(__kernel_fabs(asuint64(x), IEEE64_SPEC_ARGS));
}
