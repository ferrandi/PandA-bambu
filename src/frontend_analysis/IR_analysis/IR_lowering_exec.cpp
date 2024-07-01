/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2024 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "config_HAVE_ASSERTS.hpp"                // for HAVE_ASSERTS
#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp" // for HAVE_FROM_DISCRE...

#include "Discrepancy.hpp"                 // for Discrepancy
#include "IR_lowering.hpp"                 // for IR_lowering, blo...
#include "Parameter.hpp"                   // for Parameter, OPT_d...
#include "application_manager.hpp"         // for application_manager
#include "custom_map.hpp"                  // for map, _Rb_tree_it...
#include "dbgPrintHelper.hpp"              // for INDENT_DBG_MEX
#include "design_flow_step.hpp"            // for ParameterConstRef
#include "exceptions.hpp"                  // for THROW_ASSERT
#include "frontend_flow_step.hpp"          // for application_mana...
#include "function_behavior.hpp"           // for FunctionBehavior
#include "function_frontend_flow_step.hpp" // for FunctionBehaviorRef
#include "math_function.hpp"               // for resize_to_1_8_16...
#include "refcount.hpp"                    // for GetPointer
#include "string_manipulation.hpp"         // for STR
#include "tree_basic_block.hpp"            // for bloc
#include "tree_common.hpp"                 // for addr_expr_K, mem...
#include "tree_helper.hpp"                 // for tree_helper
#include "tree_manager.hpp"                // for tree_manager
#include "tree_manipulation.hpp"           // for tree_nodeRef
#include "tree_node.hpp"                   // for gimple_assign
#include <algorithm>                       // for max, min
#include <cstring>                         // for memset, size_t
#include <iterator>                        // for next
#include <string>                          // for operator+, alloc...
#include <utility>                         // for pair
#include <vector>                          // for vector

/// Compute the inverse of X mod 2**n, i.e., find Y such that X * Y is
/// congruent to 1 (mod 2**N).
static unsigned long long int invert_mod2n(unsigned long long int x, unsigned long long n)
{
   /* Solve x*y == 1 (mod 2^n), where x is odd.  Return y.  */

   /* The algorithm notes that the choice y = x satisfies
      x*y == 1 mod 2^3, since x is assumed odd.
      Each iteration doubles the number of bits of significance in y.  */

   unsigned long long int mask;
   auto y = x;
   auto nbit = 3ull;

   mask = (n == sizeof(long long int) * 8 ? ~static_cast<unsigned long long int>(0) :
                                            (static_cast<unsigned long long int>(1) << n) - 1);

   while(nbit < n)
   {
      y = y * (2 - x * y) & mask; /* Modulo 2^N */
      nbit *= 2;
   }
   return y;
}

#define LOWPART(x) (static_cast<unsigned long long int>(x) & ((1ULL << 32) - 1))
#define HIGHPART(x) (static_cast<unsigned long long int>(x) >> 32)
#define BASE (1LL << 32)

/* Unpack a two-word integer into 4 words.
   LOW and HI are the integer, as two `HOST_WIDE_INT' pieces.
   WORDS points to the array of HOST_WIDE_INTs.  */
static void encode(long long int* words, unsigned long long int low, long long int hi)
{
   words[0] = static_cast<long long int>(LOWPART(low));
   words[1] = static_cast<long long int>(HIGHPART(low));
   words[2] = static_cast<long long int>(LOWPART(hi));
   words[3] = static_cast<long long int>(HIGHPART(hi));
}

/* Pack an array of 4 words into a two-word integer.
   WORDS points to the array of words.
   The integer is stored into *LOW and *HI as two `HOST_WIDE_INT' pieces.  */
static void decode(long long int* words, unsigned long long int* low, long long int* hi)
{
   *low = static_cast<unsigned long long int>(words[0] + words[1] * BASE);
   *hi = words[2] + words[3] * BASE;
}

/* Divide doubleword integer LNUM, HNUM by doubleword integer LDEN, HDEN
   for a quotient (stored in *LQUO, *HQUO) and remainder (in *LREM, *HREM).
   CODE is a tree code for a kind of division, one of
   TRUNC_DIV_EXPR, FLOOR_DIV_EXPR, CEIL_DIV_EXPR, ROUND_DIV_EXPR
   or EXACT_DIV_EXPR
   It controls how the quotient is rounded to an integer.
   Return nonzero if the operation overflows.
   UNS nonzero says do unsigned division.  */
static int div_and_round_double_cprop(
    /* num == numerator == dividend */
    unsigned long long int lnum_orig, long long int hnum_orig,
    /* den == denominator == divisor */
    unsigned long long int lden_orig, unsigned long long int* lquo, long long int* hquo)
{
   long long int num[4 + 1]; /* extra element for scaling.  */
   long long int den[4], quo[4];
   int i, j;
   unsigned long long int work;
   unsigned long long int carry = 0;
   unsigned long long int lnum = lnum_orig;
   long long int hnum = hnum_orig;
   unsigned long long int lden = lden_orig;
   int overflow = 0;

   if(lden == 0)
   {
      overflow = 1, lden = 1;
   }

   if(hnum == 0)
   { /* single precision */
      *hquo = 0;
      /* This unsigned division rounds toward zero.  */
      *lquo = lnum / lden;
      goto finish_up;
   }

   if(hnum == 0)
   { /* trivial case: dividend < divisor */
      *hquo = 0;
      *lquo = 0;

      goto finish_up;
   }

   memset(quo, 0, sizeof quo);

   memset(num, 0, sizeof num); /* to zero 9th element */
   memset(den, 0, sizeof den);

   encode(num, lnum, hnum);
   encode(den, lden, 0);

   /* Special code for when the divisor < BASE.  */
   if(lden < static_cast<unsigned long long int>(BASE))
   {
      /* hnum != 0 already checked.  */
      for(i = 4 - 1; i >= 0; i--)
      {
         work = static_cast<unsigned long long int>(num[i]) + carry * BASE;
         quo[i] = static_cast<long long int>(work / lden);
         carry = work % lden;
      }
   }
   else
   {
      /* Full double precision division,
     with thanks to Don Knuth's "Seminumerical Algorithms".  */
      int num_hi_sig, den_hi_sig;
      unsigned long long int quo_est, scale;

      /* Find the highest nonzero divisor digit.  */
      for(i = 4 - 1;; i--)
      {
         if(den[i] != 0)
         {
            den_hi_sig = i;
            break;
         }
      }

      /* Insure that the first digit of the divisor is at least BASE/2.
     This is required by the quotient digit estimation algorithm.  */

      scale = static_cast<unsigned long long int>(BASE / (den[den_hi_sig] + 1));
      if(scale > 1)
      { /* scale divisor and dividend */
         carry = 0;
         for(i = 0; i <= 4 - 1; i++)
         {
            work = (static_cast<unsigned long long int>(num[i]) * scale) + carry;
            num[i] = LOWPART(work);
            carry = HIGHPART(work);
         }

         num[4] = static_cast<long long int>(carry);
         carry = 0;
         for(i = 0; i <= 4 - 1; i++)
         {
            work = (static_cast<unsigned long long int>(den[i]) * scale) + carry;
            den[i] = LOWPART(work);
            carry = HIGHPART(work);
            if(den[i] != 0)
            {
               den_hi_sig = i;
            }
         }
      }

      num_hi_sig = 4;

      /* Main loop */
      for(i = num_hi_sig - den_hi_sig - 1; i >= 0; i--)
      {
         /* Guess the next quotient digit, quo_est, by dividing the first
         two remaining dividend digits by the high order quotient digit.
         quo_est is never low and is at most 2 high.  */
         unsigned long long int tmp;

         num_hi_sig = i + den_hi_sig + 1;
         work = static_cast<unsigned long long int>(num[num_hi_sig]) * BASE +
                static_cast<unsigned long long int>(num[num_hi_sig - 1]);
         if(num[num_hi_sig] != den[den_hi_sig])
         {
            quo_est = work / static_cast<unsigned long long int>(den[den_hi_sig]);
         }
         else
         {
            quo_est = BASE - 1;
         }

         /* Refine quo_est so it's usually correct, and at most one high.  */
         tmp = work - quo_est * static_cast<unsigned long long int>(den[den_hi_sig]);
         if(tmp < BASE && (static_cast<unsigned long long int>(den[den_hi_sig - 1]) * quo_est >
                           (tmp * BASE + static_cast<unsigned long long int>(num[num_hi_sig - 2]))))
         {
            quo_est--;
         }

         /* Try QUO_EST as the quotient digit, by multiplying the
         divisor by QUO_EST and subtracting from the remaining dividend.
         Keep in mind that QUO_EST is the I - 1st digit.  */

         carry = 0;
         for(j = 0; j <= den_hi_sig; j++)
         {
            work = quo_est * static_cast<unsigned long long int>(den[j]) + carry;
            carry = HIGHPART(work);
            work = static_cast<unsigned long long int>(num[i + j]) - LOWPART(work);
            num[i + j] = LOWPART(work);
            carry += HIGHPART(work) != 0;
         }

         /* If quo_est was high by one, then num[i] went negative and
         we need to correct things.  */
         if(num[num_hi_sig] < static_cast<long long int>(carry))
         {
            quo_est--;
            carry = 0; /* add divisor back in */
            for(j = 0; j <= den_hi_sig; j++)
            {
               work = static_cast<unsigned long long int>(num[i + j]) + static_cast<unsigned long long int>(den[j]) +
                      carry;
               carry = HIGHPART(work);
               num[i + j] = LOWPART(work);
            }

            num[num_hi_sig] = num[num_hi_sig] + static_cast<long long int>(carry);
         }

         /* Store the quotient digit.  */
         quo[i] = static_cast<long long int>(quo_est);
      }
   }

   decode(quo, lquo, hquo);

finish_up:

   return overflow;
}

/* Choose a minimal N + 1 bit approximation to 1/D that can be used to
   replace division by D, and put the least significant N bits of the result
   in *MULTIPLIER_PTR and return the most significant bit.

   The width of operations is N (should be <= HOST_BITS_PER_WIDE_INT), the
   needed precision is in PRECISION (should be <= N).

   PRECISION should be as small as possible so this function can choose
   multiplier more freely.

   The rounded-up logarithm of D is placed in *lgup_ptr.  A shift count that
   is to be used for a final right shift is placed in *POST_SHIFT_PTR.

   Using this function, x/D will be equal to (x * m) >> (*POST_SHIFT_PTR),
   where m is the full HOST_BITS_PER_WIDE_INT + 1 bit multiplier.  */
static unsigned long long int choose_multiplier(unsigned long long int d, int n, int precision,
                                                unsigned long long int* multiplier_ptr, int* post_shift_ptr,
                                                int* lgup_ptr)
{
   long long int mhigh_hi, mlow_hi;
   unsigned long long int mhigh_lo, mlow_lo;
   int lgup, post_shift;
   int pow, pow2;
   unsigned long long int nl;
   long long int nh;

   /* lgup = ceil(log2(divisor)); */
   lgup = static_cast<int>(ceil_log2(d));

   THROW_ASSERT(lgup <= n, "unexpected condition");

   pow = n + lgup;
   pow2 = n + lgup - precision;

   /* We could handle this with some effort, but this case is much
     better handled directly with a scc insn, so rely on caller using
     that.  */
   THROW_ASSERT(pow != 128, "unexpected condition");

   /* mlow = 2^(N + lgup)/d */
   if(pow >= 64)
   {
      nh = 1LL << (pow - 64);
      nl = 0;
   }
   else
   {
      nh = 0;
      nl = 1ULL << pow;
   }

   div_and_round_double_cprop(nl, nh, d, &mlow_lo, &mlow_hi);

   /* mhigh = (2^(N + lgup) + 2^(N + lgup - precision))/d */
   if(pow2 >= 64)
   {
      nh |= 1LL << (pow2 - 64);
   }
   else
   {
      nl |= 1ULL << pow2;
   }

   div_and_round_double_cprop(nl, nh, d, &mhigh_lo, &mhigh_hi);

   THROW_ASSERT(!mhigh_hi || static_cast<unsigned long long int>(nh) - d < d, "unexpected condition");
   THROW_ASSERT(mhigh_hi <= 1 && mlow_hi <= 1, "unexpected condition");
   /* Assert that mlow < mhigh.  */
   THROW_ASSERT(mlow_hi < mhigh_hi || (mlow_hi == mhigh_hi && mlow_lo < mhigh_lo), "unexpected condition");

   /* If precision == N, then mlow, mhigh exceed 2^N
     (but they do not exceed 2^(N+1)).  */

   /* Reduce to lowest terms.  */
   for(post_shift = lgup; post_shift > 0; post_shift--)
   {
      unsigned long long int ml_lo =
          static_cast<unsigned long long int>((static_cast<unsigned long long int>(mlow_hi)) << (64 - 1)) |
          (mlow_lo >> 1);
      unsigned long long int mh_lo =
          static_cast<unsigned long long int>((static_cast<unsigned long long int>(mhigh_hi)) << (64 - 1)) |
          (mhigh_lo >> 1);
      if(ml_lo >= mh_lo)
      {
         break;
      }

      mlow_hi = 0;
      mlow_lo = ml_lo;
      mhigh_hi = 0;
      mhigh_lo = mh_lo;
   }

   *post_shift_ptr = post_shift;
   *lgup_ptr = lgup;
   if(n < 64)
   {
      unsigned long long int mask = (1ULL << n) - 1;
      *multiplier_ptr = mhigh_lo & mask;
      return mhigh_lo >= mask;
   }
   else
   {
      *multiplier_ptr = mhigh_lo;
      return static_cast<unsigned long long int>(mhigh_hi);
   }
}

