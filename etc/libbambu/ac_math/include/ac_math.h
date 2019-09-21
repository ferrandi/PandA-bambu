/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Math Library                                       *
 *                                                                        *
 *  Software Version: 2.0                                                 *
 *                                                                        *
 *  Release Date    : Thu Aug  2 11:19:34 PDT 2018                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2.0.10                                              *
 *                                                                        *
 *  Copyright , Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
#ifndef _INCLUDED_AC_MATH_H_
#define _INCLUDED_AC_MATH_H_

#include <ac_math/ac_abs.h>
// ac_abs()

#include <ac_math/ac_arccos_cordic.h>
// ac_arccos_cordic()

#include <ac_math/ac_arcsin_cordic.h>
// ac_arcsin_cordic()

#include <ac_math/ac_atan_pwl.h>
// ac_atan_pwl()

#include <ac_math/ac_atan2_cordic.h>
// ac_atan2_cordic()

#include <ac_math/ac_div.h>
// ac_div()

#include <ac_math/ac_hcordic.h>
// ac_log_cordic()
// ac_log2_cordic()
// ac_exp_cordic()
// ac_exp2_cordic()
// ac_pow_cordic()

#include <ac_math/ac_inverse_sqrt_pwl.h>
// ac_inverse_sqrt_pwl()

#include <ac_math/ac_log_pwl.h>
// ac_log_pwl()
// ac_log2_pwl()

#include <ac_math/ac_normalize.h>
// ac_normalize()

#include <ac_math/ac_pow_pwl.h>
// ac_pow2_pwl()
// ac_exp_pwl()

#ifndef __BAMBU__
#include <ac_math/ac_random.h>
// ac_random()
#endif
#include <ac_math/ac_reciprocal_pwl.h>
// ac_reciprocal_pwl()

#include <ac_math/ac_shift.h>
// ac_shift_left()
// ac_shift_right()

#include <ac_math/ac_sigmoid_pwl.h>
// ac_sigmoid_pwl()

#include <ac_math/ac_sincos_cordic.h>
// ac_sincos_cordic()

#include <ac_math/ac_sincos_lut.h>
// ac_sincos_lut()

#include <ac_math/ac_sqrt.h>
// ac_sqrt()

#include <ac_math/ac_sqrt_pwl.h>
// ac_sqrt_pwl()

#include <ac_math/ac_tan_pwl.h>
// ac_tan_pwl()

#endif
