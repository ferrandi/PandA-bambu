/*

Copyright 2007-2011 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France

and by

Centre de recherche INRIA Sophia-Antipolis Mediterranee, equipe APICS,
Sophia Antipolis, France.

Contributors Ch. Lauter, S. Chevillard

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinity norm, an automatic polynomial
implementer and a fast Remez algorithm.

This software is governed by the CeCILL-C license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL-C
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C license and that you accept its terms.

This program is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#ifndef MPFI_COMPAT_H
#define MPFI_COMPAT_H

#include <gmp.h>
#include <mpfr.h>
#include <mpfi.h>

typedef mpfi_t sollya_mpfi_t;

int sollya_mpfi_has_nan(sollya_mpfi_t op); 
int sollya_mpfi_nan_p(sollya_mpfi_t op); 
int sollya_mpfi_set_nan(sollya_mpfi_t rop); 
void sollya_mpfi_nan_normalize(sollya_mpfi_t rop); 
int sollya_mpfi_is_empty(sollya_mpfi_t op); 
int sollya_mpfi_set_empty(sollya_mpfi_t rop); 
void sollya_mpfi_empty_normalize(sollya_mpfi_t rop); 
int sollya_mpfi_set_full_range(sollya_mpfi_t rop); 
int sollya_mpfi_set_negative_inf(sollya_mpfi_t rop); 
int sollya_mpfi_set_positive_inf(sollya_mpfi_t rop); 
int sollya_mpfi_has_zero(sollya_mpfi_t op); 
int sollya_mpfi_has_zero_inside(sollya_mpfi_t op); 
int sollya_mpfi_is_zero(sollya_mpfi_t op); 
int mpfr_is_positive_infinity(mpfr_t op); 
int mpfr_is_negative_infinity(mpfr_t op); 
int sollya_mpfi_has_positive_numbers(sollya_mpfi_t op); 
int sollya_mpfi_has_negative_numbers(sollya_mpfi_t op); 
int sollya_mpfi_is_nonneg(sollya_mpfi_t op); 
int sollya_mpfi_is_nonpos(sollya_mpfi_t op); 
int sollya_mpfi_has_positive_infinity(sollya_mpfi_t op); 
int sollya_mpfi_is_positive_infinity(sollya_mpfi_t op); 
int sollya_mpfi_has_negative_infinity(sollya_mpfi_t op); 
int sollya_mpfi_is_negative_infinity(sollya_mpfi_t op); 
int sollya_mpfi_has_infinity(sollya_mpfi_t op); 
int sollya_mpfi_is_infinity(sollya_mpfi_t op); 
int sollya_mpfi_inf_p(sollya_mpfi_t op); 
int sollya_mpfi_set(sollya_mpfi_t rop, sollya_mpfi_t op); 
int mpfi_to_sollya_mpfi(sollya_mpfi_t rop, mpfi_t op); 
int sollya_mpfi_to_mpfi(mpfi_t rop, sollya_mpfi_t op); 
int sollya_mpfi_set_d(sollya_mpfi_t rop, double op); 
int sollya_mpfi_set_fr(sollya_mpfi_t rop, mpfr_t op); 
int sollya_mpfi_set_q(sollya_mpfi_t rop, mpq_t op); 
int sollya_mpfi_set_si(sollya_mpfi_t rop, long op); 
int sollya_mpfi_set_ui(sollya_mpfi_t rop, unsigned long op); 
int sollya_mpfi_interv_d(sollya_mpfi_t rop, double op1, double op2); 
int sollya_mpfi_interv_fr(sollya_mpfi_t rop, mpfr_t op1, mpfr_t op2); 
int sollya_mpfi_interv_si(sollya_mpfi_t rop, long op1, long op2); 
int sollya_mpfi_interv_d_safe(sollya_mpfi_t rop, double op1, double op2); 
int sollya_mpfi_interv_fr_safe(sollya_mpfi_t rop, mpfr_t op1, mpfr_t op2); 
int sollya_mpfi_interv_si_safe(sollya_mpfi_t rop, long op1, long op2); 
int sollya_mpfi_abs(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_acos(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_acosh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_asin(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_asinh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_atan(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_atanh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_cosh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_sinh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_tanh(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_exp(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_expm1(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_sqr(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_sqrt(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_cos(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_sin(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_tan (sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_log(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_log2(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_log10(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_log1p(sollya_mpfi_t rop, sollya_mpfi_t op);
int sollya_mpfi_div(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_mul(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_add(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_add_ui(sollya_mpfi_t rop, sollya_mpfi_t op1, unsigned long op2); 
int sollya_mpfi_sub(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_sub_fr(sollya_mpfi_t rop, sollya_mpfi_t op1, mpfr_t op2); 
int sollya_mpfi_sub_ui(sollya_mpfi_t rop, sollya_mpfi_t op1, unsigned long op2); 
int sollya_mpfi_div_ui(sollya_mpfi_t rop, sollya_mpfi_t op1, unsigned long op2); 
int sollya_mpfi_mul_ui(sollya_mpfi_t rop, sollya_mpfi_t op1, unsigned long op2); 
int sollya_mpfi_ui_div(sollya_mpfi_t rop, unsigned long op1, sollya_mpfi_t op2); 
int sollya_mpfi_inv(sollya_mpfi_t rop, sollya_mpfi_t op); 
int sollya_mpfi_blow(sollya_mpfi_t rop, sollya_mpfi_t op1, double op2); 
int sollya_mpfi_bounded_p(sollya_mpfi_t op); 
void sollya_mpfi_clear(sollya_mpfi_t op); 
int sollya_mpfi_const_pi(sollya_mpfi_t rop); 
int sollya_mpfi_diam_abs(mpfr_t rop, sollya_mpfi_t op); 
int sollya_mpfi_get_left(mpfr_t rop, sollya_mpfi_t op); 
int sollya_mpfi_get_right(mpfr_t rop, sollya_mpfi_t op); 
void sollya_mpfi_get_fr(mpfr_t rop, sollya_mpfi_t op); 
mp_prec_t sollya_mpfi_get_prec(sollya_mpfi_t op); 
const char *sollya_mpfi_get_version(); 
void sollya_mpfi_init2(sollya_mpfi_t rop, mp_prec_t op); 
int sollya_mpfi_intersect(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_is_inside(sollya_mpfi_t op1, sollya_mpfi_t op2); 
int sollya_mpfi_mid(mpfr_t rop, sollya_mpfi_t op); 
int sollya_mpfi_neg(sollya_mpfi_t rop, sollya_mpfi_t op); 
void sollya_mpfi_set_prec(sollya_mpfi_t rop, mp_prec_t op); 
int sollya_mpfi_union(sollya_mpfi_t rop, sollya_mpfi_t op1, sollya_mpfi_t op2);

#endif /* ifdef MPFI_COMPAT_H */