void IR_lowering::division_by_a_constant(const std::pair<unsigned int, blocRef>& block,
                                         std::list<tree_nodeRef>::const_iterator& it_los, gimple_assign* ga,
                                         const tree_nodeRef& op1, enum kind code1, bool& restart_analysis,
                                         const std::string& srcp_default, const std::string& step_name)
{
   THROW_ASSERT(tree_helper::IsConstant(op1), "");
   const auto op0 = GetPointerS<const binary_expr>(ga->op1)->op0;
   const auto type_expr = GetPointerS<const binary_expr>(ga->op1)->type;

   const auto unsignedp = tree_helper::IsUnsignedIntegerType(type_expr);
   THROW_ASSERT(std::numeric_limits<long long>::min() <= tree_helper::GetConstValue(op1) &&
                    tree_helper::GetConstValue(op1) <= std::numeric_limits<long long>::max(),
                "");
   const auto ext_op1 = static_cast<long long>(tree_helper::GetConstValue(op1));
   const auto rem_flag = code1 == trunc_mod_expr_K;

   if(!AppM->ApplyNewTransformation())
   {
      return;
   }
   /// very special case op1 == 1
   if(ext_op1 == 1)
   {
      tree_nodeRef new_op1;
      if(rem_flag)
      {
         new_op1 = TM->CreateUniqueIntegerCst(0, type_expr);
      }
      else
      {
         new_op1 = op0;
      }
      ga->op1 = new_op1;
      restart_analysis = true;
   }
   else if(!unsignedp && ext_op1 == -1)
   {
      tree_nodeRef new_op1;
      if(rem_flag)
      {
         new_op1 = TM->CreateUniqueIntegerCst(0, type_expr);
      }
      else
      {
         new_op1 = tree_man->create_unary_operation(type_expr, op0, srcp_default, negate_expr_K);
      }
      ga->op1 = new_op1;
      restart_analysis = true;
   }
   else
   {
      const auto op1_is_pow2 =
          ((EXACT_POWER_OF_2_OR_ZERO_P(ext_op1) || (!unsignedp && EXACT_POWER_OF_2_OR_ZERO_P(-ext_op1))));
      // long long int last_div_const = ! rem_flag ? ext_op1 : 0;

      if(code1 == exact_div_expr_K && op1_is_pow2)
      {
         code1 = trunc_div_expr_K;
      }

      if(ext_op1 != 0)
      {
         switch(code1)
         {
            case trunc_div_expr_K:
            case trunc_mod_expr_K:
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Trunc_div or trunc_mod");
               if(unsignedp)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---unsigned");
                  int pre_shift;
                  auto d = static_cast<unsigned long long int>(ext_op1);
                  if(EXACT_POWER_OF_2_OR_ZERO_P(d))
                  {
                     pre_shift = static_cast<int>(floor_log2(d));
                     tree_nodeRef new_op1;
                     if(rem_flag)
                     {
                        const auto mask =
                            TM->CreateUniqueIntegerCst((static_cast<long long int>(1) << pre_shift) - 1, type_expr);
                        new_op1 = tree_man->create_binary_operation(type_expr, op0, mask, srcp_default, bit_and_expr_K);
                     }
                     else
                     {
                        const auto shift = TM->CreateUniqueIntegerCst(pre_shift, type_expr);
                        new_op1 = tree_man->create_binary_operation(type_expr, op0, shift, srcp_default, rshift_expr_K);
                     }
                     ga->op1 = new_op1;
                     restart_analysis = true;
                  }
                  else
                  {
                     const auto data_bitsize = static_cast<int>(tree_helper::Size(tree_helper::CGetType(type_expr)));
                     if(d < (1ull << (data_bitsize - 1)))
                     {
                        unsigned long long int mh, ml;
                        int post_shift;
                        int dummy;
                        const auto previous_precision = data_bitsize;
                        int precision;
                        if(code1 == trunc_mod_expr_K)
                        {
                           precision =
                               std::min(data_bitsize,
                                        static_cast<int>(std::max(tree_helper::Size(ga->op0), tree_helper::Size(op0)) +
                                                         1ULL + ceil_log2(d)));
                        }
                        else
                        {
                           precision = std::min(data_bitsize,
                                                static_cast<int>(tree_helper::Size(ga->op0) + 1ULL + ceil_log2(d)));
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---correct rounding: precision=" + STR(data_bitsize) +
                                           " vs precision=" + STR(precision));
                        if(previous_precision > precision)
                        {
                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                          "-->Decreased precision keeping correct rounding to value " + STR(precision) +
                                              ". Gained " + STR(previous_precision - precision) + " bits.");
                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                        }
                        /// Find a suitable multiplier and right shift count
                        /// instead of multiplying with D.
                        mh = choose_multiplier(d, data_bitsize, precision, &ml, &post_shift, &dummy);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---ml = " + STR(ml) + " mh =" + STR(mh));
                        /* If the suggested multiplier is more than SIZE bits,
                           we can do better for even divisors, using an
                           initial right shift.  */
                        if(mh != 0 && (d & 1) == 0)
                        {
                           pre_shift = static_cast<int>(floor_log2(d & -d));
                           mh = choose_multiplier(d >> pre_shift, data_bitsize, precision - pre_shift, &ml, &post_shift,
                                                  &dummy);
                           THROW_ASSERT(!mh, "unexpected condition");
                        }
                        else
                        {
                           pre_shift = 0;
                        }
                        tree_nodeRef quotient_expr;
                        if(mh != 0)
                        {
                           THROW_ASSERT(post_shift - 1 < 64, "fail1");

                           tree_nodeRef t1_ga_var = expand_mult_highpart(op0, ml, type_expr, data_bitsize, it_los,
                                                                         block.second, srcp_default);
                           THROW_ASSERT(t1_ga_var, "fail1");

                           const auto t2_expr =
                               tree_man->create_binary_operation(type_expr, op0, t1_ga_var, srcp_default, minus_expr_K);
                           const auto t2_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                           t2_expr, function_id, srcp_default);
                           block.second->PushBefore(t2_ga, *it_los, AppM);
                           const auto t2_ga_var = GetPointer<gimple_assign>(t2_ga)->op0;

                           const auto const1_node = TM->CreateUniqueIntegerCst(1, type_expr);
                           const auto t3_expr = tree_man->create_binary_operation(type_expr, t2_ga_var, const1_node,
                                                                                  srcp_default, rshift_expr_K);
                           const auto t3_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                           t3_expr, function_id, srcp_default);
                           block.second->PushBefore(t3_ga, *it_los, AppM);
                           const auto t3_ga_var = GetPointer<gimple_assign>(t3_ga)->op0;

                           const auto t4_expr = tree_man->create_binary_operation(type_expr, t1_ga_var, t3_ga_var,
                                                                                  srcp_default, plus_expr_K);
                           const auto t4_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                           t4_expr, function_id, srcp_default);
                           block.second->PushBefore(t4_ga, *it_los, AppM);
                           const auto t4_ga_var = GetPointer<gimple_assign>(t4_ga)->op0;

                           THROW_ASSERT(post_shift > 0, "unexpected condition");

                           if(post_shift > 1)
                           {
                              const auto post_shift_minusone_node =
                                  TM->CreateUniqueIntegerCst(post_shift - 1, type_expr);
                              quotient_expr = tree_man->create_binary_operation(
                                  type_expr, t4_ga_var, post_shift_minusone_node, srcp_default, rshift_expr_K);
                           }
                           else
                           {
                              quotient_expr = t4_ga_var;
                           }
                        }
                        else
                        {
                           THROW_ASSERT(pre_shift < data_bitsize && post_shift < data_bitsize, "fail1");
                           tree_nodeRef t1_ga_var;
                           if(pre_shift != 0)
                           {
                              const auto pre_shift_node = TM->CreateUniqueIntegerCst(pre_shift, type_expr);
                              const auto t1_expr = tree_man->create_binary_operation(type_expr, op0, pre_shift_node,
                                                                                     srcp_default, rshift_expr_K);
                              const auto t1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                              t1_expr, function_id, srcp_default);
                              block.second->PushBefore(t1_ga, *it_los, AppM);
                              t1_ga_var = GetPointer<gimple_assign>(t1_ga)->op0;
                           }
                           else
                           {
                              t1_ga_var = op0;
                           }

                           const auto t2_ga_var = expand_mult_highpart(t1_ga_var, ml, type_expr, data_bitsize, it_los,
                                                                       block.second, srcp_default);

                           if(post_shift != 0)
                           {
                              const auto post_shift_node = TM->CreateUniqueIntegerCst(post_shift, type_expr);
                              quotient_expr = tree_man->create_binary_operation(type_expr, t2_ga_var, post_shift_node,
                                                                                srcp_default, rshift_expr_K);
                           }
                           else
                           {
                              quotient_expr = t2_ga_var;
                           }
                        }
                        AppM->RegisterTransformation(step_name, *it_los);
                        if(rem_flag)
                        {
                           const auto quotient_ga = tree_man->CreateGimpleAssign(
                               type_expr, tree_nodeRef(), tree_nodeRef(), quotient_expr, function_id, srcp_default);
                           block.second->PushBefore(quotient_ga, *it_los, AppM);
                           const auto quotient_ga_var = GetPointer<gimple_assign>(quotient_ga)->op0;
                           const auto mul_expr = tree_man->create_binary_operation(type_expr, quotient_ga_var, op1,
                                                                                   srcp_default, mult_expr_K);
                           const auto mul_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                            mul_expr, function_id, srcp_default);
                           block.second->PushBefore(mul_ga, *it_los, AppM);
                           const auto mul_ga_var = GetPointerS<gimple_assign>(mul_ga)->op0;
                           /// restrict if needed the input bit-widths
                           tree_nodeRef sub_expr;
                           if(AppM->ApplyNewTransformation() &&
                              static_cast<unsigned long long>(data_bitsize) > tree_helper::Size(ga->op0))
                           {
                              const auto masklow = (integer_cst_t(1) << tree_helper::Size(ga->op0)) - 1;
                              const auto Constmasklow = TM->CreateUniqueIntegerCst(masklow, type_expr);
                              const auto temp_op0_expr = tree_man->create_binary_operation(
                                  type_expr, op0, Constmasklow, srcp_default, bit_and_expr_K);
                              const auto temp_op0_ga = tree_man->CreateGimpleAssign(
                                  type_expr, tree_nodeRef(), tree_nodeRef(), temp_op0_expr, function_id, srcp_default);
                              block.second->PushBefore(temp_op0_ga, *it_los, AppM);
                              const auto temp_op0_ga_var = GetPointer<gimple_assign>(temp_op0_ga)->op0;
                              const auto temp_op1_expr = tree_man->create_binary_operation(
                                  type_expr, mul_ga_var, Constmasklow, srcp_default, bit_and_expr_K);
                              const auto temp_op1_ga = tree_man->CreateGimpleAssign(
                                  type_expr, tree_nodeRef(), tree_nodeRef(), temp_op1_expr, function_id, srcp_default);
                              block.second->PushBefore(temp_op1_ga, *it_los, AppM);
                              const auto temp_op1_ga_var = GetPointerS<gimple_assign>(temp_op1_ga)->op0;
                              const auto temp_sub_expr = tree_man->create_binary_operation(
                                  type_expr, temp_op0_ga_var, temp_op1_ga_var, srcp_default, minus_expr_K);
                              const auto temp_sub_expr_ga = tree_man->CreateGimpleAssign(
                                  type_expr, tree_nodeRef(), tree_nodeRef(), temp_sub_expr, function_id, srcp_default);
                              AppM->RegisterTransformation(step_name, temp_sub_expr_ga);
                              block.second->PushBefore(temp_sub_expr_ga, *it_los, AppM);
                              const auto temp_sub_expr_ga_var = GetPointerS<gimple_assign>(temp_sub_expr_ga)->op0;
                              sub_expr = tree_man->create_binary_operation(type_expr, temp_sub_expr_ga_var,
                                                                           Constmasklow, srcp_default, bit_and_expr_K);
                           }
                           else
                           {
                              sub_expr = tree_man->create_binary_operation(type_expr, op0, mul_ga_var, srcp_default,
                                                                           minus_expr_K);
                           }
                           ga->op1 = sub_expr;
                        }
                        else
                        {
                           ga->op1 = quotient_expr;
                        }
                        restart_analysis = true;
                     }
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---signed");
                  tree_nodeRef new_op1;
                  auto d = ext_op1;
                  unsigned long long abs_d;

                  /* Since d might be INT_MIN, we have to cast to
                     unsigned long long before negating to avoid
                     undefined signed overflow.  */
                  abs_d = (d >= 0 ? static_cast<unsigned long long int>(d) : static_cast<unsigned long long int>(-d));

                  /* n rem d = n rem -d */
                  if(rem_flag && d < 0)
                  {
                     d = static_cast<long long int>(abs_d);
                  }

                  const auto size = tree_helper::Size(type_expr);
                  if(abs_d == (1ull << (size - 1)))
                  {
                     if(AppM->ApplyNewTransformation())
                     {
                        const auto bt = tree_man->GetBooleanType();
                        const auto quotient_expr =
                            tree_man->create_binary_operation(bt, op0, op1, srcp_default, eq_expr_K);
                        const auto quotient_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                              TM->CreateUniqueIntegerCst(1, bt),
                                                                              quotient_expr, function_id, srcp_default);
                        block.second->PushBefore(quotient_ga, *it_los, AppM);
                        const auto quotient_ga_var = GetPointerS<gimple_assign>(quotient_ga)->op0;
                        const auto quotient_nop_expr =
                            tree_man->create_unary_operation(type_expr, quotient_ga_var, srcp_default, nop_expr_K);
                        if(rem_flag)
                        {
                           const auto quotient_nop = tree_man->CreateGimpleAssign(
                               type_expr, TM->CreateUniqueIntegerCst(0, type_expr),
                               TM->CreateUniqueIntegerCst(1, type_expr), quotient_nop_expr, function_id, srcp_default);
                           block.second->PushBefore(quotient_nop, *it_los, AppM);
                           const auto quotient_nop_var = GetPointerS<gimple_assign>(quotient_nop)->op0;
                           const auto mul_expr = tree_man->create_binary_operation(type_expr, quotient_nop_var, op1,
                                                                                   srcp_default, mult_expr_K);
                           const auto mul_ga = tree_man->CreateGimpleAssign(type_expr, nullptr, nullptr, mul_expr,
                                                                            function_id, srcp_default);
                           block.second->PushBefore(mul_ga, *it_los, AppM);
                           const auto mul_ga_var = GetPointerS<gimple_assign>(mul_ga)->op0;
                           const auto sub_expr = tree_man->create_binary_operation(type_expr, op0, mul_ga_var,
                                                                                   srcp_default, minus_expr_K);
                           ga->op1 = sub_expr;
                        }
                        else
                        {
                           ga->op1 = quotient_nop_expr;
                        }
                        AppM->RegisterTransformation(step_name, *it_los);
                        restart_analysis = true;
                     }
                  }
                  else if(EXACT_POWER_OF_2_OR_ZERO_P(abs_d))
                  {
                     if(AppM->ApplyNewTransformation())
                     {
                        if(rem_flag)
                        {
                           new_op1 = expand_smod_pow2(op0, abs_d, *it_los, block.second, type_expr, srcp_default);
                           ga->op1 = new_op1;
                        }
                        else
                        {
                           new_op1 = expand_sdiv_pow2(op0, abs_d, *it_los, block.second, type_expr, srcp_default);
                           /// We have computed OP0 / abs(OP1).  If OP1 is negative, negate the quotient.
                           if(d < 0)
                           {
                              tree_nodeRef sdiv_pow2_ga = tree_man->CreateGimpleAssign(
                                  type_expr, tree_nodeRef(), tree_nodeRef(), new_op1, function_id, srcp_default);
                              block.second->PushBefore(sdiv_pow2_ga, *it_los, AppM);
                              tree_nodeRef sdiv_pow2_ga_var = GetPointer<gimple_assign>(sdiv_pow2_ga)->op0;
                              new_op1 = tree_man->create_unary_operation(type_expr, sdiv_pow2_ga_var, srcp_default,
                                                                         negate_expr_K);
                           }
                           ga->op1 = new_op1;
                        }
                        AppM->RegisterTransformation(step_name, *it_los);
                        restart_analysis = true;
                     }
                  }
                  else
                  {
                     const auto data_bitsize = static_cast<int>(tree_helper::Size(type_expr));
                     if(AppM->ApplyNewTransformation() && data_bitsize <= 64)
                     {
                        unsigned long long int ml;
                        int post_shift;
                        int lgup;
                        choose_multiplier(abs_d, data_bitsize, data_bitsize - 1, &ml, &post_shift, &lgup);
                        tree_nodeRef quotient_expr;
                        THROW_ASSERT(post_shift < 64 && size - 1 < 64, "unexpected condition");
                        if(ml < (1ULL << (data_bitsize - 1)))
                        {
                           tree_nodeRef t1_ga_var = expand_mult_highpart(op0, ml, type_expr, data_bitsize, it_los,
                                                                         block.second, srcp_default);
                           THROW_ASSERT(t1_ga_var, "fail1");

                           tree_nodeRef t2_ga_var;
                           if(post_shift != 0)
                           {
                              tree_nodeRef post_shift_node =
                                  TM->CreateUniqueIntegerCst(static_cast<long long int>(post_shift), type_expr);

                              tree_nodeRef t2_expr = tree_man->create_binary_operation(
                                  type_expr, t1_ga_var, post_shift_node, srcp_default, rshift_expr_K);
                              tree_nodeRef t2_ga = tree_man->CreateGimpleAssign(
                                  type_expr, tree_nodeRef(), tree_nodeRef(), t2_expr, function_id, srcp_default);
                              block.second->PushBefore(t2_ga, *it_los, AppM);
                              t2_ga_var = GetPointer<gimple_assign>(t2_ga)->op0;
                           }
                           else
                           {
                              t2_ga_var = t1_ga_var;
                           }

                           tree_nodeRef data_bitsize_minusone_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(data_bitsize - 1), type_expr);
                           tree_nodeRef t3_expr = tree_man->create_binary_operation(
                               type_expr, op0, data_bitsize_minusone_node, srcp_default, rshift_expr_K);
                           tree_nodeRef t3_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                             t3_expr, function_id, srcp_default);
                           block.second->PushBefore(t3_ga, *it_los, AppM);
                           tree_nodeRef t3_ga_var = GetPointer<gimple_assign>(t3_ga)->op0;

                           if(d < 0)
                           {
                              quotient_expr = tree_man->create_binary_operation(type_expr, t3_ga_var, t2_ga_var,
                                                                                srcp_default, minus_expr_K);
                           }
                           else
                           {
                              quotient_expr = tree_man->create_binary_operation(type_expr, t2_ga_var, t3_ga_var,
                                                                                srcp_default, minus_expr_K);
                           }
                        }
                        else
                        {
                           ml |= (~0ULL) << (data_bitsize - 1);
                           tree_nodeRef t1_ga_var = expand_mult_highpart(op0, ml, type_expr, data_bitsize, it_los,
                                                                         block.second, srcp_default);
                           THROW_ASSERT(t1_ga_var, "fail1");
                           tree_nodeRef t2_expr =
                               tree_man->create_binary_operation(type_expr, t1_ga_var, op0, srcp_default, plus_expr_K);
                           tree_nodeRef t2_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                             t2_expr, function_id, srcp_default);
                           block.second->PushBefore(t2_ga, *it_los, AppM);
                           tree_nodeRef t2_ga_var = GetPointer<gimple_assign>(t2_ga)->op0;

                           tree_nodeRef post_shift_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(post_shift), type_expr);

                           tree_nodeRef t3_expr = tree_man->create_binary_operation(
                               type_expr, t2_ga_var, post_shift_node, srcp_default, rshift_expr_K);
                           tree_nodeRef t3_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                             t3_expr, function_id, srcp_default);
                           block.second->PushBefore(t3_ga, *it_los, AppM);
                           tree_nodeRef t3_ga_var = GetPointer<gimple_assign>(t3_ga)->op0;

                           tree_nodeRef data_bitsize_minusone_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(data_bitsize - 1), type_expr);
                           tree_nodeRef t4_expr = tree_man->create_binary_operation(
                               type_expr, op0, data_bitsize_minusone_node, srcp_default, rshift_expr_K);
                           tree_nodeRef t4_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                             t4_expr, function_id, srcp_default);
                           block.second->PushBefore(t4_ga, *it_los, AppM);
                           tree_nodeRef t4_ga_var = GetPointer<gimple_assign>(t4_ga)->op0;

                           if(d < 0)
                           {
                              quotient_expr = tree_man->create_binary_operation(type_expr, t4_ga_var, t3_ga_var,
                                                                                srcp_default, minus_expr_K);
                           }
                           else
                           {
                              quotient_expr = tree_man->create_binary_operation(type_expr, t3_ga_var, t4_ga_var,
                                                                                srcp_default, minus_expr_K);
                           }
                        }
                        if(rem_flag)
                        {
                           tree_nodeRef quotient_ga = tree_man->CreateGimpleAssign(
                               type_expr, tree_nodeRef(), tree_nodeRef(), quotient_expr, function_id, srcp_default);
                           block.second->PushBefore(quotient_ga, *it_los, AppM);
                           tree_nodeRef quotient_ga_var = GetPointer<gimple_assign>(quotient_ga)->op0;
                           tree_nodeRef mul_expr = tree_man->create_binary_operation(type_expr, quotient_ga_var, op1,
                                                                                     srcp_default, mult_expr_K);
                           tree_nodeRef mul_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(),
                                                                              mul_expr, function_id, srcp_default);
                           block.second->PushBefore(mul_ga, *it_los, AppM);
                           tree_nodeRef mul_ga_var = GetPointer<gimple_assign>(mul_ga)->op0;
                           tree_nodeRef sub_expr = tree_man->create_binary_operation(type_expr, op0, mul_ga_var,
                                                                                     srcp_default, minus_expr_K);
                           ga->op1 = sub_expr;
                        }
                        else
                        {
                           ga->op1 = quotient_expr;
                        }
                        AppM->RegisterTransformation(step_name, *it_los);
                        restart_analysis = true;
                     }
                  }
               }
               break;
            }
            case exact_div_expr_K:
            {
               int pre_shift;
               long long int d = ext_op1;
               unsigned long long int ml;
               auto size = tree_helper::Size(tree_helper::CGetType(type_expr));

               pre_shift = static_cast<int>(floor_log2(static_cast<unsigned long long int>(d & -d)));
               ml = invert_mod2n(static_cast<unsigned long long int>(d >> pre_shift), size);
               tree_nodeRef pre_shift_node = TM->CreateUniqueIntegerCst(pre_shift, type_expr);
               tree_nodeRef ml_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(ml), type_expr);
               tree_nodeRef t1_expr =
                   tree_man->create_binary_operation(type_expr, op0, pre_shift_node, srcp_default, rshift_expr_K);
               tree_nodeRef t1_ga = tree_man->CreateGimpleAssign(type_expr, tree_nodeRef(), tree_nodeRef(), t1_expr,
                                                                 function_id, srcp_default);
               block.second->PushBefore(t1_ga, *it_los, AppM);
               tree_nodeRef t1_ga_var = GetPointer<gimple_assign>(t1_ga)->op0;
               tree_nodeRef quotient_expr =
                   tree_man->create_binary_operation(type_expr, t1_ga_var, ml_node, srcp_default, mult_expr_K);

               ga->op1 = quotient_expr;
               AppM->RegisterTransformation(step_name, *it_los);
               restart_analysis = true;
               break;
            }
            case assert_expr_K:
            case bit_and_expr_K:
            case bit_ior_expr_K:
            case bit_xor_expr_K:
            case catch_expr_K:
            case ceil_div_expr_K:
            case ceil_mod_expr_K:
            case complex_expr_K:
            case compound_expr_K:
            case eh_filter_expr_K:
            case eq_expr_K:
            case fdesc_expr_K:
            case floor_div_expr_K:
            case floor_mod_expr_K:
            case ge_expr_K:
            case gt_expr_K:
            case goto_subroutine_K:
            case in_expr_K:
            case init_expr_K:
            case le_expr_K:
            case lrotate_expr_K:
            case lshift_expr_K:
            case lt_expr_K:
            case max_expr_K:
            case mem_ref_K:
            case min_expr_K:
            case minus_expr_K:
            case modify_expr_K:
            case mult_expr_K:
            case mult_highpart_expr_K:
            case ne_expr_K:
            case ordered_expr_K:
            case plus_expr_K:
            case pointer_plus_expr_K:
            case postdecrement_expr_K:
            case postincrement_expr_K:
            case predecrement_expr_K:
            case preincrement_expr_K:
            case range_expr_K:
            case rdiv_expr_K:
            case frem_expr_K:
            case round_div_expr_K:
            case round_mod_expr_K:
            case rrotate_expr_K:
            case rshift_expr_K:
            case set_le_expr_K:
            case truth_and_expr_K:
            case truth_andif_expr_K:
            case truth_or_expr_K:
            case truth_orif_expr_K:
            case truth_xor_expr_K:
            case try_catch_expr_K:
            case try_finally_K:
            case uneq_expr_K:
            case ltgt_expr_K:
            case lut_expr_K:
            case unge_expr_K:
            case ungt_expr_K:
            case unle_expr_K:
            case unlt_expr_K:
            case unordered_expr_K:
            case widen_sum_expr_K:
            case widen_mult_expr_K:
            case with_size_expr_K:
            case vec_lshift_expr_K:
            case vec_rshift_expr_K:
            case widen_mult_hi_expr_K:
            case widen_mult_lo_expr_K:
            case vec_pack_trunc_expr_K:
            case vec_pack_sat_expr_K:
            case vec_pack_fix_trunc_expr_K:
            case vec_extracteven_expr_K:
            case vec_extractodd_expr_K:
            case vec_interleavehigh_expr_K:
            case vec_interleavelow_expr_K:
            case binfo_K:
            case block_K:
            case call_expr_K:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case constructor_K:
            case identifier_node_K:
            case ssa_name_K:
            case statement_list_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case tree_list_K:
            case tree_vec_K:
            case error_mark_K:
            case extract_bit_expr_K:
            case sat_plus_expr_K:
            case sat_minus_expr_K:
            case extractvalue_expr_K:
            case extractelement_expr_K:
            case CASE_CPP_NODES:
            case CASE_CST_NODES:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            case CASE_UNARY_EXPRESSION:
            default:
            {
               THROW_ERROR("not yet supported code: " + ga->op1->get_kind_text());
               break;
            }
         }
      }
   }
}

