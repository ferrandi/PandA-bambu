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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file Range_Analysis.cpp
 * @brief 
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Range_Analysis.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// stl
#include <map>
#include <vector>

/// Tree includes
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "ext_tree_node.hpp"
#include "tree_reindex.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

namespace RangeAnalysis
{
   using namespace boost::multiprecision;

   using UAPInt = uint128_t;

   namespace 
   {
      // The number of bits needed to store the largest variable of the function (llvm::APInt).
      unsigned MAX_BIT_INT = 0;
      unsigned POINTER_BITSIZE = 32;

      // ========================================================================== //
      // Static global functions and definitions
      // ========================================================================== //

      APInt Min, Max, Zero, One;

      APInt getMaxValue(unsigned bitwidth)
      {
         return APInt((uint256_t(1) << (bitwidth + 1)) - 1);
      }

      APInt getMinValue(unsigned bitwidth)
      {
         return APInt(0);
      }

      APInt getSignedMaxValue(unsigned bitwidth)
      {
         return (APInt(1) << (bitwidth - 1)) - 1;
      }

      APInt getSignedMinValue(unsigned bitwidth)
      {
         return -(APInt(1) << (bitwidth - 1));
      }

      inline APInt ap_trunc(APInt x, unsigned bitwidth)
      {
         bitwidth = (1 << bitwidth) - 1;
         return x & bitwidth;
      }

      APInt lshr(const APInt& a, unsigned p)
      {
         APInt mask = (APInt(1) << (128 - p)) - 1;
         return (a >> p) & mask;
      }

      UAPInt lshr(const UAPInt& a, unsigned p)
      {
         if(p > 128)
         {
            return 0;
         }
         UAPInt mask = (UAPInt(1) << (128 - p)) - 1;
         return (a >> p) & mask;
      }

      unsigned countLeadingZeros(const APInt& a)
      {
         int i = 127;
         for(; i >= 0; --i)
         {
            if(bit_test(a, i))
            {
                  break;
            }
         }
         i++;
         return 128 - i;
      }

      unsigned countLeadingOnes(const APInt& a)
      {
         int i = 127;
         for(; i >= 0; --i)
         {
            if(!bit_test(a, i))
            {
                  break;
            }
         }
         i++;
         return 128 - i;
      }

   }

   // ========================================================================== //
   // Range
   // ========================================================================== //
   Range_base::Range_base(RangeType rType, unsigned rbw) : l(Min), u(Max), bw(rbw), type(rType)
   {
      assert(rbw);
   }