DesignFlowStep_Status IR_lowering::InternalExec()
{
   const auto tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   /// first analyze phis
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining PHI of BB" + STR(block.first));
      for(const auto& phi : block.second->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---phi operation");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---phi index: " + STR(phi->index));
         const auto pn = GetPointerS<gimple_phi>(phi);
         const auto srcp_default = pn->include_name + ":" + STR(pn->line_number) + ":" + STR(pn->column_number);

         bool is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            gimple_phi::DefEdgeList to_be_replaced;
            for(const auto& def_edge : pn->CGetDefEdgesList())
            {
               THROW_ASSERT(tree_helper::IsSameType(pn->res, def_edge.first), "required a conversion");
               const auto def_kind = def_edge.first->get_kind();
               if(def_kind == addr_expr_K || def_kind == view_convert_expr_K || def_kind == nop_expr_K ||
                  def_kind == pointer_plus_expr_K || def_kind == minus_expr_K || def_kind == constructor_K)
               {
                  to_be_replaced.push_back(def_edge);
               }
            }
            for(const auto& def_edge : to_be_replaced)
            {
               const auto ue = GetPointer<unary_expr>(def_edge.first);
               tree_nodeRef op_ga;
               if(ue)
               {
                  const auto ue_expr = tree_man->create_unary_operation(
                      ue->type, ue->op, srcp_default,
                      def_edge.first->get_kind()); /// It is required to de-share some IR nodes
                  op_ga = tree_man->CreateGimpleAssign(ue->type, tree_nodeRef(), tree_nodeRef(), ue_expr, function_id,
                                                       srcp_default);
               }
               else
               {
                  auto op_node = def_edge.first;
                  const auto en_type = GetPointer<expr_node>(op_node) ? GetPointerS<expr_node>(op_node)->type :
                                                                        GetPointer<constructor>(op_node)->type;
                  op_ga = tree_man->CreateGimpleAssign(en_type, tree_nodeRef(), tree_nodeRef(), def_edge.first,
                                                       function_id, srcp_default);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + op_ga->ToString());
               const auto ue_vd = GetPointerS<gimple_assign>(op_ga)->op0;
               const auto pred_block = sl->list_of_bloc.at(def_edge.second);
               if(pred_block->CGetStmtList().empty())
               {
                  pred_block->PushBack(op_ga, AppM);
               }
               else
               {
                  const auto last_statement = pred_block->CGetStmtList().back();
                  const auto last_stmt_kind = last_statement->get_kind();
                  if(last_stmt_kind == gimple_cond_K || last_stmt_kind == gimple_multi_way_if_K ||
                     last_stmt_kind == gimple_return_K || last_stmt_kind == gimple_switch_K ||
                     last_stmt_kind == gimple_goto_K)
                  {
                     pred_block->PushBefore(op_ga, last_statement, AppM);
                  }
                  else
                  {
                     pred_block->PushAfter(op_ga, last_statement, AppM);
                     GetPointerS<ssa_name>(ue_vd)->SetDefStmt(op_ga);
                  }
               }
               pn->ReplaceDefEdge(TM, def_edge, gimple_phi::DefEdge(ue_vd, def_edge.second));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined PHI of BB" + STR(block.first));
   }

   /// for each basic block B in CFG do > Consider all blocks successively
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      const auto& list_of_stmt = block.second->CGetStmtList();
      bool restart_analysis;
      do
      {
         restart_analysis = false;
         auto it_los_end = list_of_stmt.end();
         auto it_los = list_of_stmt.begin();
         TreeNodeSet bitfield_vuses;
         TreeNodeSet bitfield_vdefs;
         while(it_los != it_los_end)
         {
            if(GetPointer<gimple_node>(*it_los) && GetPointerS<gimple_node>(*it_los)->vdef)
            {
               bitfield_vuses.insert(GetPointer<gimple_node>(*it_los)->vdef);
            }
            if(GetPointer<gimple_node>(*it_los) &&
               (GetPointerS<gimple_node>(*it_los)->vdef || !GetPointerS<gimple_node>(*it_los)->vuses.empty()))
            {
               for(const auto& vd : bitfield_vdefs)
               {
                  GetPointer<gimple_node>(*it_los)->AddVuse(vd);
               }
            }

            const auto srcp_default = [&]() -> std::string {
               const auto gn = GetPointer<gimple_node>(*it_los);
               if(gn)
               {
                  return gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
               }
               return "";
            }();

            const auto extract_expr = [&](tree_nodeRef& op, bool set_temp_addr) {
               tree_nodeRef min;
               tree_nodeRef max;
               if(op->get_kind() == nop_expr_K)
               {
                  auto nop = GetPointer<nop_expr>(op);
                  if(nop->op->get_kind() == ssa_name_K)
                  {
                     auto opssa = GetPointerS<ssa_name>(nop->op);
                     if(opssa->min && opssa->max)
                     {
                        auto is_op_signed = tree_helper::IsSignedIntegerType(nop->op);
                        auto is_res_signed = tree_helper::IsSignedIntegerType(nop->type);
                        if(is_op_signed == is_res_signed)
                        {
                           min = opssa->min;
                           max = opssa->max;
                        }
                        else
                        {
                           auto size_op = tree_helper::Size(nop->op);
                           if(is_res_signed)
                           {
                              min = TM->CreateUniqueIntegerCst(-(integer_cst_t(1ll) << (size_op - 1)), nop->type);
                              max = TM->CreateUniqueIntegerCst((integer_cst_t(1ll) << (size_op - 1)) - 1, nop->type);
                           }
                           else
                           {
                              min = TM->CreateUniqueIntegerCst(0, nop->type);
                              max = TM->CreateUniqueIntegerCst((integer_cst_t(1ll) << size_op) - 1, nop->type);
                           }
                        }
                     }
                  }
               }
               const auto en_type =
                   GetPointer<expr_node>(op) ? GetPointerS<expr_node>(op)->type : GetPointer<constructor>(op)->type;
               const auto new_ga = tree_man->CreateGimpleAssign(en_type, min, max, op, function_id, srcp_default);
               const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
               op = ssa_vd;
               if(set_temp_addr)
               {
                  GetPointer<gimple_assign>(new_ga)->temporary_address = true;
               }
               block.second->PushBefore(new_ga, *it_los, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + new_ga->ToString());
               restart_analysis = true;
            };
            const auto extract_unary_expr = [&](tree_nodeRef& op, bool duplicate, bool set_temp_addr) {
               if(duplicate)
               {
                  auto* ue = GetPointer<unary_expr>(op);
                  op = tree_man->create_unary_operation(ue->type, ue->op, srcp_default, ue->get_kind());
               }
               extract_expr(op, set_temp_addr);
            };
            const auto type_cast = [&](tree_nodeRef& op, const tree_nodeConstRef& type) {
               const auto nop =
                   tree_man->CreateNopExpr(op, type, tree_nodeConstRef(), tree_nodeConstRef(), function_id);
               const auto nop_ssa = GetPointerS<const gimple_assign>(nop)->op0;
               op = nop_ssa;
               block.second->PushBefore(nop, *it_los, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + nop->ToString());
               restart_analysis = true;
            };

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + (*it_los)->ToString());
            if((*it_los)->get_kind() == gimple_assign_K)
            {
               auto* ga = GetPointer<gimple_assign>(*it_los);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Left part is " + ga->op0->get_kind_text() + " - Right part is " +
                                  ga->op1->get_kind_text());
               const auto code0 = ga->op0->get_kind();
               const auto code1 = ga->op1->get_kind();

               if(ga->clobber)
               {
                  /// found a a clobber assignment
                  /// do nothing
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Init or clobber assignment");
                  it_los++;
                  continue;
               }

               if(GetPointer<unary_expr>(ga->op1) &&
                  ga->op1->get_kind() != addr_expr_K) /// required by the CLANG/LLVM plugin
               {
                  auto ue = GetPointer<unary_expr>(ga->op1);
                  if(GetPointer<unary_expr>(ue->op))
                  {
                     extract_unary_expr(ue->op, ue->op->get_kind() == addr_expr_K || ue->op->get_kind() == nop_expr_K,
                                        ga->temporary_address || code1 == mem_ref_K);
                  }
                  //    if(GetPointer<binary_expr>(ue->op))
                  //    {
                  //       extract_binary_expr(ue->op, ga->temporary_address || code1 == mem_ref_K);
                  //    }
               }
               if(GetPointer<binary_expr>(ga->op1)) /// required by the CLANG/LLVM plugin
               {
                  auto be = GetPointer<binary_expr>(ga->op1);
                  if(GetPointer<unary_expr>(be->op0))
                  {
                     extract_unary_expr(be->op0,
                                        be->op0->get_kind() == addr_expr_K || be->op0->get_kind() == nop_expr_K,
                                        ga->temporary_address || code1 == mem_ref_K);
                  }
                  if(GetPointer<unary_expr>(be->op1))
                  {
                     extract_unary_expr(be->op1,
                                        be->op0->get_kind() == addr_expr_K || be->op0->get_kind() == nop_expr_K, false);
                  }
                  if(GetPointer<binary_expr>(be->op0) || GetPointer<constructor>(be->op0))
                  {
                     extract_expr(be->op0, ga->temporary_address || code1 == mem_ref_K);
                  }
                  if(GetPointer<binary_expr>(be->op1) || GetPointer<constructor>(be->op1))
                  {
                     extract_expr(be->op1, false);
                  }
                  const auto be_kind = be->get_kind();
                  if(be_kind == bit_and_expr_K || be_kind == bit_ior_expr_K || be_kind == bit_xor_expr_K ||
                     be_kind == plus_expr_K || be_kind == minus_expr_K || be_kind == mult_expr_K ||
                     be_kind == trunc_div_expr_K || be_kind == trunc_mod_expr_K || be_kind == sat_plus_expr_K ||
                     be_kind == sat_minus_expr_K)
                  {
                     if(!tree_helper::IsSameType(be->op0, be->type))
                     {
                        type_cast(be->op0, be->type);
                     }
                     if(!tree_helper::IsSameType(be->op1, be->type))
                     {
                        type_cast(be->op1, be->type);
                     }
                  }
               }
               if(GetPointer<ternary_expr>(ga->op1)) /// required by the CLANG/LLVM plugin
               {
                  auto op1_kind = ga->op1->get_kind();
                  if(op1_kind != component_ref_K && op1_kind != bit_field_ref_K)
                  {
                     auto te = GetPointer<ternary_expr>(ga->op1);
                     if(GetPointer<unary_expr>(te->op0) || GetPointer<binary_expr>(te->op0) ||
                        GetPointer<constructor>(te->op0))
                     {
                        extract_expr(te->op0, false);
                     }
                     if(GetPointer<unary_expr>(te->op1) || GetPointer<binary_expr>(te->op1) ||
                        GetPointer<constructor>(te->op1))
                     {
                        extract_expr(te->op1, false);
                     }
                     if(te->op2 && (GetPointer<unary_expr>(te->op2) || GetPointer<binary_expr>(te->op2) ||
                                    GetPointer<constructor>(te->op2)))
                     {
                        extract_expr(te->op2, false);
                     }
                  }
               }

               if(GetPointer<binary_expr>(ga->op0)) /// required by the CLANG/LLVM plugin
               {
                  auto be = GetPointer<binary_expr>(ga->op0);
                  if(GetPointer<unary_expr>(be->op0))
                  {
                     // BEAWARE: not exactly as previuos revision, check commit history in case of issues
                     extract_unary_expr(be->op0, be->op0->get_kind() == addr_expr_K, code0 == mem_ref_K);
                  }
                  if(GetPointer<binary_expr>(be->op0))
                  {
                     extract_expr(be->op0, code0 == mem_ref_K);
                  }
                  if(GetPointer<unary_expr>(be->op1) || GetPointer<binary_expr>(be->op1))
                  {
                     extract_expr(be->op1, false);
                  }
               }
               const auto manage_realpart = [&](const tree_nodeRef op, tree_nodeRef type) -> tree_nodeRef {
                  auto* rpe = GetPointer<realpart_expr>(op);
                  auto type_index = tree_helper::get_type_index(TM, rpe->op->index);
                  const auto op_type = TM->GetTreeNode(type_index);
                  auto size_complex = tree_helper::Size(op_type);
                  auto align = size_complex / 2;
                  const auto pt = tree_man->GetPointerType(op_type, align);

                  const auto ae_cr = tree_man->create_unary_operation(pt, rpe->op, srcp_default, addr_expr_K);
                  const auto ae_cr_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae_cr,
                                                                     function_id, srcp_default);
                  const auto ae_cr_vd = GetPointer<gimple_assign>(ae_cr_ga)->op0;
                  GetPointer<gimple_assign>(ae_cr_ga)->temporary_address = ga->temporary_address;
                  block.second->PushBefore(ae_cr_ga, *it_los, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ae_cr_ga->ToString());
                  auto ssa_vd = ae_cr_vd;
                  if(type)
                  {
                     const auto ga_nop =
                         tree_man->CreateNopExpr(ae_cr_vd, type, tree_nodeRef(), tree_nodeRef(), function_id);
                     THROW_ASSERT(ga_nop, "unexpected pattern");
                     tree_nodeRef ga_nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                     block.second->PushBefore(ga_nop, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + ga_nop->ToString());
                     ssa_vd = ga_nop_vd;
                  }
                  else
                  {
                     type = pt;
                  }
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, type);
                  return tree_man->create_binary_operation(rpe->type, ssa_vd, offset, srcp_default, mem_ref_K);
               };
               const auto manage_imagpart = [&](const tree_nodeRef op, tree_nodeRef type) -> tree_nodeRef {
                  auto* ipe = GetPointer<imagpart_expr>(op);
                  auto type_index = tree_helper::get_type_index(TM, ipe->op->index);
                  tree_nodeRef op_type = TM->GetTreeNode(type_index);
                  auto size_complex = tree_helper::Size(op_type);
                  auto align = size_complex / 2;
                  tree_nodeRef pt = tree_man->GetPointerType(op_type, align);

                  tree_nodeRef ae_cr = tree_man->create_unary_operation(pt, ipe->op, srcp_default, addr_expr_K);
                  tree_nodeRef ae_cr_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae_cr,
                                                                       function_id, srcp_default);
                  tree_nodeRef ae_cr_vd = GetPointer<gimple_assign>(ae_cr_ga)->op0;
                  GetPointer<gimple_assign>(ae_cr_ga)->temporary_address = ga->temporary_address;
                  block.second->PushBefore(ae_cr_ga, *it_los, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ae_cr_ga->ToString());
                  auto ssa_vd = ae_cr_vd;
                  if(type)
                  {
                     const auto ga_nop =
                         tree_man->CreateNopExpr(ae_cr_vd, type, tree_nodeRef(), tree_nodeRef(), function_id);
                     THROW_ASSERT(ga_nop, "unexpected pattern");
                     tree_nodeRef ga_nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                     block.second->PushBefore(ga_nop, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + ga_nop->ToString());
                     ssa_vd = ga_nop_vd;
                  }
                  else
                  {
                     type = pt;
                  }
                  auto offset_value = tree_helper::Size(tree_helper::CGetType(ipe->op)) / 16;
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(static_cast<long long>(offset_value), type);
                  return tree_man->create_binary_operation(ipe->type, ssa_vd, offset, srcp_default, mem_ref_K);
               };
               if(code1 == array_ref_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand array_ref 1 " + STR(ga->op1->index));
                  auto* AR = GetPointer<array_ref>(ga->op1);
                  ga->op1 = array_ref_lowering(AR, srcp_default, block, it_los, true);
                  restart_analysis = true;
               }
               else if(code1 == ssa_name_K && code0 == ssa_name_K)
               {
                  /// check for a missing cast
                  if(tree_helper::Size(tree_helper::CGetType(ga->op0)) !=
                     tree_helper::Size(tree_helper::CGetType(ga->op1)))
                  {
                     auto ssa0 = GetPointerS<ssa_name>(ga->op0);
                     const auto ga_nop =
                         tree_man->CreateNopExpr(ga->op1, ssa0->type, ssa0->min, ssa0->max, function_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + ga_nop->ToString());
                     const auto nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                     block.second->PushBefore(ga_nop, *it_los, AppM);
                     ga->op1 = nop_vd;
                     restart_analysis = true;
                  }
               }
               else if(code1 == addr_expr_K)
               {
                  auto addr_exprNormalize = [&] {
                     auto* ae = GetPointer<addr_expr>(ga->op1);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---Op of addr expr is " + ae->op->get_kind_text());
                     enum kind ae_code = ae->op->get_kind();
                     if(ae_code == mem_ref_K)
                     {
                        /// normalize op0
                        auto* MR = GetPointer<mem_ref>(ae->op);
                        const auto mem_op0_kind = MR->op0->get_kind();
                        if(mem_op0_kind == addr_expr_K || mem_op0_kind == pointer_plus_expr_K ||
                           mem_op0_kind == view_convert_expr_K)
                        {
                           // BEAWARE: it is ok to use this function even with binary_expr provided that duplicate =
                           // false in those cases
                           extract_unary_expr(MR->op0, mem_op0_kind == addr_expr_K, ga->temporary_address);
                        }

                        tree_nodeRef op1 = MR->op1;
                        const auto op1_val = tree_helper::GetConstValue(op1);
                        if(mem_op0_kind == integer_cst_K && op1_val == 0)
                        {
                           ga->op1 = MR->op0;
                        }
                        else if(ga->temporary_address)
                        {
                           if(op1_val != 0)
                           {
                              const auto offset = TM->CreateUniqueIntegerCst(op1_val, tree_man->GetSizeType());
                              const auto mr = tree_man->create_binary_operation(ae->type, MR->op0, offset, srcp_default,
                                                                                pointer_plus_expr_K);
                              ga->op1 = mr;
                           }
                           else
                           {
                              ga->op1 = MR->op0;
                           }
                        }
                        else
                        {
                           if(op1_val != 0)
                           {
                              const auto offset = TM->CreateUniqueIntegerCst(op1_val, tree_man->GetSizeType());
                              const auto mr = tree_man->create_binary_operation(ae->type, MR->op0, offset, srcp_default,
                                                                                pointer_plus_expr_K);
                              const auto pp_ga = tree_man->CreateGimpleAssign(ae->type, tree_nodeRef(), tree_nodeRef(),
                                                                              mr, function_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---adding statement " + pp_ga->ToString());
                              GetPointer<gimple_assign>(pp_ga)->temporary_address = true;
                              const auto pp_vd = GetPointer<gimple_assign>(pp_ga)->op0;
                              block.second->PushBefore(pp_ga, *it_los, AppM);
                              MR->op0 = pp_vd;
                              MR->op1 = TM->CreateUniqueIntegerCst(0, ae->type);
                           }
                           else
                           {
                              auto type_index = tree_helper::get_type_index(TM, MR->op0->index);
                              if(type_index != ae->type->index)
                              {
                                 const auto ga_nop = tree_man->CreateNopExpr(MR->op0, ae->type, tree_nodeRef(),
                                                                             tree_nodeRef(), function_id);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + ga_nop->ToString());
                                 const auto nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                                 block.second->PushBefore(ga_nop, *it_los, AppM);
                                 ga->op1 = nop_vd;
                              }
                              else
                              {
                                 ga->op1 = MR->op0;
                              }
                           }
                        }
                        restart_analysis = true;
                     }
                     else if(ae_code == array_ref_K)
                     {
                        auto* AR = GetPointer<array_ref>(ae->op);
                        ae->op = array_ref_lowering(AR, srcp_default, block, it_los, ga->temporary_address);
                        restart_analysis = true;
                     }
                     else if(ae_code == target_mem_ref461_K)
                     {
                        auto* tmr = GetPointer<target_mem_ref461>(ae->op);
                        /// expose some part of the target_mem_ref statement
                        expand_target_mem_ref(tmr, *it_los, block.second, srcp_default, ga->temporary_address);
                        const auto pt = tree_man->GetPointerType(tmr->type, GetPointer<type_node>(ae->type)->algn);
                        const auto zero_offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr = tree_man->create_binary_operation(tmr->type, tmr->base, zero_offset,
                                                                          srcp_default, mem_ref_K);
                        ae->op = mr;
                        restart_analysis = true;
                     }
                     else if(ae_code == component_ref_K)
                     {
                        auto* cr = GetPointer<component_ref>(ae->op);
                        auto* field_d = GetPointer<field_decl>(cr->op1);
                        THROW_ASSERT(field_d, "expected an field_decl but got something of different");
                        const auto pt = tree_man->GetPointerType(cr->type, GetPointer<type_node>(ae->type)->algn);
                        const auto ae_cr = tree_man->create_unary_operation(pt, cr->op0, srcp_default, addr_expr_K);
                        const auto ae_cr_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae_cr,
                                                                           function_id, srcp_default);
                        const auto ae_cr_vd = GetPointer<gimple_assign>(ae_cr_ga)->op0;
                        GetPointer<gimple_assign>(ae_cr_ga)->temporary_address = ga->temporary_address;
                        block.second->PushBefore(ae_cr_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + ae_cr_ga->ToString());

                        const auto op1_val = tree_helper::GetConstValue(field_d->bpos);
                        const auto offset = TM->CreateUniqueIntegerCst(op1_val / 8, tree_man->GetSizeType());
                        const auto pp =
                            tree_man->create_binary_operation(pt, ae_cr_vd, offset, srcp_default, pointer_plus_expr_K);
                        const auto pp_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp,
                                                                        function_id, srcp_default);
                        GetPointer<gimple_assign>(pp_ga)->temporary_address = ga->temporary_address;
                        const auto ssa_vd = GetPointer<gimple_assign>(pp_ga)->op0;
                        block.second->PushBefore(pp_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + pp_ga->ToString());
                        const auto zero_offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr =
                            tree_man->create_binary_operation(cr->type, ssa_vd, zero_offset, srcp_default, mem_ref_K);
                        ae->op = mr;
                        restart_analysis = true;
                     }
                     else if(ae_code == bit_field_ref_K)
                     {
                        auto* bfr = GetPointer<bit_field_ref>(ae->op);
                        const auto pt = tree_man->GetPointerType(bfr->type, GetPointer<type_node>(ae->type)->algn);
                        const auto ae_cr = tree_man->create_unary_operation(pt, bfr->op0, srcp_default, addr_expr_K);
                        const auto ae_cr_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae_cr,
                                                                           function_id, srcp_default);
                        const auto ae_cr_vd = GetPointer<gimple_assign>(ae_cr_ga)->op0;
                        GetPointer<gimple_assign>(ae_cr_ga)->temporary_address = ga->temporary_address;
                        block.second->PushBefore(ae_cr_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + ae_cr_ga->ToString());

                        const auto op1_val = tree_helper::GetConstValue(bfr->op2);
                        const auto offset = TM->CreateUniqueIntegerCst(op1_val / 8, tree_man->GetSizeType());
                        const auto pp =
                            tree_man->create_binary_operation(pt, ae_cr_vd, offset, srcp_default, pointer_plus_expr_K);
                        const auto pp_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), pp,
                                                                        function_id, srcp_default);
                        GetPointer<gimple_assign>(pp_ga)->temporary_address = ga->temporary_address;
                        const auto ssa_vd = GetPointer<gimple_assign>(pp_ga)->op0;
                        block.second->PushBefore(pp_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + pp_ga->ToString());
                        const auto zero_offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr =
                            tree_man->create_binary_operation(bfr->type, ssa_vd, zero_offset, srcp_default, mem_ref_K);
                        ae->op = mr;
                        restart_analysis = true;
                     }
                     else if(ae_code == indirect_ref_K)
                     {
                        auto* ir = GetPointerS<indirect_ref>(ae->op);
                        const auto type = ir->type;
                        const auto pt = tree_man->GetPointerType(type, GetPointer<type_node>(ae->type)->algn);
                        const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr =
                            tree_man->create_binary_operation(type, ir->op, offset, srcp_default, mem_ref_K);
                        ae->op = mr;
                        restart_analysis = true;
                     }
                     else if(ae_code == realpart_expr_K)
                     {
                        ae->op = manage_realpart(ae->op, ae->type);
                        restart_analysis = true;
                     }
                     else if(ae_code == imagpart_expr_K)
                     {
                        ae->op = manage_imagpart(ae->op, ae->type);
                        restart_analysis = true;
                     }
                     else if(ae_code == var_decl_K || ae_code == parm_decl_K || ae_code == function_decl_K ||
                             ae_code == string_cst_K)
                     {
                        if(code0 == ssa_name_K && ae_code != function_decl_K && ae_code != parm_decl_K)
                        {
                           auto ssa_var = GetPointerS<const ssa_name>(ga->op0);
                           if(ssa_var->use_set->variables.empty())
                           {
                              ssa_var->use_set->Add(ae->op);
                           }
                        }
                     }
                     else
                     {
                        THROW_ERROR("not supported " + ae->op->get_kind_text());
                     }

                     /// check missing cast
#if 1
                     auto* pt_ae = GetPointerS<pointer_type>(ae->type);
                     auto ptd_index = pt_ae->ptd->index;
                     const auto op_type_node = tree_helper::CGetType(ae->op);
                     const auto op_type_id = op_type_node->index;
                     if(op_type_id != ptd_index)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix missing cast");
                        const auto ae_new_node = tree_man->CreateAddrExpr(ae->op, srcp_default);
                        auto* ae_new = GetPointer<addr_expr>(ae_new_node);
                        const auto a_ga = tree_man->CreateGimpleAssign(ae_new->type, tree_nodeRef(), tree_nodeRef(),
                                                                       ae_new_node, function_id, srcp_default);
                        GetPointer<gimple_assign>(a_ga)->temporary_address = ga->temporary_address;
                        block.second->PushBefore(a_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + a_ga->ToString());
                        const auto nop_ae = tree_man->create_unary_operation(
                            ae->type, GetPointer<gimple_assign>(a_ga)->op0, srcp_default, nop_expr_K);
                        ga->op1 = nop_ae;
                        ga->temporary_address = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed missing cast");
                        restart_analysis = true;
                     }
#endif
                  };
                  addr_exprNormalize();
               }
               else if(code1 == realpart_expr_K)
               {
                  auto* rpe = GetPointer<realpart_expr>(ga->op1);
                  if(rpe->op->get_kind() != ssa_name_K)
                  {
                     ga->op1 = manage_realpart(ga->op1, tree_nodeRef());
                     restart_analysis = true;
                  }
               }
               else if(code1 == imagpart_expr_K)
               {
                  auto* ipe = GetPointer<imagpart_expr>(ga->op1);
                  if(ipe->op->get_kind() != ssa_name_K)
                  {
                     ga->op1 = manage_imagpart(ga->op1, tree_nodeRef());
                     restart_analysis = true;
                  }
               }
               else if(code1 == target_mem_ref461_K)
               {
                  auto target_mem_ref1 = [&] {
                     auto* tmr = GetPointer<target_mem_ref461>(ga->op1);
                     /// split the target_mem_ref in a address computation statement and in a load statement
                     const auto type = tmr->type;

                     const auto pt = tree_man->GetPointerType(type, 8);
                     const auto ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                     const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                      function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;

                     const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                     const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                     const auto mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                     ga->op1 = mr;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     restart_analysis = true;
                  };
                  target_mem_ref1();
               }
               else if(code1 == mem_ref_K)
               {
                  auto mem_ref1 = [&] {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand mem_ref 1 " + STR(ga->op1->index));
                     auto* MR = GetPointer<mem_ref>(ga->op1);
                     const auto mem_op0_kind = MR->op0->get_kind();
                     if(mem_op0_kind == addr_expr_K || mem_op0_kind == pointer_plus_expr_K ||
                        mem_op0_kind == view_convert_expr_K)
                     {
                        // BEAWARE: it is ok to use this function even with binary_expr provided that duplicate = false
                        // in those cases
                        extract_unary_expr(MR->op0, mem_op0_kind == addr_expr_K, true);
                     }

                     tree_nodeRef op1 = MR->op1;
                     const auto op1_val = tree_helper::GetConstValue(op1);
                     if(op1_val != 0)
                     {
                        const auto type = MR->type;

                        /// check if there is a misaligned access
                        auto obj_size = static_cast<long long>(tree_helper::Size(tree_helper::CGetType(ga->op1)));
                        auto bram_size = std::max(8ll, obj_size / 2ll);
                        if(((op1_val * 8) % bram_size) != 0)
                        {
                           function_behavior->set_unaligned_accesses(true);
                        }

                        const auto pt = tree_man->GetPointerType(type, 8);
                        const auto ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                        const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                         function_id, srcp_default);
                        GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                        const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                        const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr =
                            tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                        ga->op1 = mr;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                  };
                  mem_ref1();
               }
               else if(code1 == eq_expr_K or code1 == ne_expr_K or code1 == gt_expr_K or code1 == lt_expr_K or
                       code1 == ge_expr_K or code1 == le_expr_K)
               {
                  auto rel_expr1 = [&] {
                     const auto lhs_type = tree_helper::CGetType(ga->op0);
                     if(code0 == ssa_name_K && !tree_helper::IsBooleanType(lhs_type) &&
                        !(tree_helper::IsVectorType(lhs_type) &&
                          tree_helper::IsBooleanType(tree_helper::CGetElements(lhs_type))))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix lhs to be bool");
                        // fix the left hand side to be a bool
                        const auto new_left_type = [&]() -> tree_nodeRef {
                           if(!tree_helper::IsVectorType(ga->op0))
                           {
                              return tree_man->GetBooleanType();
                           }
                           const auto element_type = tree_helper::CGetElements(lhs_type);
                           const auto element_size = tree_helper::SizeAlloc(element_type);
                           const auto vector_size = tree_helper::SizeAlloc(lhs_type);
                           const auto num_elements = vector_size / element_size;
                           return tree_man->CreateVectorType(tree_man->GetBooleanType(), num_elements);
                        }();
                        GetPointer<binary_expr>(ga->op1)->type = new_left_type;
                        const auto lt_ga = tree_man->CreateGimpleAssign(new_left_type, nullptr, nullptr, ga->op1,
                                                                        function_id, srcp_default);
                        block.second->PushBefore(lt_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + lt_ga->ToString());
                        const auto type_node = tree_helper::CGetType(ga->op0);
                        const auto nop_e = tree_man->create_unary_operation(
                            type_node, GetPointer<const gimple_assign>(lt_ga)->op0, srcp_default, nop_expr_K);
                        ga->op1 = nop_e;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed lhs to be bool");
                        restart_analysis = true;
                     }
                  };
                  rel_expr1();
               }
               else if(code1 == truth_not_expr_K)
               {
                  auto tn_expr1 = [&] {
                     const auto lhs = ga->op1;
                     auto* e = GetPointer<truth_not_expr>(lhs);
                     const auto bt = tree_man->GetBooleanType();
                     e->type = bt;
                     if(!tree_helper::IsBooleanType(e->op))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix first operand to be bool");
                        const auto operand_type = tree_helper::CGetType(e->op);
                        const auto zero_value = TM->CreateUniqueIntegerCst(0, operand_type);
                        const auto not_zero =
                            tree_man->create_binary_operation(bt, e->op, zero_value, srcp_default, ne_expr_K);
                        const auto not_zero_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                              TM->CreateUniqueIntegerCst(1, bt),
                                                                              not_zero, function_id, srcp_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + not_zero_ga->ToString());
                        block.second->PushBefore(not_zero_ga, *it_los, AppM);
                        e->op = GetPointer<gimple_assign>(not_zero_ga)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed first operand to be bool");
                        restart_analysis = true;
                     }
                     if(code0 == ssa_name_K && !tree_helper::IsBooleanType(ga->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix lhs to be bool");
                        // fix the left hand side to be a bool
                        const auto lt_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                        TM->CreateUniqueIntegerCst(1, bt), ga->op1,
                                                                        function_id, srcp_default);
                        block.second->PushBefore(lt_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + lt_ga->ToString());
                        const auto type_node = tree_helper::CGetType(ga->op0);
                        const auto nop_e = tree_man->create_unary_operation(
                            type_node, GetPointer<const gimple_assign>(lt_ga)->op0, srcp_default, nop_expr_K);
                        ga->op1 = nop_e;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed lhs to be bool");
                        restart_analysis = true;
                     }
                  };
                  tn_expr1();
               }
               else if(code1 == truth_and_expr_K or code1 == truth_andif_expr_K or code1 == truth_or_expr_K or
                       code1 == truth_orif_expr_K or code1 == truth_xor_expr_K)
               {
                  auto tn_expr1 = [&] {
                     const auto lhs = ga->op1;
                     auto* e = GetPointer<binary_expr>(lhs);
                     const auto bt = tree_man->GetBooleanType();
                     e->type = bt;
                     if(!tree_helper::IsBooleanType(e->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix first operand to be bool");
                        const auto operand_type = tree_helper::CGetType(e->op0);
                        const auto zero_value = TM->CreateUniqueIntegerCst(0, operand_type);
                        const auto not_zero =
                            tree_man->create_binary_operation(bt, e->op0, zero_value, srcp_default, ne_expr_K);
                        const auto not_zero_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                              TM->CreateUniqueIntegerCst(1, bt),
                                                                              not_zero, function_id, srcp_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + not_zero_ga->ToString());
                        block.second->PushBefore(not_zero_ga, *it_los, AppM);
                        e->op0 = GetPointer<gimple_assign>(not_zero_ga)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed first operand to be bool");
                        restart_analysis = true;
                     }
                     if(!tree_helper::IsBooleanType(e->op1))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix second operand to be bool");
                        const auto operand_type = tree_helper::CGetType(e->op1);
                        const auto zero_value = TM->CreateUniqueIntegerCst(0, operand_type);
                        const auto not_zero =
                            tree_man->create_binary_operation(bt, e->op1, zero_value, srcp_default, ne_expr_K);
                        const auto not_zero_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                              TM->CreateUniqueIntegerCst(1, bt),
                                                                              not_zero, function_id, srcp_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + not_zero_ga->ToString());
                        block.second->PushBefore(not_zero_ga, *it_los, AppM);
                        e->op1 = GetPointer<gimple_assign>(not_zero_ga)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed second operand to be bool");
                        restart_analysis = true;
                     }
                     if(code0 == ssa_name_K && !tree_helper::IsBooleanType(ga->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix lhs to be bool");
                        // fix the left hand side to be a bool
                        const auto lt_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                        TM->CreateUniqueIntegerCst(1, bt), ga->op1,
                                                                        function_id, srcp_default);
                        block.second->PushBefore(lt_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + lt_ga->ToString());
                        const auto type_node = tree_helper::CGetType(ga->op0);
                        const auto nop_e = tree_man->create_unary_operation(
                            type_node, GetPointer<const gimple_assign>(lt_ga)->op0, srcp_default, nop_expr_K);
                        ga->op1 = nop_e;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed lhs to be bool");
                        restart_analysis = true;
                     }
                  };
                  tn_expr1();
               }
               else if(code1 == call_expr_K || code1 == aggr_init_expr_K)
               {
                  auto* ce = GetPointer<call_expr>(ga->op1);
                  for(auto& arg : ce->args)
                  {
                     if(GetPointer<unary_expr>(arg) || GetPointer<binary_expr>(arg))
                     {
                        extract_unary_expr(arg, arg->get_kind() == addr_expr_K, false);
                     }
                  }
               }
               else if(code1 == pointer_plus_expr_K)
               {
                  auto pp_expr1 = [&] {
                     auto* ppe = GetPointer<pointer_plus_expr>(ga->op1);
                     THROW_ASSERT(ppe->op0 && ppe->op1, "expected two parameters");
                     if(GetPointer<addr_expr>(ppe->op0))
                     {
                        extract_expr(ppe->op0, ga->temporary_address);
                     }
                     else if(GetPointer<pointer_plus_expr>(ppe->op0)) /// required by CLANG/LLVM plugin
                     {
                        extract_expr(ppe->op0, ga->temporary_address);
                     }
                     else if(GetPointer<mult_expr>(ppe->op1)) /// required by CLANG/LLVM plugin
                     {
                        extract_expr(ppe->op1, false);
                     }
                     else if(GetPointer<var_decl>(ppe->op0))
                     {
                        auto* vd = GetPointer<var_decl>(ppe->op0);
                        const auto type = vd->type;
                        const auto pt = tree_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
                        const auto ae = tree_man->create_unary_operation(pt, ppe->op0, srcp_default, addr_expr_K);
                        const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                         function_id, srcp_default);
                        GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                        const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                        ppe->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(GetPointer<ssa_name>(ppe->op0) && GetPointer<integer_cst>(ppe->op1))
                     {
                        auto temp_def = GetPointer<const ssa_name>(ppe->op0)->CGetDefStmt();
                        if(temp_def->get_kind() == gimple_assign_K)
                        {
                           const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                           if(prev_ga->op1->get_kind() == pointer_plus_expr_K)
                           {
                              const auto prev_ppe = GetPointer<const pointer_plus_expr>(prev_ga->op1);
                              if(GetPointer<ssa_name>(prev_ppe->op0) && GetPointer<integer_cst>(prev_ppe->op1))
                              {
                                 const auto prev_val = tree_helper::GetConstValue(prev_ppe->op1);
                                 const auto curr_val = tree_helper::GetConstValue(ppe->op1);
                                 ppe->op1 =
                                     TM->CreateUniqueIntegerCst(prev_val + curr_val, tree_helper::CGetType(ppe->op1));
                                 ppe->op0 = prev_ppe->op0;
                                 restart_analysis = true;
                              }
                           }
                        }
                     }
                     else if(GetPointer<ssa_name>(ppe->op0))
                     {
                        auto temp_def = GetPointer<const ssa_name>(ppe->op0)->CGetDefStmt();
                        if(temp_def->get_kind() == gimple_assign_K)
                        {
                           const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                           if(prev_ga->op1->get_kind() == addr_expr_K)
                           {
                              auto* prev_ae = GetPointer<addr_expr>(prev_ga->op1);
                              enum kind prev_ae_code = prev_ae->op->get_kind();
                              if(prev_ae_code == mem_ref_K)
                              {
                                 auto* prev_MR = GetPointer<mem_ref>(prev_ae->op);
                                 const auto prev_op1_val = tree_helper::GetConstValue(prev_MR->op1);
                                 if(prev_op1_val == 0)
                                 {
                                    if(prev_MR->op0->get_kind() == ssa_name_K)
                                    {
                                       ppe->op0 = prev_MR->op0;
                                       restart_analysis = true;
                                    }
                                 }
                              }
                           }
                        }
                     }
                  };
                  pp_expr1();
               }
               else if(code1 == component_ref_K)
               {
                  auto cr_expr1 = [&] {
                     const auto cr = GetPointer<component_ref>(ga->op1);
                     const auto field_d = GetPointer<field_decl>(cr->op1);
                     tree_nodeRef type;
                     if(field_d->is_bitfield())
                     {
                        THROW_ASSERT(tree_helper::GetConstValue(field_d->bpos) >= 0, "");
                        const auto op1_val = static_cast<unsigned long long>(tree_helper::GetConstValue(field_d->bpos));
                        const auto right_shift_val = op1_val % 8;
                        if(cr->type->get_kind() == boolean_type_K)
                        {
                           type = tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(1 + right_shift_val)), true);
                        }
                        else
                        {
                           const auto it = GetPointer<integer_type>(cr->type);
                           THROW_ASSERT(it, "unexpected pattern");
                           type = tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(it->prec + right_shift_val)),
                                                                 it->unsigned_flag);
                        }
                     }
                     else
                     {
                        type = cr->type;
                     }
                     THROW_ASSERT(type, "unexpected condition");
                     const auto pt = tree_man->GetPointerType(type, 8);
                     THROW_ASSERT(pt, "unexpected condition");
                     const auto ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                     const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                      function_id, srcp_default);
                     GetPointerS<gimple_assign>(new_ga)->temporary_address = true;

                     const auto ssa_vd = GetPointerS<gimple_assign>(new_ga)->op0;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());

                     if(field_d->packed_flag)
                     {
                        function_behavior->set_packed_vars(true);
                     }
                     const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                     const auto mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                     if(field_d->is_bitfield())
                     {
                        const auto op1_val = static_cast<long long>(tree_helper::GetConstValue(field_d->bpos));
                        const auto right_shift_val = op1_val % 8;
                        const auto size_tp = tree_helper::Size(type);
                        const auto size_field_decl = tree_helper::Size(cr->op1);

                        if(right_shift_val == 0 and size_field_decl == size_tp)
                        {
                           ga->op1 = mr;
                        }
                        else
                        {
                           const auto mr_dup =
                               tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                           const auto mr_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), mr_dup,
                                                                           function_id, srcp_default);
                           GetPointerS<gimple_assign>(mr_ga)->memuse = ga->memuse;
                           GetPointerS<gimple_assign>(mr_ga)->vuses = ga->vuses;
                           GetPointerS<gimple_assign>(mr_ga)->vovers = ga->vovers;
                           ga->memuse = tree_nodeRef();
                           ga->vuses.clear();
                           ga->vovers.clear();
                           block.second->PushBefore(mr_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + mr_ga->ToString());
                           const auto mr_vd = GetPointerS<gimple_assign>(mr_ga)->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned loading
                            * the bitfield must not be checked, because it has
                            * not been resized yet and it may also load
                            * inconsistent stuff from other bitfields nearby
                            */
                           if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy) &&
                              (!parameters->isOption(OPT_discrepancy_force) ||
                               !parameters->getOption<bool>(OPT_discrepancy_force)))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---skip discrepancy of ssa " + mr_vd->ToString() +
                                                 " assigned in new stmt: " + mr_ga->ToString());
                              AppM->RDiscr->ssa_to_skip.insert(mr_vd);
                           }