   void Range_base::normalizeRange(const APInt& lb, const APInt& ub, RangeType rType)
   {
      if(rType == Empty || rType == Unknown)
      {
         l = Min;
         u = Max;
      }
      else if(rType == Anti)
      {
         if(lb > ub)
         {
            type = Regular;
            l = Min;
            u = Max;
         }
         else
         {
            if((lb == Min) && (ub == Max))
            {
               type = Empty;
            }
            else if(lb == Min)
            {
               type = Regular;
               l = ub + 1;
               u = Max;
            }
            else if(ub == Max)
            {
               type = Regular;
               l = Min;
               u = lb - 1;
            }
            else
            {
               assert(ub >= lb);
               auto maxS = getSignedMaxValue(bw);
               auto minS = getSignedMinValue(bw);
               bool lbgt = lb >maxS;
               bool ubgt = ub > maxS;
               bool lblt = lb < minS;
               bool ublt = ub < minS;
               if(lbgt && ubgt)
               {
                  l = ap_trunc(lb, bw);
                  u = ap_trunc(ub, bw);
               }
               else if(lblt && ublt)
               {
                  l = ap_trunc(ub, bw);
                  u = ap_trunc(lb, bw);
               }
               else if(!lblt && ubgt)
               {
                  auto ubnew = ap_trunc(ub, bw);
                  if(ubnew >= (lb - 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ubnew + 1;
                     u = lb - 1;
                  }
               }
               else if(lblt && !ubgt)
               {
                  auto lbnew = ap_trunc(lb, bw);
                  if(lbnew <= (ub + 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ub + 1;
                     u = lbnew - 1;
                  }
               }
               else if(!lblt && !ubgt)
               {
                  l = lb;
                  u = ub;
               }
               else
               {
                  THROW_UNREACHABLE("unexpected condition");
               }
            }
         }
      }
      else if((lb - 1) == ub)
      {
         type = Regular;
         l = Min;
         u = Max;
      }
      else if(lb > ub)
      {
         normalizeRange(ub + 1, lb - 1, Anti);
      }
      else
      {
         assert(ub >= lb);
         auto maxS = getSignedMaxValue(bw);
         auto minS = getSignedMinValue(bw);

         bool lbgt = lb > maxS;
         bool ubgt = ub > maxS;
         bool lblt = lb < minS;
         bool ublt = ub < minS;
         if(ubgt && lblt)
         {
            l = Min;
            u = Max;
         }
         else if(lbgt && ubgt)
         {
            l = ap_trunc(lb, bw);
            u = ap_trunc(ub, bw);
         }
         else if(lblt && ublt)
         {
            l = ap_trunc(ub, bw);
            u = ap_trunc(lb, bw);
         }
         else if(!lblt && ubgt)
         {
            auto ubnew = ap_trunc(ub, bw);
            if(ubnew >= (lb - 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ubnew + 1;
               u = lb - 1;
            }
         }
         else if(lblt && !ubgt)
         {
            auto lbnew = ap_trunc(lb, bw);
            if(lbnew <= (ub + 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ub + 1;
               u = lbnew - 1;
            }
         }
         else if(!lblt && !ubgt)
         {
            l = lb;
            u = ub;
         }
         else
         {
            THROW_UNREACHABLE("unexpected condition");
         }
      }
      if(!(u >= l))
      {
         l = Min;
         u = Max;
      }
   }

   unsigned Range_base::neededBits(const APInt& a, const APInt& b, bool sign)
   {
      auto max_active_bits = [](const APInt& x, const APInt& y)
      {
         size_t i = 128;
         while(i > 0)
         {
            --i;
            if(bit_test(x, i) || bit_test(y, i))
            {
               break;
            }
         }

         return i;
      };

      if(sign)
      {
         auto a_u = a.sign() == -1 ? -a : a;
         auto b_u = b.sign() == -1 ? -b : b;

         return max_active_bits(a_u, b_u);
      }

      return max_active_bits(a, b);
   }

   Range_base::Range_base(RangeType rType, unsigned rbw, const APInt& lb, const APInt& ub) : l(lb), u(ub), bw(rbw), type(rType)
   {
      assert(rbw);
      normalizeRange(lb, ub, rType);
   }

   Range_base Range_base::getAnti(const Range_base& o)
   {
      if(o.type == Anti)
      {
         return Range_base(Regular, o.bw, o.l, o.u);
      }
      if(o.type == Regular)
      {
         return Range_base(Anti, o.bw, o.l, o.u);
      }
      if(o.type == Empty)
      {
         return Range_base(Regular, o.bw, Min, Max);
      }
      if(o.type == Unknown)
      {
         return o;
      }

      THROW_UNREACHABLE("unexpected condition");
   }

   unsigned int Range_base::getBitWidth() const
   {
      return bw;
   }

   const APInt Range_base::getLower() const
   {
      assert(type != Anti);
      return l;
   }

   const APInt Range_base::getUpper() const
   {
      assert(type != Anti);
      return u;
   }

   const APInt Range_base::getSignedMax() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         auto maxS = getSignedMaxValue(bw);
         if(u > maxS)
         {
            return maxS;
         }

         return ap_trunc(u, bw);
      }

      return getSignedMaxValue(bw);
   }
   const APInt Range_base::getSignedMin() const
   {
      assert(type != Unknown && type != Empty);
      if(type != Anti)
      {
         auto minS = getSignedMinValue(bw);
         if(l < minS)
         {
            return minS;
         }

         return ap_trunc(l, bw);
      }

      return getSignedMinValue(bw);
   }
   const APInt Range_base::getUnsignedMax() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         if((l.sign() == -1) && (u.sign() == -1))
         {
            return ap_trunc(u, bw);
         }
         if(l < 0)
         {
            return getMaxValue(bw);
         }

         return ap_trunc(u, bw);
      }

      auto lb = ap_trunc(l, bw) - 1;
      auto ub = ap_trunc(u, bw) + 1;

      if((lb >= 0) || (ub < 0))
      {
         return getMaxValue(bw);
      }

      return lb;
   }
   const APInt Range_base::getUnsignedMin() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         if((l.sign() == -1) && (u.sign() == -1))
         {
            return ap_trunc(l, bw);
         }
         if(l < 0)
         {
            return 0;
         }