#endif
                           if(right_shift_val)
                           {
                              const auto rshift_value = TM->CreateUniqueIntegerCst(right_shift_val, type);
                              const auto rsh_node = tree_man->create_binary_operation(type, mr_vd, rshift_value,
                                                                                      srcp_default, rshift_expr_K);
                              if(size_field_decl == size_tp)
                              {
                                 ga->op1 = rsh_node;
                              }
                              else
                              {
                                 const auto rsh_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                                  rsh_node, function_id, srcp_default);
                                 const auto rsh_vd = GetPointerS<gimple_assign>(rsh_ga)->op0;
                                 block.second->PushBefore(rsh_ga, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + rsh_ga->ToString());
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned loading
                                  * the bitfield must not be checked, because it has
                                  * not been resized yet and it may also load
                                  * inconsistent stuff from other bitfields nearby
                                  */
                                 if(parameters->isOption(OPT_discrepancy) &&
                                    parameters->getOption<bool>(OPT_discrepancy) &&
                                    (!parameters->isOption(OPT_discrepancy_force) ||
                                     !parameters->getOption<bool>(OPT_discrepancy_force)))
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---skip discrepancy of ssa " + rsh_vd->ToString() +
                                                       " assigned in new stmt: " + rsh_ga->ToString());
                                    AppM->RDiscr->ssa_to_skip.insert(rsh_vd);
                                 }
#endif
                                 const auto and_mask_value =
                                     TM->CreateUniqueIntegerCst((integer_cst_t(1) << size_field_decl) - 1, type);
                                 ga->op1 = tree_man->create_binary_operation(type, rsh_vd, and_mask_value, srcp_default,
                                                                             bit_and_expr_K);
                              }
                           }
                           else
                           {
                              const auto and_mask_value =
                                  TM->CreateUniqueIntegerCst((integer_cst_t(1) << size_field_decl) - 1, type);
                              ga->op1 = tree_man->create_binary_operation(type, mr_vd, and_mask_value, srcp_default,
                                                                          bit_and_expr_K);
                           }
                        }
                     }
                     else
                     {
                        ga->op1 = mr;
                     }
                     restart_analysis = true;
                  };
                  cr_expr1();
               }
               else if(code1 == vec_cond_expr_K)
               {
                  auto vc_expr1 = [&] {
                     const auto vce = GetPointerS<vec_cond_expr>(ga->op1);
                     THROW_ASSERT(vce->op1 && vce->op2, "expected three parameters");
                     if(GetPointer<binary_expr>(vce->op0))
                     {
                        const auto be = GetPointerS<binary_expr>(vce->op0);
                        THROW_ASSERT(be->get_kind() == le_expr_K or be->get_kind() == eq_expr_K or
                                         be->get_kind() == ne_expr_K or be->get_kind() == gt_expr_K or
                                         be->get_kind() == lt_expr_K or be->get_kind() == ge_expr_K,
                                     be->get_kind_text());
                        const auto new_ga = tree_man->CreateGimpleAssign(be->type, tree_nodeRef(), tree_nodeRef(),
                                                                         vce->op0, function_id, srcp_default);
                        const auto ssa_vd = GetPointerS<gimple_assign>(new_ga)->op0;

                        vce->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                  };
                  vc_expr1();
               }
               else if(code1 == view_convert_expr_K || code1 == nop_expr_K)
               {
                  auto vcne_expr1 = [&] {
                     auto* ue = GetPointer<unary_expr>(ga->op1);
                     if(ue->op->get_kind() == var_decl_K)
                     {
                        auto vc = GetPointer<view_convert_expr>(ga->op1);
                        tree_nodeRef pt = tree_man->GetPointerType(vc->type, GetPointer<type_node>(vc->type)->algn);
                        tree_nodeRef ae = tree_man->create_unary_operation(pt, ue->op, srcp_default, addr_expr_K);
                        tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                           function_id, srcp_default);
                        GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                        tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                        tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                        tree_nodeRef mr =
                            tree_man->create_binary_operation(vc->type, ssa_vd, offset, srcp_default, mem_ref_K);
                        ga->op1 = mr;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(ue->op->get_kind() != ssa_name_K && !GetPointer<cst_node>(ue->op))
                     {
                        auto type_index = tree_helper::get_type_index(TM, ue->op->index);
                        tree_nodeRef op_type = TM->GetTreeNode(type_index);
                        tree_nodeRef op_ga = tree_man->CreateGimpleAssign(op_type, tree_nodeRef(), tree_nodeRef(),
                                                                          ue->op, function_id, srcp_default);
                        tree_nodeRef op_vd = GetPointer<gimple_assign>(op_ga)->op0;
                        if(ga->temporary_address)
                        {
                           GetPointer<gimple_assign>(op_ga)->temporary_address = true;
                        }
                        block.second->PushBefore(op_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + op_ga->ToString());
                        ue->op = op_vd;
                        restart_analysis = true;
                     }
                  };
                  vcne_expr1();
               }
               else if(code1 == cond_expr_K)
               {
                  auto ce_expr1 = [&] {
                     auto* ce = GetPointer<cond_expr>(ga->op1);
                     THROW_ASSERT(ce->op1 && ce->op2, "expected three parameters");
                     if(GetPointer<binary_expr>(ce->op0))
                     {
#if HAVE_ASSERTS
                        auto* be = GetPointer<binary_expr>(ce->op0);
                        THROW_ASSERT(be->get_kind() == le_expr_K or be->get_kind() == eq_expr_K or
                                         be->get_kind() == ne_expr_K or be->get_kind() == gt_expr_K or
                                         be->get_kind() == lt_expr_K or be->get_kind() == ge_expr_K,
                                     be->get_kind_text());
#endif
                        auto bt = tree_man->GetBooleanType();
                        tree_nodeRef new_ga = tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                           TM->CreateUniqueIntegerCst(1, bt), ce->op0,
                                                                           function_id, srcp_default);
                        tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                        ce->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(!tree_helper::IsBooleanType(ce->op0))
                     {
                        const auto bt = tree_man->GetBooleanType();
                        const auto ga_nop = tree_man->CreateNopExpr(ce->op0, bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                    TM->CreateUniqueIntegerCst(1, bt), function_id);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + ga_nop->ToString());
                        block.second->PushBefore(ga_nop, *it_los, AppM);
                        ce->op0 = GetPointer<gimple_assign>(ga_nop)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        restart_analysis = true;
                     }
                  };
                  ce_expr1();
               }
               else if(code1 == indirect_ref_K)
               {
                  auto* ir = GetPointer<indirect_ref>(ga->op1);
                  tree_nodeRef type = ir->type;
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, tree_helper::CGetType(ir->op));
                  tree_nodeRef mr = tree_man->create_binary_operation(type, ir->op, offset, srcp_default, mem_ref_K);
                  ga->op1 = mr;
                  restart_analysis = true;
               }
               else if(code1 == misaligned_indirect_ref_K)
               {
                  auto* MIR = GetPointer<misaligned_indirect_ref>(ga->op1);
                  tree_nodeRef type = MIR->type;
                  tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                  tree_nodeRef mr = tree_man->create_binary_operation(type, MIR->op, offset, srcp_default, mem_ref_K);
                  ga->op1 = mr;
                  restart_analysis = true;
               }
               else if(code1 == var_decl_K && !ga->init_assignment)
               {
                  auto vd_expr1 = [&] {
                     auto* vd = GetPointer<var_decl>(ga->op1);
                     tree_nodeRef type = vd->type;
                     tree_nodeRef pt = tree_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
                     tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                     tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                        function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                     tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                     auto ssa_var_decl = GetPointer<ssa_name>(ssa_vd);
                     ssa_var_decl->use_set->Add(ga->op1);
                     tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                     ga->op1 = mr;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     restart_analysis = true;
                  };
                  vd_expr1();
               }
               else if(reached_max_transformation_limit(*it_los))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Reached max cfg transformations (" + ga->op1->get_kind_text() + ")");
               }
               else
               {
                  if(code1 == bit_field_ref_K)
                  {
                     auto bfe_expr1 = [&] {
                        auto* bfr = GetPointer<bit_field_ref>(ga->op1);
                        if(bfr->op0->get_kind() != ssa_name_K)
                        {
                           THROW_ASSERT(tree_helper::GetConstValue(bfr->op2) >= 0, "");
                           const auto op1_val = static_cast<unsigned int>(tree_helper::GetConstValue(bfr->op2));
                           const auto right_shift_val = op1_val % 8;
                           tree_nodeRef type;
                           if(bfr->type->get_kind() == boolean_type_K)
                           {
                              type = tree_man->GetCustomIntegerType(std::max(8u, ceil_pow2(1 + right_shift_val)), true);
                           }
                           else
                           {
                              const auto it = GetPointer<integer_type>(bfr->type);
                              THROW_ASSERT(it, "unexpected pattern");
                              type = tree_man->GetCustomIntegerType(std::max(8u, ceil_pow2(it->prec + right_shift_val)),
                                                                    it->unsigned_flag);
                           }
                           tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                           tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                           tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                              function_id, srcp_default);
                           GetPointer<gimple_assign>(new_ga)->temporary_address = true;

                           tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                           block.second->PushBefore(new_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + new_ga->ToString());
                           AppM->RegisterTransformation(GetName(), new_ga);
                           const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                           const auto mr =
                               tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                           const auto size_tp = tree_helper::Size(type);
                           THROW_ASSERT(tree_helper::GetConstValue(bfr->op1) >= 0, "");
                           const auto size_field_decl =
                               static_cast<unsigned long long>(tree_helper::GetConstValue(bfr->op1));
                           if(right_shift_val == 0 && size_field_decl == size_tp)
                           {
                              ga->op1 = mr;
                           }
                           else
                           {
                              tree_nodeRef mr_dup =
                                  tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                              tree_nodeRef mr_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                                mr_dup, function_id, srcp_default);
                              tree_nodeRef mr_vd = GetPointer<gimple_assign>(mr_ga)->op0;
                              GetPointer<gimple_assign>(mr_ga)->memuse = ga->memuse;
                              GetPointer<gimple_assign>(mr_ga)->vuses = ga->vuses;
                              GetPointer<gimple_assign>(mr_ga)->vovers = ga->vovers;
                              ga->memuse = tree_nodeRef();
                              ga->vuses.clear();
                              ga->vovers.clear();
                              block.second->PushBefore(mr_ga, *it_los, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---adding statement " + mr_ga->ToString());
#if HAVE_FROM_DISCREPANCY_BUILT
                              /*
                               * for discrepancy analysis, the ssa assigned loading
                               * the bitfield must not be checked, because it has
                               * not been resized yet and it may also load
                               * inconsistent stuff from other bitfields nearby
                               */
                              if(parameters->isOption(OPT_discrepancy) and
                                 parameters->getOption<bool>(OPT_discrepancy) and
                                 (not parameters->isOption(OPT_discrepancy_force) or
                                  not parameters->getOption<bool>(OPT_discrepancy_force)))
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---skip discrepancy of ssa " + mr_vd->ToString() +
                                                    " assigned in new stmt: " + mr_ga->ToString());
                                 AppM->RDiscr->ssa_to_skip.insert(mr_vd);
                              }
#endif
                              if(right_shift_val)
                              {
                                 tree_nodeRef rshift_value = TM->CreateUniqueIntegerCst(right_shift_val, type);
                                 tree_nodeRef rsh_node = tree_man->create_binary_operation(type, mr_vd, rshift_value,
                                                                                           srcp_default, rshift_expr_K);
                                 if(size_field_decl == size_tp)
                                 {
                                    ga->op1 = rsh_node;
                                 }
                                 else
                                 {
                                    tree_nodeRef rsh_ga = tree_man->CreateGimpleAssign(
                                        type, tree_nodeRef(), tree_nodeRef(), rsh_node, function_id, srcp_default);
                                    tree_nodeRef rsh_vd = GetPointer<gimple_assign>(rsh_ga)->op0;
                                    block.second->PushBefore(rsh_ga, *it_los, AppM);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---adding statement " + rsh_ga->ToString());
#if HAVE_FROM_DISCREPANCY_BUILT
                                    /*
                                     * for discrepancy analysis, the ssa assigned loading
                                     * the bitfield must not be checked, because it has
                                     * not been resized yet and it may also load
                                     * inconsistent stuff from other bitfields nearby
                                     */
                                    if(parameters->isOption(OPT_discrepancy) and
                                       parameters->getOption<bool>(OPT_discrepancy) and
                                       (not parameters->isOption(OPT_discrepancy_force) or
                                        not parameters->getOption<bool>(OPT_discrepancy_force)))
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---skip discrepancy of ssa " + rsh_vd->ToString() +
                                                          " assigned in new stmt: " + rsh_ga->ToString());
                                       AppM->RDiscr->ssa_to_skip.insert(rsh_vd);
                                    }
#endif
                                    tree_nodeRef and_mask_value = TM->CreateUniqueIntegerCst(
                                        static_cast<long long int>((1ULL << size_field_decl) - 1), type);
                                    ga->op1 = tree_man->create_binary_operation(type, rsh_vd, and_mask_value,
                                                                                srcp_default, bit_and_expr_K);
                                 }
                              }
                              else
                              {
                                 tree_nodeRef and_mask_value = TM->CreateUniqueIntegerCst(
                                     static_cast<long long int>((1ULL << size_field_decl) - 1), type);
                                 ga->op1 = tree_man->create_binary_operation(type, mr_vd, and_mask_value, srcp_default,
                                                                             bit_and_expr_K);
                              }
                           }
                           restart_analysis = true;
                        }
                        else
                        {
                           const auto type_node = tree_helper::CGetType(bfr->op0);
                           bool is_scalar = tree_helper::IsRealType(type_node) ||
                                            tree_helper::IsSignedIntegerType(type_node) ||
                                            tree_helper::IsUnsignedIntegerType(type_node);
                           if(is_scalar)
                           {
                              auto data_size = tree_helper::Size(type_node);
                              const auto type_vc =
                                  tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(data_size)), true);
                              const auto vc_expr = tree_man->create_unary_operation(type_vc, bfr->op0, srcp_default,
                                                                                    view_convert_expr_K);
                              const auto vc_ga = tree_man->CreateGimpleAssign(type_vc, tree_nodeRef(), tree_nodeRef(),
                                                                              vc_expr, function_id, srcp_default);
                              block.second->PushBefore(vc_ga, *it_los, AppM);
                              const auto vc_ga_var = GetPointer<gimple_assign>(vc_ga)->op0;
                              THROW_ASSERT(tree_helper::GetConstValue(bfr->op1) >= 0, "");
                              const auto sel_size =
                                  static_cast<unsigned long long>(tree_helper::GetConstValue(bfr->op1));
                              const auto vc_shift_expr = tree_man->create_binary_operation(type_vc, vc_ga_var, bfr->op2,
                                                                                           srcp_default, rshift_expr_K);
                              const auto vc_shift_ga = tree_man->CreateGimpleAssign(
                                  type_vc, tree_nodeRef(), tree_nodeRef(), vc_shift_expr, function_id, srcp_default);
                              block.second->PushBefore(vc_shift_ga, *it_los, AppM);
                              const auto vc_shift_ga_var = GetPointer<gimple_assign>(vc_shift_ga)->op0;
                              const auto sel_type =
                                  tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(sel_size)), true);
                              const auto vc_nop_expr =
                                  tree_man->create_unary_operation(sel_type, vc_shift_ga_var, srcp_default, nop_expr_K);
                              const auto vc_nop_ga = tree_man->CreateGimpleAssign(
                                  sel_type, tree_nodeRef(), tree_nodeRef(), vc_nop_expr, function_id, srcp_default);
                              block.second->PushBefore(vc_nop_ga, *it_los, AppM);
                              const auto vc_nop_ga_var = GetPointer<gimple_assign>(vc_nop_ga)->op0;
                              const auto fvc_expr = tree_man->create_unary_operation(bfr->type, vc_nop_ga_var,
                                                                                     srcp_default, view_convert_expr_K);
                              ga->op1 = fvc_expr;
                              restart_analysis = true;
                           }
                        }
                     };
                     bfe_expr1();
                  }
                  else if(code1 == trunc_div_expr_K || code1 == trunc_mod_expr_K || code1 == exact_div_expr_K)
                  {
                     auto be = GetPointer<binary_expr>(ga->op1);
                     tree_nodeRef op1 = be->op1;
                     if(GetPointer<cst_node>(op1))
                     {
                        division_by_a_constant(block, it_los, ga, op1, code1, restart_analysis, srcp_default,
                                               GetName());
                     }
                  }
                  else if(code1 == mult_expr_K)
                  {
                     auto me_expr1 = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "-->Expanding mult_expr  " + STR(ga->op1->index));
                        tree_nodeRef op1 = GetPointer<binary_expr>(ga->op1)->op1;
                        if(GetPointer<integer_cst>(op1))
                        {
                           auto* cn = GetPointer<integer_cst>(op1);
                           tree_nodeRef op0 = GetPointer<binary_expr>(ga->op1)->op0;
                           tree_nodeRef type_expr = GetPointer<binary_expr>(ga->op1)->type;

                           if(cn)
                           {
                              auto prev_index = ga->op1->index;
                              ga->op1 = expand_MC(op0, cn, ga->op1, *it_los, block.second, type_expr, srcp_default);
                              restart_analysis = restart_analysis || (prev_index != ga->op1->index);
                              if(prev_index != ga->op1->index)
                              {
                                 AppM->RegisterTransformation(GetName(), *it_los);
                              }
                           }
                        }
                        /// if the previous transformation still give a multiplication let's check if this
                        /// multiplication is actually widen_mult_expr
                        if(not reached_max_transformation_limit(*it_los) and ga->op1->get_kind() == mult_expr_K)
                        {
                           tree_nodeRef type_expr = GetPointer<binary_expr>(ga->op1)->type;

                           bool realp = tree_helper::is_real(TM, type_expr->index);
                           if(!realp)
                           {
                              /// check if a mult_expr may become a widen_mult_expr
                              auto dw_out = tree_helper::Size(ga->op0);
                              auto data_bitsize_out = ceil_pow2(dw_out);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "---data_bitsize_out " + STR(data_bitsize_out) + " <- " + STR(dw_out) +
                                                 "\n");
                              auto dw_in0 = tree_helper::Size(GetPointer<binary_expr>(ga->op1)->op0);
                              auto data_bitsize_in0 = ceil_pow2(dw_in0);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "---data_bitsize_in0 " + STR(data_bitsize_in0) + " <- " + STR(dw_in0) +
                                                 "\n");
                              auto dw_in1 = tree_helper::Size(GetPointer<binary_expr>(ga->op1)->op1);
                              auto data_bitsize_in1 = ceil_pow2(dw_in1);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "---data_bitsize_in1 " + STR(data_bitsize_in1) + " <- " + STR(dw_in1) +
                                                 "\n");
                              if(std::max(data_bitsize_in0, data_bitsize_in1) * 2 == data_bitsize_out)
                              {
                                 auto type_index = tree_helper::get_type_index(TM, ga->op0->index);
                                 tree_nodeRef op0_type = TM->GetTreeNode(type_index);
                                 ga->op1 = tree_man->create_binary_operation(
                                     op0_type, GetPointer<binary_expr>(ga->op1)->op0,
                                     GetPointer<binary_expr>(ga->op1)->op1, srcp_default, widen_mult_expr_K);
                                 restart_analysis = true;
                                 AppM->RegisterTransformation(GetName(), *it_los);
                              }
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
                     };
                     me_expr1();
                  }
                  else if(code1 == widen_mult_expr_K)
                  {
                     auto wme_expr1 = [&] {
                        const auto be = GetPointer<binary_expr>(ga->op1);
                        tree_nodeRef op1 = be->op1;
                        if(GetPointer<cst_node>(op1) && !GetPointer<vector_cst>(op1))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "-->Expanding widen_mult_expr  " + STR(ga->op1->index));
                           auto* cn = GetPointer<cst_node>(op1);
                           tree_nodeRef op0 = GetPointer<binary_expr>(ga->op1)->op0;
                           tree_nodeRef type_expr = GetPointer<binary_expr>(ga->op1)->type;

                           bool realp = tree_helper::IsRealType(type_expr);
                           if(!realp)
                           {
                              auto prev_index = ga->op1->index;
                              ga->op1 = expand_MC(op0, static_cast<integer_cst*>(cn), ga->op1, *it_los, block.second,
                                                  type_expr, srcp_default);
                              restart_analysis = restart_analysis || (prev_index != ga->op1->index);
                              if(prev_index != ga->op1->index)
                              {
                                 AppM->RegisterTransformation(GetName(), *it_los);
                              }
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
                        }
                        else
                        {
                           if(tree_helper::IsUnsignedIntegerType(be->type) !=
                              tree_helper::IsUnsignedIntegerType(be->op0))
                           {
                              THROW_ASSERT(tree_helper::IsUnsignedIntegerType(be->type),
                                           "Conversion from unsigned to signed is required for first input of " +
                                               STR(ga->op1->index));
                              const auto ga_nop = tree_man->CreateNopExpr(
                                  be->op0, tree_man->CreateUnsigned(tree_helper::CGetType(be->op0)), tree_nodeRef(),
                                  tree_nodeRef(), function_id);
                              if(ga_nop)
                              {
                                 be->op0 = GetPointer<gimple_assign>(ga_nop)->op0;
                                 AppM->RegisterTransformation(GetName(), ga_nop);
                                 block.second->PushBefore(ga_nop, *it_los, AppM);
                                 restart_analysis = true;
                              }
                              else
                              {
#ifndef NDEBUG
                                 THROW_UNREACHABLE("Conversion of " + be->op0->ToString() + " cannot be created");
#else
                                 THROW_WARNING("Implicit type conversion for first input of " + ga->ToString());
#endif
                              }
                           }
                           if(tree_helper::IsUnsignedIntegerType(be->type) !=
                              tree_helper::IsUnsignedIntegerType(be->op1))
                           {
                              THROW_ASSERT(tree_helper::IsUnsignedIntegerType(be->type),
                                           "Conversion from unsigned to signed is required for first input of " +
                                               STR(ga->op1->index));
                              const auto ga_nop = tree_man->CreateNopExpr(
                                  be->op1, tree_man->CreateUnsigned(tree_helper::CGetType(be->op1)), tree_nodeRef(),
                                  tree_nodeRef(), function_id);
                              if(ga_nop)
                              {
                                 be->op1 = GetPointer<gimple_assign>(ga_nop)->op0;
                                 block.second->PushBefore(ga_nop, *it_los, AppM);
                                 AppM->RegisterTransformation(GetName(), ga_nop);
                                 restart_analysis = true;
                              }
                              else
                              {
#ifndef NDEBUG
                                 THROW_UNREACHABLE("Conversion of " + be->op1->ToString() + " cannot be created");
#else
                                 THROW_WARNING("Implicit type conversion for first input of " + ga->ToString());
#endif
                              }
                           }
                        }
                     };
                     wme_expr1();
                  }
                  else if(code1 == lt_expr_K)
                  {
                     auto lt_expr1 = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "-->Expanding lt_expr " + STR(ga->op1->index));
                        auto be = GetPointer<binary_expr>(ga->op1);
                        tree_nodeRef op0 = be->op0;
                        bool intp = tree_helper::is_int(TM, op0->index);
                        if(intp)
                        {
                           tree_nodeRef op1 = be->op1;
                           if(GetPointer<integer_cst>(op1))
                           {
                              if(tree_helper::GetConstValue(op1) == 0)
                              {
                                 auto type_index = tree_helper::get_type_index(TM, op0->index);
                                 tree_nodeRef op0_type = TM->GetTreeNode(type_index);
                                 tree_nodeRef right_shift_value = TM->CreateUniqueIntegerCst(
                                     static_cast<long long>(tree_helper::Size(op0) - 1), op0_type);
                                 tree_nodeRef rshift1 = tree_man->create_binary_operation(
                                     op0_type, op0, right_shift_value, srcp_default, rshift_expr_K);
                                 tree_nodeRef rshift1_ga = tree_man->CreateGimpleAssign(
                                     op0_type, tree_nodeRef(), tree_nodeRef(), rshift1, function_id, srcp_default);
                                 block.second->PushBefore(rshift1_ga, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + rshift1_ga->ToString());
                                 tree_nodeRef rshift1_ga_var = GetPointer<gimple_assign>(rshift1_ga)->op0;
                                 tree_nodeRef bitwise_mask_value = TM->CreateUniqueIntegerCst(1, op0_type);
                                 tree_nodeRef bitwise_masked = tree_man->create_binary_operation(
                                     op0_type, rshift1_ga_var, bitwise_mask_value, srcp_default, bit_and_expr_K);
                                 tree_nodeRef bitwise_masked_ga =
                                     tree_man->CreateGimpleAssign(op0_type, tree_nodeRef(), tree_nodeRef(),
                                                                  bitwise_masked, function_id, srcp_default);
                                 block.second->PushBefore(bitwise_masked_ga, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + bitwise_masked_ga->ToString());
                                 tree_nodeRef bitwise_masked_var = GetPointer<gimple_assign>(bitwise_masked_ga)->op0;
                                 tree_nodeRef ne = tree_man->create_unary_operation(be->type, bitwise_masked_var,
                                                                                    srcp_default, nop_expr_K);
                                 tree_nodeRef ga_nop = tree_man->CreateGimpleAssign(
                                     be->type, tree_nodeRef(), tree_nodeRef(), ne, function_id, srcp_default);
                                 block.second->PushBefore(ga_nop, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + ga_nop->ToString());
                                 AppM->RegisterTransformation(GetName(), ga_nop);
                                 ga->op1 = GetPointer<gimple_assign>(ga_nop)->op0;
                                 restart_analysis = true;
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                              }
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                           }
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                        }
                     };
                     lt_expr1();
                  }
                  else if(code1 == ge_expr_K)
                  {
                     auto ge_expr1 = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "-->Expanding ge_expr " + STR(ga->op1->index));
                        tree_nodeRef op0 = GetPointer<binary_expr>(ga->op1)->op0;
                        bool intp = tree_helper::is_int(TM, op0->index);
                        if(intp)
                        {
                           tree_nodeRef op1 = GetPointer<binary_expr>(ga->op1)->op1;
                           if(GetPointer<integer_cst>(op1))
                           {
                              const auto op1_value = tree_helper::GetConstValue(op1);
                              if(op1_value == 0)
                              {
                                 auto type_index = tree_helper::get_type_index(TM, op0->index);
                                 tree_nodeRef op0_type = TM->GetTreeNode(type_index);
                                 tree_nodeRef right_shift_value = TM->CreateUniqueIntegerCst(
                                     static_cast<long long>(tree_helper::Size(op0) - 1), op0_type);
                                 tree_nodeRef rshift1 = tree_man->create_binary_operation(
                                     op0_type, op0, right_shift_value, srcp_default, rshift_expr_K);
                                 tree_nodeRef rshift1_ga = tree_man->CreateGimpleAssign(
                                     op0_type, tree_nodeRef(), tree_nodeRef(), rshift1, function_id, srcp_default);
                                 block.second->PushBefore(rshift1_ga, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + rshift1_ga->ToString());
                                 tree_nodeRef rshift1_ga_var = GetPointer<gimple_assign>(rshift1_ga)->op0;
                                 tree_nodeRef bitwise_mask_value = TM->CreateUniqueIntegerCst(1, op0_type);
                                 tree_nodeRef bitwise_masked = tree_man->create_binary_operation(
                                     op0_type, rshift1_ga_var, bitwise_mask_value, srcp_default, bit_and_expr_K);
                                 tree_nodeRef bitwise_masked_ga =
                                     tree_man->CreateGimpleAssign(op0_type, tree_nodeRef(), tree_nodeRef(),
                                                                  bitwise_masked, function_id, srcp_default);
                                 block.second->PushBefore(bitwise_masked_ga, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + bitwise_masked_ga->ToString());
                                 tree_nodeRef bitwise_masked_var = GetPointer<gimple_assign>(bitwise_masked_ga)->op0;
                                 tree_nodeRef ne =
                                     tree_man->create_unary_operation(GetPointer<binary_expr>(ga->op1)->type,
                                                                      bitwise_masked_var, srcp_default, nop_expr_K);
                                 tree_nodeRef ga_nop = tree_man->CreateGimpleAssign(
                                     GetPointer<binary_expr>(ga->op1)->type, tree_nodeRef(), tree_nodeRef(), ne,
                                     function_id, srcp_default);
                                 block.second->PushBefore(ga_nop, *it_los, AppM);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---adding statement " + ga_nop->ToString());
                                 tree_nodeRef ga_nop_var = GetPointer<gimple_assign>(ga_nop)->op0;
                                 auto booleanType = tree_man->GetBooleanType();
                                 tree_nodeRef not_masked = tree_man->create_unary_operation(
                                     booleanType, ga_nop_var, srcp_default, truth_not_expr_K);
                                 ga->op1 = not_masked;
                                 restart_analysis = true;
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                              }
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                           }
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
                        }
                     };
                     ge_expr1();
                  }
                  else if(code1 == plus_expr_K)
                  {
                     auto pe_expr1 = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "-->Expanding plus_expr " + STR(ga->op1->index));
                        tree_nodeRef bop0 = GetPointer<binary_expr>(ga->op1)->op0;
                        tree_nodeRef bop1 = GetPointer<binary_expr>(ga->op1)->op1;
                        if(tree_helper::Size(tree_helper::CGetType(ga->op0)) !=
                           tree_helper::Size(tree_helper::CGetType(bop0)))
                        {
                           auto ssa0 = GetPointerS<ssa_name>(ga->op0);
                           const auto ga_nop =
                               tree_man->CreateNopExpr(bop0, ssa0->type, ssa0->min, ssa0->max, function_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + ga_nop->ToString());
                           const auto nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                           block.second->PushBefore(ga_nop, *it_los, AppM);
                           GetPointer<binary_expr>(ga->op1)->op0 = bop1 = nop_vd;
                           restart_analysis = true;
                        }
                        if(tree_helper::Size(tree_helper::CGetType(ga->op0)) !=
                           tree_helper::Size(tree_helper::CGetType(bop1)))
                        {
                           auto ssa0 = GetPointerS<ssa_name>(ga->op0);
                           const auto ga_nop =
                               tree_man->CreateNopExpr(bop1, ssa0->type, ssa0->min, ssa0->max, function_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + ga_nop->ToString());
                           const auto nop_vd = GetPointer<gimple_assign>(ga_nop)->op0;
                           block.second->PushBefore(ga_nop, *it_los, AppM);
                           GetPointer<binary_expr>(ga->op1)->op1 = bop1 = nop_vd;
                           restart_analysis = true;
                        }
                        bool intp = tree_helper::IsSignedIntegerType(bop0) || tree_helper::IsUnsignedIntegerType(bop0);
                        if(intp)
                        {
                           if(bop0->index == bop1->index)
                           {
                              tree_nodeRef type = GetPointer<binary_expr>(ga->op1)->type;
                              tree_nodeRef left_shift_value = TM->CreateUniqueIntegerCst(1, type);
                              tree_nodeRef left1 = tree_man->create_binary_operation(type, bop0, left_shift_value,
                                                                                     srcp_default, lshift_expr_K);
                              ga->op1 = left1;
                              restart_analysis = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not Expanded");
                           }
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not Expanded");
                        }
                     };
                     pe_expr1();
                  }
                  else if(code1 == parm_decl_K)
                  {
                     auto* pd = GetPointer<parm_decl>(ga->op1);
                     tree_nodeRef type = pd->type;
                     tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                     tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op1, srcp_default, addr_expr_K);
                     tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                        function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                     tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                     tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                     ga->op1 = mr;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     AppM->RegisterTransformation(GetName(), new_ga);
                     restart_analysis = true;
                  }
               }

               if(code0 == array_ref_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand array_ref 2 " + STR(ga->op0->index));
                  auto* AR = GetPointer<array_ref>(ga->op0);
                  ga->op0 = array_ref_lowering(AR, srcp_default, block, it_los, true);
                  restart_analysis = true;
               }
               else if(code0 == mem_ref_K)
               {
                  auto mem_ref0 = [&] {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand mem_ref 2 " + STR(ga->op0->index));
                     auto* MR = GetPointer<mem_ref>(ga->op0);
                     const auto mem_op0_kind = MR->op0->get_kind();
                     if(mem_op0_kind == addr_expr_K || mem_op0_kind == pointer_plus_expr_K ||
                        mem_op0_kind == view_convert_expr_K)
                     {
                        extract_unary_expr(MR->op0, mem_op0_kind == addr_expr_K, true);
                     }

                     tree_nodeRef op1 = MR->op1;
                     const auto op1_val = tree_helper::GetConstValue(op1);
                     if(op1_val != 0)
                     {
                        tree_nodeRef type = MR->type;

                        /// check if there is a misaligned access
                        auto obj_size = tree_helper::Size(tree_helper::CGetType(ga->op0));
                        auto bram_size = std::max(8ull, obj_size / 2);
                        /// check if the mem_ref corresponds to an implicit memset and then it will be lowered to simple
                        /// loop initializing the variable
                        bool implicit_memset = ga->op1->get_kind() == constructor_K &&
                                               GetPointer<constructor>(ga->op1) &&
                                               GetPointer<constructor>(ga->op1)->list_of_idx_valu.size() == 0;
                        if(implicit_memset)
                        {
                           auto var = tree_helper::get_base_index(TM, MR->op0->index);
                           implicit_memset = var != 0;
                           if(implicit_memset)
                           {
                              auto type_index = tree_helper::get_type_index(TM, var);
                              const auto type_node = TM->GetTreeNode(type_index);
                              implicit_memset = type_node->get_kind() == array_type_K;
                           }
                        }
                        if(!implicit_memset && ((((op1_val * 8)) % static_cast<long long>(bram_size)) != 0))
                        {
                           function_behavior->set_unaligned_accesses(true);
                        }

                        const auto pt = tree_man->GetPointerType(type, 8);
                        const auto ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                        const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                         function_id, srcp_default);
                        GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                        const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                        const auto offset = TM->CreateUniqueIntegerCst(0, pt);
                        const auto mr =
                            tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                        ga->op0 = mr;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     const auto op1_type = tree_helper::CGetType(ga->op1);
                     auto view_convert_pattern =
                         op1_type->get_kind() == record_type_K && ga->op1->get_kind() == view_convert_expr_K;
                     if(!view_convert_pattern && ga->op1->get_kind() != ssa_name_K && !GetPointer<cst_node>(ga->op1) &&
                        ga->op1->get_kind() != mem_ref_K && ga->op1->get_kind() != constructor_K)
                     {
                        auto type_index = tree_helper::get_type_index(TM, ga->op1->index);
                        const auto op_type = TM->GetTreeNode(type_index);
                        const auto op_ga = tree_man->CreateGimpleAssign(op_type, tree_nodeRef(), tree_nodeRef(),
                                                                        ga->op1, function_id, srcp_default);
                        const auto op_vd = GetPointer<gimple_assign>(op_ga)->op0;
                        block.second->PushBefore(op_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + op_ga->ToString());
                        ga->op1 = op_vd;
                        restart_analysis = true;
                     }
                  };
                  mem_ref0();
               }
               else if(code0 == component_ref_K)
               {
                  auto component_ref0 = [&] {
                     auto* cr = GetPointer<component_ref>(ga->op0);
                     auto* field_d = GetPointer<field_decl>(cr->op1);
                     tree_nodeRef type;
                     if(field_d->is_bitfield())
                     {
                        THROW_ASSERT(tree_helper::GetConstValue(field_d->bpos) >= 0, "");
                        auto op1_val = static_cast<unsigned long long>(tree_helper::GetConstValue(field_d->bpos));
                        auto right_shift_val = op1_val % 8;
                        if(cr->type->get_kind() == boolean_type_K)
                        {
                           type = tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(1 + right_shift_val)), true);
                        }
                        else
                        {
                           auto* it = GetPointer<integer_type>(cr->type);
                           THROW_ASSERT(it, "unexpected pattern");
                           type = tree_man->GetCustomIntegerType(std::max(8ull, ceil_pow2(it->prec + right_shift_val)),
                                                                 it->unsigned_flag);
                        }
                     }
                     else
                     {
                        type = cr->type;
                     }
                     const auto pt = tree_man->GetPointerType(type, 8);
                     const auto ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                     const auto new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                      function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;

                     tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());

                     if(field_d->packed_flag)
                     {
                        function_behavior->set_packed_vars(true);
                     }

                     tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                     if(field_d->is_bitfield())
                     {
                        THROW_ASSERT(tree_helper::GetConstValue(field_d->bpos) >= 0, "");
                        auto op1_val = static_cast<unsigned int>(tree_helper::GetConstValue(field_d->bpos));
                        auto right_shift_val = op1_val % 8;
                        auto size_tp = tree_helper::Size(type);
                        auto size_field_decl = tree_helper::Size(cr->op1);

                        if(right_shift_val == 0 and size_field_decl == size_tp)
                        {
                           ga->op0 = mr;
                        }
                        else
                        {
                           tree_nodeRef mr_dup =
                               tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                           tree_nodeRef load_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                               mr_dup, function_id, srcp_default);
                           tree_nodeRef load_vd = GetPointer<gimple_assign>(load_ga)->op0;
                           GetPointer<gimple_assign>(load_ga)->memuse = ga->memuse;
                           GetPointer<gimple_assign>(load_ga)->vuses = ga->vuses;
                           for(const auto& vd : bitfield_vuses)
                           {
                              GetPointer<gimple_assign>(load_ga)->AddVuse(vd);
                           }
                           THROW_ASSERT(ga->vdef, "unexpected condition");
                           bitfield_vdefs.insert(ga->vdef);
                           block.second->PushBefore(load_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + load_ga->ToString());

                           tree_nodeRef load_var;
                           if(size_field_decl != size_tp)
                           {
#if HAVE_FROM_DISCREPANCY_BUILT
                              /*
                               * for discrepancy analysis, the ssa assigned loading
                               * the bitfield must not be checked, because it has
                               * not been resized yet and it may also load
                               * inconsistent stuff from other bitfields nearby
                               */
                              if(parameters->isOption(OPT_discrepancy) and
                                 parameters->getOption<bool>(OPT_discrepancy) and
                                 (not parameters->isOption(OPT_discrepancy_force) or
                                  not parameters->getOption<bool>(OPT_discrepancy_force)))
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---skip discrepancy of ssa " + load_vd->ToString() +
                                                    " assigned in new stmt: " + load_ga->ToString());
                                 AppM->RDiscr->ssa_to_skip.insert(load_vd);
                              }