         return ap_trunc(l, bw);
      }

      auto lb = ap_trunc(l, bw) - 1;
      auto ub = ap_trunc(u, bw) + 1;

      if((lb >= 0) || (ub < 0))
      {
         return 0;
      }

      return ub;
   }

   bool Range::isFullSet() const
   {
      if(isAnti())
      {
         return false;
      }
      unsigned bw = getBitWidth();
      return (getSignedMaxValue(bw) <= getUpper() && getSignedMinValue(bw) >= getLower())
         || (getMaxValue(bw) <= getUpper() && getMinValue(bw) >= getLower());
   }

   bool Range::isMaxRange() const
   {
      if(isAnti())
      {
         return false;
      }
      return (this->getLower() == Min) && (this->getUpper() == Max);
   }
   bool Range::isConstant() const
   {
      if(isAnti())
      {
         return false;
      }
      return this->getLower() == this->getUpper();
   }

   /// Add and Mul are commutative. So, they are a little different
   /// than the other operations.
   /// Many Range reductions are done by exploiting ConstantRange code
   Range Range::add(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(ol >= (Max - u))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l + ol, u + ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l == Min)
         {
            assert(u != Max);
            return Range(Regular, getBitWidth(), l, u + ol);
         }
         if(u == Max)
         {
            assert(l != Min);
            return Range(Regular, getBitWidth(), l + ol, u);
         }

         return Range(Regular, getBitWidth(), l + ol, u + ol);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      Range thisR = (this_min == (this_max + 1)) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      Range thisUR = (thisU_min == thisU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), thisU_min, thisU_max + 1);
      Range otherUR = (otherU_min == otherU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), otherU_min, otherU_max + 1);
      //ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      //ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto AddU = Range(Regular, getBitWidth(), thisUR.getLower() + otherUR.getLower(), thisUR.getUpper() + otherUR.getUpper());
      auto AddS = Range(Regular, getBitWidth(), thisR.getLower() + otherR.getLower(), thisR.getUpper() + otherR.getUpper());
      
      return BestRange(AddU, AddS, getBitWidth());
   }

   Range Range::sub(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(l <= (Min + ol))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l - ol, u - ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l == Min)
         {
            assert(u != Max);
            auto bw = getBitWidth();
            auto minValue = getSignedMinValue(bw);
            auto upper = u - ol;
            if(minValue > upper)
            {
               return Range(Regular, bw);
            }

            return Range(Regular, bw, l, upper);
         }
         if(u == Max)
         {
            assert(l != Min);
            auto bw = getBitWidth();
            auto maxValue = getSignedMaxValue(bw);
            auto lower = l - ol;
            if(maxValue < lower)
            {
               return Range(Regular, bw);
            }

            return Range(Regular, getBitWidth(), l - ol, u);
         }

         auto bw = getBitWidth();
         auto lower = ap_trunc(l - ol, bw);
         auto upper = ap_trunc(u - ol, bw);
         if(lower <= upper)
         {
            return Range(Regular, bw, lower, upper);
         }

         return Range(Anti, bw, upper + 1, lower - 1);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      Range thisR = (this_min == (this_max + 1)) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      Range thisUR = (thisU_min == thisU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), thisU_min, thisU_max + 1);
      Range otherUR = (otherU_min == otherU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), otherU_min, otherU_max + 1);
      //ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      //ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto SubU = Range(Regular, getBitWidth(), thisUR.getLower() - otherUR.getUpper(), thisUR.getUpper() - otherUR.getLower());
      auto SubS = Range(Regular, getBitWidth(), thisR.getLower() - otherR.getUpper(), thisR.getUpper() - otherR.getLower());

      return BestRange(SubU, SubS, getBitWidth());
   }

   Range Range::mul(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      // Multiplication is signedness-independent. However different ranges can be
      // obtained depending on how the input ranges are treated. These different
      // ranges are all conservatively correct, but one might be better than the
      // other. We calculate two ranges; one treating the inputs as unsigned
      // and the other signed, then return the smallest of these ranges.

      // Unsigned range first.
      APInt this_min = getUnsignedMin();
      APInt this_max = getUnsignedMax();
      APInt Other_min = other.getUnsignedMin();
      APInt Other_max = other.getUnsignedMax();

      Range Result_zext = Range(Regular, getBitWidth() * 2, this_min * Other_min, this_max * Other_max + 1);
      Range UR = Result_zext.truncate(getBitWidth());

      // Now the signed range. Because we could be dealing with negative numbers
      // here, the lower bound is the smallest of the Cartesian product of the
      // lower and upper ranges; for example:
      //   [-1,4) * [-2,3) = min(-1*-2, -1*2, 3*-2, 3*2) = -6.
      // Similarly for the upper bound, swapping min for max.

      this_min = getSignedMin();
      this_max = getSignedMax();
      Other_min = other.getSignedMin();
      Other_max = other.getSignedMax();

      auto L = {this_min * Other_min, this_min * Other_max, this_max * Other_min, this_max * Other_max};
      auto Compare = [](const APInt& A, const APInt& B) { return A < B; };
      Range Result_sext(Regular, getBitWidth() * 2, std::min(L, Compare), std::max(L, Compare) + 1);
      Range SR = Result_sext.truncate(getBitWidth());
      return BestRange(UR, SR, getBitWidth());
   }