#endif
                              tree_nodeRef nmask_value = TM->CreateUniqueIntegerCst(
                                  static_cast<long long int>(~(((1ULL << size_field_decl) - 1) << right_shift_val)),
                                  type);
                              tree_nodeRef nmask_node = tree_man->create_binary_operation(type, load_vd, nmask_value,
                                                                                          srcp_default, bit_and_expr_K);
                              tree_nodeRef nmask_ga = tree_man->CreateGimpleAssign(
                                  type, tree_nodeRef(), tree_nodeRef(), nmask_node, function_id, srcp_default);
                              block.second->PushBefore(nmask_ga, *it_los, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---adding statement " + nmask_ga->ToString());
                              load_var = GetPointer<gimple_assign>(nmask_ga)->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                              /*
                               * for discrepancy analysis, the ssa assigned loading
                               * the bitfield must not be checked, because it has
                               * not been resized yet and it may also load
                               * inconsistent stuff from other bitfields nearby
                               */
                              if(parameters->isOption(OPT_discrepancy) and
                                 parameters->getOption<bool>(OPT_discrepancy) and
                                 (not parameters->isOption(OPT_discrepancy_force) or
                                  not parameters->getOption<bool>(OPT_discrepancy_force)))
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---skip discrepancy of ssa " + load_var->ToString() +
                                                    " assigned in new stmt: " + nmask_ga->ToString());
                                 AppM->RDiscr->ssa_to_skip.insert(load_var);
                              }
#endif
                           }
                           else
                           {
                              THROW_ASSERT(right_shift_val == 0, "unexpected pattern");
                              load_var = load_vd;
                           }
                           tree_nodeRef written_var;
                           if(size_field_decl != size_tp)
                           {
                              tree_nodeRef and_mask_value = TM->CreateUniqueIntegerCst(
                                  static_cast<long long int>((1ULL << size_field_decl) - 1), type);
                              tree_nodeRef and_mask_node = tree_man->create_binary_operation(
                                  type, ga->op1, and_mask_value, srcp_default, bit_and_expr_K);
                              tree_nodeRef and_mask_ga = tree_man->CreateGimpleAssign(
                                  type, tree_nodeRef(), tree_nodeRef(), and_mask_node, function_id, srcp_default);
                              block.second->PushBefore(and_mask_ga, *it_los, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---adding statement " + and_mask_ga->ToString());
                              written_var = GetPointer<gimple_assign>(and_mask_ga)->op0;
                           }
                           else
                           {
                              written_var = ga->op1;
                           }
                           tree_nodeRef lshifted_var;
                           if(right_shift_val)
                           {
                              tree_nodeRef lsh_value = TM->CreateUniqueIntegerCst(right_shift_val, type);
                              tree_nodeRef lsh_node = tree_man->create_binary_operation(type, written_var, lsh_value,
                                                                                        srcp_default, lshift_expr_K);
                              tree_nodeRef lsh_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                                 lsh_node, function_id, srcp_default);
                              block.second->PushBefore(lsh_ga, *it_los, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---adding statement " + lsh_ga->ToString());
                              lshifted_var = GetPointer<gimple_assign>(lsh_ga)->op0;
                           }
                           else
                           {
                              lshifted_var = written_var;
                           }

                           tree_nodeRef res_node = tree_man->create_binary_operation(type, lshifted_var, load_var,
                                                                                     srcp_default, bit_ior_expr_K);
                           tree_nodeRef res_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                              res_node, function_id, srcp_default);
                           block.second->PushBefore(res_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + res_ga->ToString());
                           tree_nodeRef res_vd = GetPointer<gimple_assign>(res_ga)->op0;
                           ga->op1 = res_vd;
                           ga->op0 = mr;
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned loading
                            * the bitfield must not be checked, because it has
                            * not been resized yet and it may also load
                            * inconsistent stuff from other bitfields nearby
                            */
                           if(size_field_decl != size_tp and parameters->isOption(OPT_discrepancy) and
                              parameters->getOption<bool>(OPT_discrepancy) and
                              (not parameters->isOption(OPT_discrepancy_force) or
                               not parameters->getOption<bool>(OPT_discrepancy_force)))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---skip discrepancy of ssa " + res_vd->ToString() +
                                                 " assigned in new stmt: " + res_ga->ToString());
                              AppM->RDiscr->ssa_to_skip.insert(res_vd);
                           }