#define DIV_HELPER(x, y) (x == Max) ? ((y < 0) ? Min : ((y == 0) ? 0 : Max)) : ((y == Max) ? 0 : ((x == Min) ? ((y < 0) ? Max : ((y == 0) ? 0 : Min)) : ((y == Min) ? 0 : ((x) / (y)))))

   Range Range::udiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      //      llvm::errs() << "this:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      UAPInt a(getUnsignedMin());
      UAPInt b(getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());

      // Deal with division by 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c == 0)
      {
         c = 1;
      }

      APInt candidates[4];
      candidates[0] = DIV_HELPER(a, c);
      candidates[1] = DIV_HELPER(a, d);
      candidates[2] = DIV_HELPER(b, c);
      candidates[3] = DIV_HELPER(b, d);
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
   }

   Range Range::sdiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c1, d1, c2, d2;
      bool is_zero_in = false;
      if(other.isAnti())
      {
         auto antiRange = getAnti(other);
         c1 = Min;
         d1 = antiRange.getLower() - 1;
         if(d1 == 0)
         {
            d1 = -One;
         }
         else if(d1 > 0)
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         c2 = antiRange.getUpper() + 1;
         if(c2 == 0)
         {
            c2 = One;
         }
         else if(c2 < 0)
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         d2 = Max;
      }
      else
      {
         c1 = other.getLower();
         d1 = other.getUpper();
         // Deal with division by 0 exception
         if((c1 == 0) && (d1 == 0))
         {
            return Range(Regular, getBitWidth(), Min, Max);
         }
         is_zero_in = (c1 < 0) && (d1 > 0);
         if(is_zero_in)
         {
            d1 = -One;
            c2 = One;
         }
         else
         {
            c2 = other.getLower();
            if(c2 == 0)
            {
               c1 = c2 = One;
            }
         }
         d2 = other.getUpper();
         if(d2 == 0)
         {
            d1 = d2 = -One;
         }
      }
      auto n_iters = (is_zero_in || other.isAnti()) ? 8u : 4u;

      APInt candidates[8];
      candidates[0] = DIV_HELPER(a, c1);
      candidates[1] = DIV_HELPER(a, d1);
      candidates[2] = DIV_HELPER(b, c1);
      candidates[3] = DIV_HELPER(b, d1);
      if(n_iters == 8)
      {
         candidates[4] = DIV_HELPER(a, c2);
         candidates[5] = DIV_HELPER(a, d2);
         candidates[6] = DIV_HELPER(b, c2);
         candidates[7] = DIV_HELPER(b, d2);
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < n_iters; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
      ;
   }

   Range Range::urem(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getUnsignedMin();
      const APInt& b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      const APInt& d = other.getUnsignedMax();

      // Deal with mod 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth());
      }
      if(c == 0)
      {
         c = 1;
      }
      //      llvm::errs() << "this-urem:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-urem:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      APInt candidates[8];

      candidates[0] = a < c ? a : 0;
      candidates[1] = a < d ? a : 0;
      candidates[2] = b < c ? b : 0;
      candidates[3] = b < d ? b : 0;
      candidates[4] = a < c ? a : c - 1;
      candidates[5] = a < d ? a : d - 1;
      candidates[6] = b < c ? b : c - 1;
      candidates[7] = b < d ? b : d - 1;

      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 8; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      //      llvm::errs() << "res-urem:";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   Range Range::srem(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other == Range(Regular, other.getBitWidth(), 0, 0))
      {
         return Range(Empty, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c = other.getLower();
      const APInt& d = other.getUpper();

      // Deal with mod 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c == 0)
      {
         c = 1;
      }

      //      llvm::errs() << "this-rem:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-rem:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      APInt candidates[4];
      candidates[0] = Min;
      candidates[1] = Min;
      candidates[2] = Max;
      candidates[3] = Max;
      if((a != Min) && (c != Min))
      {
         candidates[0] = a & c; // lower lower
      }
      if((a != Min) && (d != Max))
      {
         candidates[1] = a % d; // lower upper
      }
      if((b != Max) && (c != Min))
      {
         candidates[2] = b % c; // upper lower
      }
      if((b != Max) && (d != Max))
      {
         candidates[3] = b % d; // upper upper
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      //      llvm::errs() << "res-rem:";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";

      return res;
   }

   // Logic has been borrowed from ConstantRange
   Range Range::shl(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      auto bw = getBitWidth();
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, bw);
      }

      //      llvm::errs() << "this-shl:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-shl:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      const UAPInt c(other.getUnsignedMin());
      const UAPInt d(other.getUnsignedMax());
      UAPInt Prec = bw;

      if(c >= Prec)
      {
         return Range(Regular, bw);
      }
      if(d >= Prec)
      {
         return Range(Regular, bw);
      }
      if((a == Min) || (b == Max))
      {
         return Range(Regular, bw);
      }
      if((a == b) && (c == d)) // constant case
      {
         auto minmax = ap_trunc(a, bw) << ap_trunc(c, bw).convert_to<size_t>();
         minmax = ap_trunc(minmax, MAX_BIT_INT);
         return Range(Regular, bw, minmax, minmax);
      }
      if((a.sign() == -1) && (b.sign() == -1))
      {
         UAPInt clOnes(countLeadingOnes(a) - (MAX_BIT_INT - getBitWidth()));
         if(d > clOnes)
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a << d.convert_to<size_t>(), b << c.convert_to<size_t>());
      }
      if((a < 0) && (b >= 0))
      {
         UAPInt clOnes(countLeadingOnes(a) - (MAX_BIT_INT - getBitWidth()));
         UAPInt clZeros(countLeadingZeros(b) - (MAX_BIT_INT - getBitWidth()));
         if(d > clOnes || d > clZeros)
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a << d.convert_to<size_t>(), b << d.convert_to<size_t>());
      }

      UAPInt clZeros(countLeadingZeros(b) - (MAX_BIT_INT - getBitWidth()));
      if(d > clZeros)
      { // overflow
         return Range(Regular, bw);
      }

      return Range(Regular, getBitWidth(), a << c.convert_to<size_t>(), b << d.convert_to<size_t>());
   }

   Range Range::lshr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      UAPInt this_min(getUnsignedMin());
      UAPInt this_max(getUnsignedMax());
      UAPInt other_min(other.getUnsignedMin());
      UAPInt other_max(other.getUnsignedMax());
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      const Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      // TODO: check correctness
      auto lshrU = Range(Regular, getBitWidth(), 
         RangeAnalysis::lshr(thisR.getLower(), otherR.getUpper().convert_to<unsigned>()), 
         RangeAnalysis::lshr(thisR.getUpper(), otherR.getLower().convert_to<unsigned>()));
      if(lshrU.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }

      return lshrU;
   }

   static Range local_ashr(const Range& lthis, const Range& Other)
   {
      if(lthis.isEmpty() || Other.isEmpty())
      {
         return Range(Empty, lthis.getBitWidth());
      }

      // May straddle zero, so handle both positive and negative cases.
      // 'PosMax' is the upper bound of the result of the ashr
      // operation, when Upper of the LHS of ashr is a non-negative.
      // number. Since ashr of a non-negative number will result in a
      // smaller number, the Upper value of LHS is shifted right with
      // the minimum value of 'Other' instead of the maximum value.
      APInt PosMax = lthis.getSignedMax() >> (Other.getUnsignedMin().convert_to<unsigned>() + 1);

      // 'PosMin' is the lower bound of the result of the ashr
      // operation, when Lower of the LHS is a non-negative number.
      // Since ashr of a non-negative number will result in a smaller
      // number, the Lower value of LHS is shifted right with the
      // maximum value of 'Other'.
      APInt PosMin = lthis.getSignedMin() >> (Other.getUnsignedMax().convert_to<unsigned>());

      // 'NegMax' is the upper bound of the result of the ashr
      // operation, when Upper of the LHS of ashr is a negative number.
      // Since 'ashr' of a negative number will result in a bigger
      // number, the Upper value of LHS is shifted right with the
      // maximum value of 'Other'.
      APInt NegMax = lthis.getSignedMax() >> (Other.getUnsignedMax().convert_to<unsigned>() + 1);

      // 'NegMin' is the lower bound of the result of the ashr
      // operation, when Lower of the LHS of ashr is a negative number.
      // Since 'ashr' of a negative number will result in a bigger
      // number, the Lower value of LHS is shifted right with the
      // minimum value of 'Other'.
      APInt NegMin = lthis.getSignedMin() >> (Other.getUnsignedMin().convert_to<unsigned>());

      APInt max, min;
      if(lthis.getSignedMin() >= 0)
      {
         // Upper and Lower of LHS are non-negative.
         min = PosMin;
         max = PosMax;
      }
      else if(lthis.getSignedMax() < 0)
      {
         // Upper and Lower of LHS are negative.
         min = NegMin;
         max = NegMax;
      }
      else
      {
         // Upper is non-negative and Lower is negative.
         min = NegMin;
         max = PosMax;
      }
      if(min == max)
      {
         return Range(Regular, lthis.getBitWidth());
      }

      return Range(Regular, lthis.getBitWidth(), std::move(min), std::move(max));
   }

   Range Range::ashr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      UAPInt other_min(other.getUnsignedMin());
      UAPInt other_max(other.getUnsignedMax());
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      const Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);
      auto AshrU = local_ashr(thisR, otherR);

      if(AshrU.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }
      return AshrU;
   }

   /*
    * 	This and operation is coded following Hacker's Delight algorithm.
    * 	According to the author, it provides tight results.
    */
   Range Range::And(const Range& other) const
   {
      //      llvm::errs() << "And-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "And-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      APInt a = this->isAnti() ? Min : this->getLower();
      APInt b = this->isAnti() ? Max : this->getUpper();
      APInt c = other.isAnti() ? Min : other.getLower();
      APInt d = other.isAnti() ? Max : other.getUpper();

      // negate everybody
      APInt negA = ~APInt(a);
      APInt negB = ~APInt(b);
      APInt negC = ~APInt(c);
      APInt negD = ~APInt(d);
      auto bw = getBitWidth();

      Range inv1 = Range(Regular, bw, negB, negA);
      Range inv2 = Range(Regular, bw, negD, negC);
      Range invres = inv1.Or(inv2);

      // negate the result of the 'or'
      APInt invLower = ~invres.getUpper();
      APInt invUpper = ~invres.getLower();
      auto res = Range(Regular, bw, invLower, invUpper);
      //      llvm::errs() << "And-res: ";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   namespace
   {
      APInt minOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         UAPInt ub(b), ud(d), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((~a & c & m) != 0)
            {
               temp = UAPInt((a | m) & -m);
               if(temp <= ub)
               {
                  a = temp;
                  break;
               }
            }
            else if((a & ~c & m) != 0)
            {
               temp = UAPInt((c | m) & -m);
               if(temp <= ud)
               {
                  c = temp;
                  break;
               }
            }
            m = lshr(m, 1);
         }
         return a | c;
      }

      APInt maxOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         UAPInt ua(a), uc(c), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((b & d & m) != 0)
            {
               temp = UAPInt((b - m) | (m - One));
               if(temp >= ua)
               {
                  b = temp;
                  break;
               }
               temp = UAPInt((d - m) | (m - One));
               if(temp >= uc)
               {
                  d = temp;
                  break;
               }
            }
            m = lshr(m, 1);
         }
         return b | d;
      }

      APInt minXOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         UAPInt ub(b), ud(d), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((~a & c & m) != 0)
            {
               temp = UAPInt((a | m) & -m);
               if(temp <= ub)
               {
                  a = temp;
               }
            }
            else if((a & ~c & m) != 0)
            {
               temp = UAPInt((c | m) & -m);
               if(temp <= ud)
               {
                  c = temp;
               }
            }
            m = lshr(m, 1);
         }
         return a ^ c;
      }

      APInt maxXOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         UAPInt ua(a), uc(c), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((b & d & m) != 0)
            {
               temp = UAPInt((b - m) | (m - 1));
               if(temp >= ua)
               {
                  b = temp;
               }
               else
               {
                  temp = UAPInt((d - m) | (m - 1));
                  if(temp >= uc)
                  {
                     d = temp;
                  }
               }
            }
            m = lshr(m, 1);
         }
         return b ^ d;
      }

   } // namespace

   /**
    * Or operation coded following Hacker's Delight algorithm.
    */
   Range Range::Or(const Range& other) const
   {
      //      llvm::errs() << "Or-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "Or-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();

      //      if (a.eq(Min) || b.eq(Max) || c.eq(Min) || d.eq(Max))
      //         return Range(Regular,getBitWidth());

      unsigned char switchval = 0;
      switchval += (a >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (b >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (c >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (d >= 0 ? 1 : 0);

      APInt l = Min, u = Max;

      // llvm::errs() << "switchval " << (unsigned)switchval << "\n";

      switch(switchval)
      {
         case 0:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 1:
            l = a;
            // u = llvm::APInt::getAllOnesValue(MAX_BIT_INT);
            u = -1;  // TODO: check correctness against commented instruction above
            break;
         case 3:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 4:
            l = c;
            // u = llvm::APInt::getAllOnesValue(MAX_BIT_INT);
            u = -1;
            break;
         case 5:
            l = (a < c ? a : c);
            u = maxOR(0, b, 0, d);
            break;
         case 7:
            // l = minOR(a, APInt::getAllOnesValue(MAX_BIT_INT), c, d);
            l = minOR(a, -1, c, d);
            u = maxOR(0, b, c, d);
            break;
         case 12:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 13:
            // l = minOR(a, b, c, APInt::getAllOnesValue(MAX_BIT_INT));
            l = minOR(a, b, c, -1);
            u = maxOR(a, b, 0, d);
            break;
         case 15:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
      }
      if((l == Min) || (u == Max))
      {
         return Range(Regular, getBitWidth());
      }
      auto res = Range(Regular, getBitWidth(), l, u);
      //      llvm::errs() << "Or-res: ";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   Range Range::Xor(const Range& other) const
   {
      //      llvm::errs() << "Xor-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "Xor-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();
      if((a >= 0) && (b >= 0) && (c >= 0) && (d >= 0))
      {
         APInt l = minXOR(a, b, c, d);
         APInt u = maxXOR(a, b, c, d);
         auto res = Range(Regular, getBitWidth(), l, u);
         //         llvm::errs() << "Xor-res: ";
         //         res.print(llvm::errs());
         //         llvm::errs() << "\n";
         //         return res;
      }
      else if((c == -1) && (d == -1) && (a >= 0) && (b >= 0))
      {
         auto res = other.sub(*this);
         //         llvm::errs() << "Xor-res-1: ";
         //         res.print(llvm::errs());
         //         llvm::errs() << "\n";
         return res;
      }
      return Range(Regular, getBitWidth());
   }

   Range Range::Eq(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if((getLower() == Min) || (getUpper() == Max) || (other.getLower() == Min) || (other.getUpper() == Max))
         {
            return Range(Regular, bw, Zero, One);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !((a == b) && *this == other);

      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, One);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, One, One);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, Zero);
      }

      THROW_UNREACHABLE("condition unexpected");
   }
   Range Range::Ne(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if((getLower() == Min) || (getUpper() == Max) || (other.getLower() == Min) || (other.getUpper() == Max))
         {
            return Range(Regular, bw, Zero, One);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !((a == b) && *this == other);
      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, One);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, Zero, Zero);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, One, One);
      }

      THROW_UNREACHABLE("condition unexpected");
   }
   Range Range::Ugt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(a > d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c >= b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Uge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(a >= d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c > b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Ult(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(b < c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d <= a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Ule(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(b <= c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d < a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sgt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a > d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c >= b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a >= d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c > b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Slt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b < c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d <= a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sle(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b <= c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d < a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }

   // Truncate
   // - if the source range is entirely inside max bit range, it is the result
   // - else, the result is the max bit range
   Range Range::truncate(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }

      APInt maxupper = getSignedMaxValue(bitwidth);
      APInt maxlower = getSignedMinValue(bitwidth);

      // Check if source range is contained by max bit range
      if(!this->isAnti() && this->getLower() >= maxlower && this->getUpper() <= maxupper)
      {
         return Range(Regular, bitwidth, this->getLower(), this->getUpper());
      }

      return Range(Regular, bitwidth, maxlower, maxupper);
   }

   Range Range::sextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto from_bw = getBitWidth();
      auto this_min = from_bw == 1 ? getUnsignedMin() : getSignedMin();
      auto this_max = from_bw == 1 ? getUnsignedMax() : getSignedMax();
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto sextRes = Range(Regular, bitwidth, ap_trunc(thisR.getLower(), bitwidth), ap_trunc(thisR.getUpper(), bitwidth));
      if(sextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return Range(Regular, bitwidth, sextRes.getSignedMin(), sextRes.getSignedMax());
   }

   Range Range::zextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto this_min = getUnsignedMin();
      auto this_max = getUnsignedMax();
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto zextRes = Range(Regular, bitwidth, ap_trunc(thisR.getLower(), bitwidth), ap_trunc(thisR.getUpper(), bitwidth));
      if(zextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return Range(Regular, bitwidth, zextRes.getUnsignedMin(), zextRes.getUnsignedMax());
   }

   Range Range::intersectWith(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      //      llvm::errs() << "intersectWith-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "intersectWith-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower() > other.getLower() ? getLower() : other.getLower();
         APInt u = getUpper() < other.getUpper() ? getUpper() : other.getUpper();
         if(u < l)
         {
            return Range(Empty, getBitWidth());
         }

         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil <= other.getLower())
         {
            if(other.getUpper() <= antiu)
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = other.getLower() > antiu ? other.getLower() : antiu + 1;
            APInt u = other.getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu >= other.getUpper())
         {
            assert(!other.getLower() >= antil);
            APInt l = other.getLower();
            APInt u = other.getUpper() < antil ? other.getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(other.getLower() == Min && other.getUpper() == Max)
         {
            return *this;
         }
         if(antil > other.getUpper() || antiu < other.getLower())
         {
            return other;
         }

         // we approximate to the range of other
         return other;
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil <= this->getLower())
         {
            if(this->getUpper() <= antiu)
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = this->getLower() > antiu ? this->getLower() : antiu + 1;
            APInt u = this->getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu >= this->getUpper())
         {
            assert(!this->getLower() >= antil);
            APInt l = this->getLower();
            APInt u = this->getUpper() < antil ? this->getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(this->getLower() == Min && this->getUpper() == Max)
         {
            return other;
         }
         if(antil > this->getUpper() || antiu < this->getLower())
         {
            return *this;
         }

         // we approximate to the range of this
         return *this;
      }

      auto antiRange_a = getAnti(*this);
      auto antiRange_b = getAnti(other);
      auto antil_a = antiRange_a.getLower();
      auto antiu_a = antiRange_a.getUpper();
      auto antil_b = antiRange_b.getLower();
      auto antiu_b = antiRange_b.getUpper();
      if(antil_a > antil_b)
      {
         std::swap(antil_a, antil_b);
         std::swap(antiu_a, antiu_b);
      }
      if(antil_b > (antiu_a + 1))
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_a);
      }
      APInt l = antil_a;
      APInt u = antiu_a > antiu_b ? antiu_a : antiu_b;
      if(l == Min && u == Max)
      {
         return Range(Empty, getBitWidth());
      }

      return Range(Anti, getBitWidth(), l, u);
   }

   Range Range::unionWith(const Range& other) const
   {
      if(this->isEmpty() || this->isUnknown())
      {
         return other;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return *this;
      }
      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower() < other.getLower() ? getLower() : other.getLower();
         APInt u = getUpper() > other.getUpper() ? getUpper() : other.getUpper();
         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil != Min);
         assert(antiu != Max);
         if(antil > other.getUpper() || antiu < other.getLower())
         {
            return *this;
         }
         if(antil > other.getLower() && antiu < other.getUpper())
         {
            return Range(Regular, getBitWidth());
         }
         if(antil >= other.getLower() && antiu > other.getUpper())
         {
            return Range(Anti, getBitWidth(), other.getUpper() + 1, antiu);
         }
         if(antil < other.getLower() && antiu <= other.getUpper())
         {
            return Range(Anti, getBitWidth(), antil, other.getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil != Min);
         assert(antiu != Max);
         if(antil > this->getUpper() || antiu < this->getLower())
         {
            return other;
         }
         if(antil > this->getLower() && antiu < this->getUpper())
         {
            return Range(Regular, getBitWidth());
         }
         if(antil >= this->getLower() && antiu > this->getUpper())
         {
            return Range(Anti, getBitWidth(), this->getUpper() + 1, antiu);
         }
         if(antil < this->getLower() && antiu <= this->getUpper())
         {
            return Range(Anti, getBitWidth(), antil, this->getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }

      auto antiRange_a = getAnti(*this);
      auto antiRange_b = getAnti(other);
      auto antil_a = antiRange_a.getLower();
      auto antiu_a = antiRange_a.getUpper();
      assert(antil_a != Min);
      assert(antiu_a != Max);
      auto antil_b = antiRange_b.getLower();
      auto antiu_b = antiRange_b.getUpper();
      assert(antil_b != Min);
      assert(antiu_b != Max);
      if(antil_a > antiu_b || antiu_a < antil_b)
      {
         return Range(Regular, getBitWidth());
      }
      if(antil_a > antil_b && antiu_a < antiu_b)
      {
         return *this;
      }
      if(antil_b > antil_a && antiu_b < antiu_a)
      {
         return *this;
      }
      if(antil_a >= antil_b && antiu_b <= antiu_a)
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_b);
      }
      if(antil_b >= antil_a && antiu_a <= antiu_b)
      {
         return Range(Anti, getBitWidth(), antil_b, antiu_a);
      }

      THROW_UNREACHABLE("unsupported condition");
   }

   Range Range::BestRange(const Range& UR, const Range& SR, unsigned bw) const
   {
      if(UR.isFullSet() && SR.isFullSet())
      {
         return Range(Regular, bw);
      }
      if(UR.isFullSet())
      {
         return SR.truncate(bw);
      }
      if(SR.isFullSet())
      {
         return UR.truncate(bw);
      }
      auto nbitU = neededBits(UR.getUnsignedMin(), UR.getUnsignedMax(), false);
      auto nbitS = neededBits(SR.getSignedMin(), SR.getSignedMax(), true);
      if(nbitU < nbitS)
      {
         return UR.truncate(bw);
      }

      return SR.truncate(bw);
   }

   bool Range::operator==(const Range& other) const
   {
      return getBitWidth() == other.getBitWidth() && Range::isSameType(*this, other) && Range::isSameRange(*this, other);
   }

   bool Range::operator!=(const Range& other) const
   {
      return getBitWidth() != other.getBitWidth() || !Range::isSameType(*this, other) || !Range::isSameRange(*this, other);
   }

   void Range::print(std::ostream& OS) const
   {
      if(this->isUnknown())
      {
         OS << "Unknown";
         return;
      }
      if(this->isEmpty())
      {
         OS << "Empty";
         return;
      }
      if(this->isAnti())
      {
         auto antiObj = getAnti(*this);
         if(antiObj.getLower() == Min)
         {
            OS << ")-inf,";
         }
         else
         {
            OS << ")" << antiObj.getLower().str() << ",";
         }
         OS << getBitWidth() << ",";
         if(antiObj.getUpper() == Max)
         {
            OS << "+inf(";
         }
         else
         {
            OS << antiObj.getUpper().str() << "(";
         }
      }
      else
      {
         if(getLower() == Min)
         {
            OS << "[-inf,";
         }
         else
         {
            OS << "[" << getLower().str() << ",";
         }
         OS << getBitWidth() << ",";
         if(getUpper() == Max)
         {
            OS << "+inf]";
         }
         else
         {
            OS << getUpper().str() << "]";
         }
      }
   }

   std::ostream& operator<<(std::ostream& OS, const Range& R)
   {
      R.print(OS);
      return OS;
   }

   // ========================================================================== //
   // RangeAnalysis
   // ========================================================================== //
   Range_Analysis::Range_Analysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par)
      : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, dfm, par)
   {
      debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   }

   Range_Analysis::~Range_Analysis() = default;

   const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> 
   Range_Analysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
   {
      std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
      switch(relationship_type)
      {
         case DEPENDENCE_RELATIONSHIP:
         {
            relationships.insert(std::make_pair(ESSA, ALL_FUNCTIONS));
            break;
         }
         case PRECEDENCE_RELATIONSHIP:
         {
            break;
         }
         case INVALIDATION_RELATIONSHIP:
         {
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
      return relationships;
   }

   bool Range_Analysis::HasToBeExecuted() const
   {
      return true;
   }

   DesignFlowStep_Status Range_Analysis::Exec()
   {
      
      return DesignFlowStep_Status::UNCHANGED;
   }

   void Range_Analysis::Initialize()
   {
   }

   unsigned Range_Analysis::getMaxBitWidth(unsigned int F)
   {
      unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

      const auto TM = AppM->get_tree_manager();
      auto* FD = GetPointer<function_decl>(TM->get_tree_node_const(F));
      auto* SL = GetPointer<statement_list>(GET_NODE(FD->body));

      for(const auto& [bb_id, bb] : SL->list_of_bloc)
      {
         const auto& stmt_list = bb->CGetStmtList();
         if(stmt_list.size())
         {
            for(const auto& stmt : stmt_list)
            {
               auto* I = GetPointer<gimple_node>(GET_NODE(stmt));

               THROW_ASSERT(I != nullptr, "Instruction should be valid");
               
               // TODO: get maximum bitwidth of I operands
            }
         }
      }
      // Bit-width equal to 0 is not valid, so we increment to 1
      if(max == 0)
      {
         ++max;
      }
      return max;
   }

   unsigned Range_Analysis::getMaxBitWidth()
   {
      unsigned max = 0;

      const auto functions = AppM->get_functions_with_body();
      for(unsigned int f : functions)
      {
         unsigned bitwidth = getMaxBitWidth(f);
         if(bitwidth > max)
         {
            max = bitwidth;
         }
      }

      return max + 1;
   }

   void Range_Analysis::updateConstantIntegers(unsigned maxBitWidth)
   {
      // Updates the Min and Max values.
      Min = getSignedMinValue(maxBitWidth);
      Max = getSignedMaxValue(maxBitWidth);
      Zero = 0;
      One = 1;
   }
}