#endif
                        }
                     }
                     else
                     {
                        ga->op0 = mr;
                     }
                     restart_analysis = true;
                  };
                  component_ref0();
               }
               else if(code0 == target_mem_ref461_K)
               {
                  auto tmr0 = [&] {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---expand target_mem_ref 2 " + STR(ga->op0->index));

                     auto* tmr = GetPointer<target_mem_ref461>(ga->op0);
                     /// split the target_mem_ref in a address computation statement and in a store statement
                     tree_nodeRef type = tmr->type;

                     tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                     tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                     tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                        function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                     tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                     tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                     ga->op0 = mr;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     restart_analysis = true;
                  };
                  tmr0();
               }
               else if(code0 == var_decl_K && !ga->init_assignment)
               {
                  auto* vd = GetPointer<var_decl>(ga->op0);
                  tree_nodeRef type = vd->type;
                  // std::cerr << "algn" << GetPointer<type_node>(type)->algn << "\n";
                  tree_nodeRef pt = tree_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
                  tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                  tree_nodeRef new_ga =
                      tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae, function_id, srcp_default);
                  GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                  tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                  auto ssa_var_decl = GetPointer<ssa_name>(ssa_vd);
                  ssa_var_decl->use_set->Add(ga->op0);
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                  tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                  ga->op0 = mr;
                  block.second->PushBefore(new_ga, *it_los, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + new_ga->ToString());
                  restart_analysis = true;

                  //                  {
                  //                     /// check if volatile attribute is missing
                  //                     /// it may happen with gcc v4.8 or greater
                  //                     if(vd->scpe && vd->scpe->get_kind() != translation_unit_decl_K &&
                  //                     !vd->static_flag && !tree_helper::is_volatile(TM,ga->op1->index) &&
                  //                           vd->type->get_kind() != array_type_K &&
                  //                           vd->type->get_kind() != record_type_K &&
                  //                           vd->type->get_kind() != union_type_K)
                  //                     {
                  //                        /// w.r.t. bambu this kind of non-ssa variable is equivalent to a static
                  //                        local variable vd->static_flag = true;
                  //                     }
                  //                  }
               }
               else if(code0 == indirect_ref_K)
               {
                  auto* ir = GetPointer<indirect_ref>(ga->op0);
                  tree_nodeRef type = ir->type;
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, tree_helper::CGetType(ir->op));
                  tree_nodeRef mr = tree_man->create_binary_operation(type, ir->op, offset, srcp_default, mem_ref_K);
                  ga->op0 = mr;
                  restart_analysis = true;
               }
               else if(code0 == misaligned_indirect_ref_K)
               {
                  auto* MIR = GetPointer<misaligned_indirect_ref>(ga->op0);
                  function_behavior->set_unaligned_accesses(true);
                  tree_nodeRef type = MIR->type;
                  tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                  tree_nodeRef mr = tree_man->create_binary_operation(type, MIR->op, offset, srcp_default, mem_ref_K);
                  ga->op0 = mr;
                  restart_analysis = true;
               }
               else if(code0 == realpart_expr_K)
               {
                  ga->op0 = manage_realpart(ga->op0, tree_nodeRef());
                  restart_analysis = true;
               }
               else if(code0 == imagpart_expr_K)
               {
                  ga->op0 = manage_imagpart(ga->op0, tree_nodeRef());
                  restart_analysis = true;
               }
               else if(reached_max_transformation_limit(*it_los))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement " + (*it_los)->ToString());
                  it_los++;
                  continue;
               }
               if(code0 == bit_field_ref_K)
               {
                  auto bit_field_ref0 = [&] {
                     auto* bfr = GetPointer<bit_field_ref>(ga->op0);
                     THROW_ASSERT(bfr->op0->get_kind() != ssa_name_K, "unexpected pattern");
                     THROW_ASSERT(tree_helper::GetConstValue(bfr->op2) >= 0, "");
                     auto op1_val = static_cast<unsigned int>(tree_helper::GetConstValue(bfr->op2));
                     auto right_shift_val = op1_val % 8;
                     tree_nodeRef type;
                     if(bfr->type->get_kind() == boolean_type_K)
                     {
                        type = tree_man->GetCustomIntegerType(std::max(8u, ceil_pow2(1 + right_shift_val)), true);
                     }
                     else
                     {
                        auto* it = GetPointer<integer_type>(bfr->type);
                        THROW_ASSERT(it, "unexpected pattern");
                        type = tree_man->GetCustomIntegerType(std::max(8u, ceil_pow2(it->prec + right_shift_val)),
                                                              it->unsigned_flag);
                     }
                     tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                     tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                     tree_nodeRef new_ga = tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae,
                                                                        function_id, srcp_default);
                     GetPointer<gimple_assign>(new_ga)->temporary_address = true;

                     tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     AppM->RegisterTransformation(GetName(), new_ga);

                     tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                     auto size_tp = tree_helper::Size(type);
                     auto size_field_decl = tree_helper::Size(bfr->op1);

                     if(right_shift_val == 0 and size_field_decl == size_tp)
                     {
                        ga->op0 = mr;
                     }
                     else
                     {
                        tree_nodeRef mr_dup =
                            tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);
                        tree_nodeRef load_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                            mr_dup, function_id, srcp_default);
                        tree_nodeRef load_vd = GetPointer<gimple_assign>(load_ga)->op0;
                        GetPointer<gimple_assign>(load_ga)->memuse = ga->memuse;
                        GetPointer<gimple_assign>(load_ga)->vuses = ga->vuses;
                        for(const auto& vd : bitfield_vuses)
                        {
                           GetPointer<gimple_assign>(load_ga)->AddVuse(vd);
                        }
                        THROW_ASSERT(ga->vdef, "unexpected condition");
                        bitfield_vdefs.insert(ga->vdef);
                        block.second->PushBefore(load_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + load_ga->ToString());

                        tree_nodeRef load_var;
                        if(size_field_decl != size_tp)
                        {
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned loading
                            * the bitfield must not be checked, because it has
                            * not been resized yet and it may also load
                            * inconsistent stuff from other bitfields nearby
                            */
                           if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy) and
                              (not parameters->isOption(OPT_discrepancy_force) or
                               not parameters->getOption<bool>(OPT_discrepancy_force)))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---skip discrepancy of ssa " + load_vd->ToString() +
                                                 " assigned in new stmt: " + load_ga->ToString());
                              AppM->RDiscr->ssa_to_skip.insert(load_vd);
                           }
#endif
                           tree_nodeRef nmask_value = TM->CreateUniqueIntegerCst(
                               static_cast<long long int>(~(((1ULL << size_field_decl) - 1) << right_shift_val)), type);
                           tree_nodeRef nmask_node = tree_man->create_binary_operation(type, load_vd, nmask_value,
                                                                                       srcp_default, bit_and_expr_K);
                           tree_nodeRef nmask_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                                nmask_node, function_id, srcp_default);
                           block.second->PushBefore(nmask_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + nmask_ga->ToString());
                           load_var = GetPointer<gimple_assign>(nmask_ga)->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned loading
                            * the bitfield must not be checked, because it has
                            * not been resized yet and it may also load
                            * inconsistent stuff from other bitfields nearby
                            */
                           if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy) and
                              (not parameters->isOption(OPT_discrepancy_force) or
                               not parameters->getOption<bool>(OPT_discrepancy_force)))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---skip discrepancy of ssa " + load_var->ToString() +
                                                 " assigned in new stmt: " + nmask_ga->ToString());
                              AppM->RDiscr->ssa_to_skip.insert(load_var);
                           }
#endif
                        }
                        else
                        {
                           THROW_ASSERT(right_shift_val == 0, "unexpected pattern");
                           load_var = load_vd;
                        }
                        tree_nodeRef written_var;
                        if(size_field_decl != size_tp)
                        {
                           tree_nodeRef and_mask_value = TM->CreateUniqueIntegerCst(
                               static_cast<long long int>((1ULL << size_field_decl) - 1), type);
                           tree_nodeRef and_mask_node = tree_man->create_binary_operation(type, ga->op1, and_mask_value,
                                                                                          srcp_default, bit_and_expr_K);
                           tree_nodeRef and_mask_ga = tree_man->CreateGimpleAssign(
                               type, tree_nodeRef(), tree_nodeRef(), and_mask_node, function_id, srcp_default);
                           block.second->PushBefore(and_mask_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + and_mask_ga->ToString());
                           written_var = GetPointer<gimple_assign>(and_mask_ga)->op0;
                        }
                        else
                        {
                           written_var = ga->op1;
                        }
                        tree_nodeRef lshifted_var;
                        if(right_shift_val)
                        {
                           tree_nodeRef lsh_value = TM->CreateUniqueIntegerCst(right_shift_val, type);
                           tree_nodeRef lsh_node = tree_man->create_binary_operation(type, written_var, lsh_value,
                                                                                     srcp_default, lshift_expr_K);
                           tree_nodeRef lsh_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                              lsh_node, function_id, srcp_default);
                           block.second->PushBefore(lsh_ga, *it_los, AppM);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---adding statement " + lsh_ga->ToString());
                           lshifted_var = GetPointer<gimple_assign>(lsh_ga)->op0;
                        }
                        else
                        {
                           lshifted_var = written_var;
                        }

                        tree_nodeRef res_node = tree_man->create_binary_operation(type, lshifted_var, load_var,
                                                                                  srcp_default, bit_ior_expr_K);
                        tree_nodeRef res_ga = tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(),
                                                                           res_node, function_id, srcp_default);
                        block.second->PushBefore(res_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + res_ga->ToString());
                        tree_nodeRef res_vd = GetPointer<gimple_assign>(res_ga)->op0;
                        ga->op1 = res_vd;
                        ga->op0 = mr;
#if HAVE_FROM_DISCREPANCY_BUILT
                        /*
                         * for discrepancy analysis, the ssa assigned loading
                         * the bitfield must not be checked, because it has
                         * not been resized yet and it may also load
                         * inconsistent stuff from other bitfields nearby
                         */
                        if(size_field_decl != size_tp and parameters->isOption(OPT_discrepancy) and
                           parameters->getOption<bool>(OPT_discrepancy) and
                           (not parameters->isOption(OPT_discrepancy_force) or
                            not parameters->getOption<bool>(OPT_discrepancy_force)))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---skip discrepancy of ssa " + res_vd->ToString() +
                                              " assigned in new stmt: " + res_ga->ToString());
                           AppM->RDiscr->ssa_to_skip.insert(res_vd);
                        }
#endif
                     }
                     restart_analysis = true;
                  };
                  bit_field_ref0();
               }
               else if(code0 == parm_decl_K)
               {
                  auto* pd = GetPointer<parm_decl>(ga->op0);
                  tree_nodeRef type = pd->type;
                  tree_nodeRef pt = tree_man->GetPointerType(type, 8);
                  tree_nodeRef ae = tree_man->create_unary_operation(pt, ga->op0, srcp_default, addr_expr_K);
                  tree_nodeRef new_ga =
                      tree_man->CreateGimpleAssign(pt, tree_nodeRef(), tree_nodeRef(), ae, function_id, srcp_default);
                  GetPointer<gimple_assign>(new_ga)->temporary_address = true;
                  tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;

                  tree_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                  tree_nodeRef mr = tree_man->create_binary_operation(type, ssa_vd, offset, srcp_default, mem_ref_K);

                  ga->op0 = mr;
                  block.second->PushBefore(new_ga, *it_los, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + new_ga->ToString());
                  AppM->RegisterTransformation(GetName(), new_ga);
                  restart_analysis = true;
               }
               else if(code0 == result_decl_K && code1 == ssa_name_K)
               {
                  const auto type_node = tree_helper::CGetType(ga->op1);
                  if(type_node->get_kind() == complex_type_K || type_node->get_kind() == vector_type_K)
                  {
                     auto it_los_next = std::next(it_los);
                     if(it_los_next != it_los_end && (*it_los_next)->get_kind() == gimple_return_K)
                     {
                        auto* gr = GetPointer<gimple_return>(*it_los_next);
                        gr->op = ga->op1;
                        block.second->RemoveStmt(*it_los, AppM);
                        it_los = list_of_stmt.begin();
                        it_los_end = list_of_stmt.end();
                        bitfield_vuses.clear();
                        bitfield_vdefs.clear();
                     }
                  }
               }
               else if(code0 == result_decl_K)
               {
                  auto type_index = tree_helper::get_type_index(TM, ga->op0->index);
                  tree_nodeRef op_type = TM->GetTreeNode(type_index);
                  tree_nodeRef new_ga = tree_man->CreateGimpleAssign(op_type, tree_nodeRef(), tree_nodeRef(), ga->op1,
                                                                     function_id, srcp_default);
                  tree_nodeRef ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                  ga->op1 = ssa_vd;
                  block.second->PushBefore(new_ga, *it_los, AppM);
                  AppM->RegisterTransformation(GetName(), new_ga);
                  restart_analysis = true;
               }
            }
            else if((*it_los)->get_kind() == gimple_cond_K)
            {
               if(reached_max_transformation_limit(*it_los))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement " + (*it_los)->ToString());
                  it_los++;
                  continue;
               }
               auto gc0 = [&] {
                  auto* gc = GetPointer<gimple_cond>(*it_los);
                  /// manage something as binary operand
                  if(GetPointer<binary_expr>(gc->op0))
                  {
                     auto be = GetPointer<binary_expr>(gc->op0);
                     const auto be_kind = be->get_kind();
                     if(be_kind == eq_expr_K || be_kind == ne_expr_K || be_kind == lt_expr_K || be_kind == le_expr_K ||
                        be_kind == gt_expr_K || be_kind == ge_expr_K || be_kind == ltgt_expr_K ||
                        be_kind == truth_and_expr_K || be_kind == truth_andif_expr_K || be_kind == truth_or_expr_K ||
                        be_kind == truth_orif_expr_K || be_kind == truth_xor_expr_K)
                     {
                        be->type = tree_man->GetBooleanType();
                     }
                     bool changed = false;
                     if(be->op0->get_kind() == addr_expr_K)
                     {
                        extract_unary_expr(be->op0, true, false);
                     }
                     if(be->op1->get_kind() == addr_expr_K)
                     {
                        extract_unary_expr(be->op1, true, false);
                     }
                     if(GetPointer<unary_expr>(be->op0) || GetPointer<binary_expr>(be->op0) ||
                        GetPointer<ternary_expr>(be->op0)) /// required by the CLANG/LLVM plugin
                     {
                        tree_nodeRef type;
                        if(GetPointer<unary_expr>(be->op0))
                        {
                           type = GetPointer<unary_expr>(be->op0)->type;
                        }
                        else if(GetPointer<binary_expr>(be->op0))
                        {
                           type = GetPointer<binary_expr>(be->op0)->type;
                        }
                        else if(GetPointer<ternary_expr>(be->op0))
                        {
                           type = GetPointer<ternary_expr>(be->op0)->type;
                        }
                        else
                        {
                           THROW_ERROR("not managed condition");
                        }

                        const auto new_ga =
                            tree_man->CreateGimpleAssign(type, nullptr, nullptr, be->op0, function_id, srcp_default);
                        const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                        be->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        changed = true;
                     }
                     if(GetPointer<unary_expr>(be->op1) || GetPointer<binary_expr>(be->op1) ||
                        GetPointer<ternary_expr>(be->op1)) /// required by the CLANG/LLVM plugin
                     {
                        tree_nodeRef type;
                        if(GetPointer<unary_expr>(be->op1))
                        {
                           type = GetPointer<unary_expr>(be->op1)->type;
                        }
                        else if(GetPointer<binary_expr>(be->op1))
                        {
                           type = GetPointer<binary_expr>(be->op1)->type;
                        }
                        else if(GetPointer<ternary_expr>(be->op1))
                        {
                           type = GetPointer<ternary_expr>(be->op1)->type;
                        }
                        else
                        {
                           THROW_ERROR("not managed condition");
                        }
                        const auto new_ga =
                            tree_man->CreateGimpleAssign(type, nullptr, nullptr, be->op1, function_id, srcp_default);
                        const auto ssa_vd = GetPointer<gimple_assign>(new_ga)->op0;
                        be->op1 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        changed = true;
                     }
                     if(changed)
                     {
                        restart_analysis = true;
                     }
                  }
                  else if(tree_helper::Size(gc->op0) != 1)
                  {
                     const auto bt = tree_man->GetBooleanType();
                     const auto ne = tree_man->create_unary_operation(bt, gc->op0, srcp_default, nop_expr_K);
                     const auto nga =
                         tree_man->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                      TM->CreateUniqueIntegerCst(1, bt), ne, function_id, srcp_default);
                     const auto n_vd = GetPointer<gimple_assign>(nga)->op0;
                     block.second->PushBefore(nga, *it_los, AppM);
                     gc->op0 = n_vd;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + nga->ToString());
                     restart_analysis = true;
                  }
                  /// optimize less than 0 or greater than or equal than 0
                  if(gc->op0->get_kind() == lt_expr_K || gc->op0->get_kind() == ge_expr_K)
                  {
                     const auto le = GetPointer<binary_expr>(gc->op0);
                     if(tree_helper::IsSignedIntegerType(le->op0))
                     {
                        if(le->op1->get_kind() == integer_cst_K)
                        {
                           if(tree_helper::GetConstValue(le->op1) == 0)
                           {
                              const auto cond_op_ga = tree_man->CreateGimpleAssign(le->type, nullptr, nullptr, gc->op0,
                                                                                   function_id, srcp_default);
                              block.second->PushBefore(cond_op_ga, *it_los, AppM);
                              const auto cond_ga_var = GetPointerS<gimple_assign>(cond_op_ga)->op0;
                              AppM->RegisterTransformation(GetName(), cond_op_ga);
                              gc->op0 = cond_ga_var;
                              restart_analysis = true;
                           }
                        }
                     }
                  }
               };
               gc0();
            }
            else if((*it_los)->get_kind() == gimple_call_K)
            {
               auto* gc = GetPointer<gimple_call>(*it_los);
               for(auto& arg : gc->args)
               {
                  if(GetPointer<unary_expr>(arg) || GetPointer<binary_expr>(arg)) /// required by the CLANG/LLVM plugin
                  {
                     extract_unary_expr(arg, arg->get_kind() == addr_expr_K, false);
                  }
               }
            }
            else if((*it_los)->get_kind() == gimple_return_K)
            {
               auto* gr = GetPointer<gimple_return>(*it_los);
               if(gr->op)
               {
                  if(GetPointer<unary_expr>(gr->op) || GetPointer<binary_expr>(gr->op))
                  {
                     extract_expr(gr->op, false);
                  }
               }
               else
               {
                  const auto ret_type = tree_helper::GetFunctionReturnType(tn);
                  if(ret_type)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                    "Control reaches end of non-void function, '" +
                                        tree_helper::print_function_name(TM, fd) + "' will return zero");
                     gr->op = TM->CreateUniqueIntegerCst(0LL, ret_type);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + (*it_los)->ToString());
            it_los++;
         }
      } while(restart_analysis);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
   }
   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
