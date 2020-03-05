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

#include "config_HAVE_ASSERTS.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "graph.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "var_pp_functor.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// stl
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include "custom_map.hpp"

/// Tree includes
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "ext_tree_node.hpp"
#include "tree_reindex.hpp"

#include "bit_lattice.hpp"

#include <boost/filesystem/operations.hpp> // for create_directories
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS


#define RA_JUMPSET
//    #define EARLY_DEAD_CODE_RESTART     // Abort analysis when dead code is detected instead of waiting step's end
#define INTEGER_PTR                 // Pointers are considered as integers
#define BITVALUE_UPDATE             // Read/write bitvalue information during the analysis

#ifndef NDEBUG
#define RA_DEBUG_NONE         0
#define RA_DEBUG_READONLY    1
#define RA_DEBUG_NOEXEC       2
//    #define DEBUG_RANGE_OP
#define SCC_DEBUG
#endif

#define CASE_MISCELLANEOUS       \
   aggr_init_expr_K:             \
   case case_label_expr_K:       \
   case lut_expr_K:              \
   case target_expr_K:           \
   case target_mem_ref_K:        \
   case target_mem_ref461_K:     \
   case binfo_K:                 \
   case block_K:                 \
   case constructor_K:           \
   case error_mark_K:            \
   case identifier_node_K:       \
   case ssa_name_K:              \
   case statement_list_K:        \
   case tree_list_K:             \
   case tree_vec_K:              \
   case call_expr_K

#define OPERATION_OPTION(opts, X) if(opts.contains("no_"#X)) { INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: "#X" operation disabled"); enable_##X = false; }
#define RETURN_DISABLED_OPTION(x, bw) if(!enable_##x) { return RangeRef(new Range(Regular, bw)); }
#define RESULT_DISABLED_OPTION(x, var, stdResult) enable_##x ? stdResult : getFullRangeFor(var)

using bw_t = Range::bw_t;

union vcFloat {
   float flt;
   struct _FP_STRUCT_LAYOUT
   {
#if __BYTE_ORDER == __BIG_ENDIAN
      uint32_t sign : 1;
      uint32_t exp : 8;
      uint32_t frac : 23;
#else
      uint32_t coded;
#endif
   } bits __attribute__((packed));
};

union vcDouble {
   double dub;
   struct _FP_STRUCT_LAYOUT
   {
#if __BYTE_ORDER == __BIG_ENDIAN
      uint64_t sign : 1;
      uint64_t exp : 11;
      uint64_t frac : 52;
#else
      uint64_t coded;
#endif
   } bits __attribute__((packed));
};

bool tree_reindexCompare::operator()(const tree_nodeConstRef &lhs, const tree_nodeConstRef &rhs) const
{
   return GET_INDEX_CONST_NODE(lhs) < GET_INDEX_CONST_NODE(rhs);
}

/**********************************************************
 * APInt
 **********************************************************/
using namespace boost::multiprecision;
APInt::APInt() : _data()
{
}

APInt::APInt(const APInt& other) : _data(other._data,nullptr)
{
}

APInt& APInt::operator=(const APInt& other)
{
   _data = other._data;
   return *this;
}

bool operator<(const APInt& lhs, const APInt& rhs)
{
   return lhs._data < rhs._data;
}

bool operator>(const APInt& lhs, const APInt& rhs)
{
   return lhs._data > rhs._data;
}

bool operator<=(const APInt& lhs, const APInt& rhs)
{
   return lhs._data <= rhs._data;
}

bool operator>=(const APInt& lhs, const APInt& rhs)
{
   return lhs._data >= rhs._data;
}

bool operator==(const APInt& lhs, const APInt& rhs)
{
   return lhs._data == rhs._data;
}

bool operator!=(const APInt& lhs, const APInt& rhs)
{
   return lhs._data != rhs._data;
}

APInt::operator bool() const
{
   return _data != 0;
}

/*
 * Binary operators
 */
APInt operator+(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) += rhs;
}

APInt operator-(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) -= rhs;
}

APInt operator*(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) *= rhs;
}

APInt operator/(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) /= rhs;
}

APInt operator%(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) %= rhs;
}

APInt operator&(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) &= rhs;
}

APInt operator|(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) |= rhs;
}

APInt operator^(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) ^= rhs;
}

APInt operator<<(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) <<= rhs;
}

APInt operator>>(const APInt& lhs, const APInt& rhs)
{
   return APInt(lhs) >>= rhs;
}

APInt& APInt::operator+=(const APInt& rhs)
{
   _data += rhs._data;
   return *this;
}

APInt& APInt::operator-=(const APInt& rhs)
{
   _data -= rhs._data;
   return *this;
}

APInt& APInt::operator*=(const APInt& rhs)
{
   _data *= rhs._data;
   return *this;
}

APInt& APInt::operator/=(const APInt& rhs)
{
   _data /= rhs._data;
   return *this;
}

APInt& APInt::operator%=(const APInt& rhs)
{
   _data %= rhs._data;
   return *this;
}

APInt& APInt::operator&=(const APInt& rhs)
{
   _data &= rhs._data;
   return *this;
}

APInt& APInt::operator|=(const APInt& rhs)
{
   _data |= rhs._data;
   return *this;
}

APInt& APInt::operator^=(const APInt& rhs)
{
   _data ^= rhs._data;
   return *this;
}

APInt& APInt::operator<<=(const APInt& rhs)
{
   mpz_mul_2exp(_data.backend().data(), _data.backend().data(), mpz_get_ui(rhs._data.backend().data()));
   return *this;
}

APInt& APInt::operator>>=(const APInt& rhs)
{
   mpz_div_2exp(_data.backend().data(), _data.backend().data(), mpz_get_ui(rhs._data.backend().data()));
   return *this;
}

/*
 * Unary operators
 */
APInt APInt::abs() const
{
   APInt abs;
   abs._data = boost::multiprecision::abs(_data);
   return abs;
}

APInt APInt::operator-() const
{
   APInt neg;
   neg._data = -_data;
   return neg;
}

APInt APInt::operator~() const
{
   APInt _not;
   _not._data = ~_data;
   return _not;
}

APInt APInt::operator++(int)
{
   APInt t = *this;
   ++_data;
   return t;
}

APInt APInt::operator--(int)
{
   APInt t = *this;
   --_data;
   return t;
}

APInt& APInt::operator++()
{
   return operator+=(1LL);
}

APInt& APInt::operator--()
{
   return operator-=(1LL);
}

void APInt::bit_set(bw_t i)
{
   boost::multiprecision::bit_set(_data, i);
}

void APInt::bit_clr(bw_t i)
{
   boost::multiprecision::bit_unset(_data, i);
}

bool APInt::bit_tst(bw_t i) const
{
   return boost::multiprecision::bit_test(_data, i);
}

bool APInt::sign() const
{
   return _data < 0;
}

APInt& APInt::extOrTrunc(bw_t bw, bool sign)
{
   THROW_ASSERT(bw, "Minimum bitwidth of 1 is required");
   const APInt_internal mask((APInt_internal(1,nullptr) << bw) - APInt_internal(1,nullptr),nullptr);
   _data &= mask;
   if(sign && bit_test(_data, static_cast<unsigned>(bw - 1)))
   {
      _data += (APInt_internal(-1,nullptr) << bw);
   }
   return *this;
}

APInt APInt::extOrTrunc(bw_t bw, bool sign) const
{
   THROW_ASSERT(bw, "Minimum bitwidth of 1 is required");
   const APInt_internal mask((APInt_internal(1,nullptr) << bw) - APInt_internal(1,nullptr),nullptr);
   APInt_internal val = _data & mask;
   if(sign && bit_test(val, static_cast<unsigned>(bw - 1)))
   {
      val += (APInt_internal(-1,nullptr) << bw);
   }
   return APInt(val);
}

bw_t APInt::trailingZeros(bw_t bw) const
{
   return static_cast<bw_t>(std::min(static_cast<mp_bitcnt_t>(bw), mpz_scan1(_data.backend().data(), 0)));
}

bw_t APInt::trailingOnes(bw_t bw) const
{
   return static_cast<bw_t>(std::min(static_cast<mp_bitcnt_t>(bw), mpz_scan0(_data.backend().data(), 0)));
}

bw_t APInt::leadingZeros(bw_t bw) const
{
   int i = bw;
   for(; i > 0;)
   {
      if(bit_test(_data, static_cast<unsigned>(--i)))
      {
         break;
      }
   }
   i = i ? (i + 1) : i;
   return static_cast<bw_t>(bw - i);
}

bw_t APInt::leadingOnes(bw_t bw) const
{
   int i = bw;
   for(; i >= 0;)
   {
      if(!bit_test(_data, static_cast<unsigned>(--i)))
      {
         break;
      }
   }
   ++i;
   return static_cast<bw_t>(bw - i);
}

APInt::bw_t APInt::minBitwidth(bool sign) const
{
   const auto bw = static_cast<bw_t>(mpz_sizeinbase(_data.backend().data(), 2));
   return sign ? (leadingOnes(bw) == 1 ? bw : static_cast<bw_t>(bw + 1)) : (_data.sign() < 0 ? static_cast<bw_t>(std::numeric_limits<bw_t>::max()) : bw);
}

int64_t APInt::toInt() const
{
   return _data.convert_to<int64_t>();
}

uint64_t APInt::toIntUnsigned() const
{
   return _data.convert_to<uint64_t>();
}

std::string APInt::str() const
{
   return _data.str();
}

APInt APInt::getMaxValue(bw_t bw)
{
   return (APInt(1) << bw) - 1;
}

APInt APInt::getMinValue(bw_t)
{
   return APInt(0);
}

APInt APInt::getSignedMaxValue(bw_t bw)
{
   return (APInt(1) << (bw - 1)) - APInt(1);
}

APInt APInt::getSignedMinValue(bw_t bw)
{
   return -(APInt(1) << (bw - 1));
}

std::ostream& operator<<(std::ostream& str, const APInt& v)
{
   str << v.str();
   return str;
}

// ========================================================================== //
// Range
// ========================================================================== //
namespace
{
   // The number of bits needed to store the largest variable of the function (APInt).
   const bw_t MAX_BIT_INT = static_cast<bw_t>(128U);
   const APInt Min = APInt::getSignedMinValue(MAX_BIT_INT);
   const APInt Max = APInt::getSignedMaxValue(MAX_BIT_INT);
   const APInt MinDelta(1);
}

static bool enable_add = true;
static bool enable_sub = true;
static bool enable_mul = true;
static bool enable_sdiv = true;
static bool enable_udiv = true;
static bool enable_srem = true;
static bool enable_urem = true;
static bool enable_shl = true;
static bool enable_shr = true;
static bool enable_abs = true;
static bool enable_negate = true;
static bool enable_not = true;
static bool enable_and = true;
static bool enable_or = true;
static bool enable_xor = true;
static bool enable_sext = true;
static bool enable_zext = true;
static bool enable_trunc = true;
static bool enable_min = true;
static bool enable_max = true;

Range::Range(RangeType rType, bw_t rbw) : l(Min), u(Max), bw(rbw), type(rType)
{
   THROW_ASSERT(rbw > 0 && rbw <= MAX_BIT_INT, "Invalid bitwidth for range (bw = " + STR(rbw) +")");
}

Range::Range(RangeType rType, bw_t rbw, const APInt& lb, const APInt& ub) : l(lb), u(ub), bw(rbw), type(rType)
{
   THROW_ASSERT(rbw > 0 && rbw <= MAX_BIT_INT, "Invalid bitwidth for range (bw = " + STR(rbw) +")");
   normalizeRange(lb, ub, rType);
}

Range* Range::clone() const
{
   return new Range(*this);
}

void Range::normalizeRange(const APInt& lb, const APInt& ub, RangeType rType)
{
   if(rType == Real)
   {
      THROW_UNREACHABLE("Real range is a storage class only");
   }
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
            l = ub + MinDelta;
            u = Max;
         }
         else if(ub == Max)
         {
            type = Regular;
            l = Min;
            u = lb - MinDelta;
         }
         else
         {
            THROW_ASSERT(ub >= lb, "");
            const auto maxS = APInt::getSignedMaxValue(bw);
            const auto minS = APInt::getSignedMinValue(bw);
            const bool lbgt = lb > maxS;
            const bool ubgt = ub > maxS;
            const bool lblt = lb < minS;
            const bool ublt = ub < minS;
            if(lbgt && ubgt)
            {
               l = lb.extOrTrunc(bw, true);
               u = ub.extOrTrunc(bw, true);
            }
            else if(lblt && ublt)
            {
               l = ub.extOrTrunc(bw, true);
               u = lb.extOrTrunc(bw, true);
            }
            else if(!lblt && ubgt)
            {
               const auto ubnew = ub.extOrTrunc(bw, true);
               if(ubnew >= (lb - MinDelta))
               {
                  l = Min;
                  u = Max;
               }
               else
               {
                  type = Regular;
                  l = ubnew + MinDelta;
                  u = lb - MinDelta;
               }
            }
            else if(lblt && !ubgt)
            {
               const auto lbnew = lb.extOrTrunc(bw, true);
               if(lbnew <= (ub + MinDelta))
               {
                  l = Min;
                  u = Max;
               }
               else
               {
                  type = Regular;
                  l = ub + MinDelta;
                  u = lbnew - MinDelta;
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
   else if((lb - MinDelta) == ub)
   {
      type = Regular;
      l = Min;
      u = Max;
   }
   else if(lb > ub)
   {
      normalizeRange(ub + MinDelta, lb - MinDelta, Anti);
   }
   else
   {
      THROW_ASSERT(ub >= lb, "");
      const auto maxS = APInt::getSignedMaxValue(bw);
      const auto minS = APInt::getSignedMinValue(bw);
      const bool lbgt = lb > maxS;
      const bool ubgt = ub > maxS;
      const bool lblt = lb < minS;
      const bool ublt = ub < minS;
      if(ubgt && lblt)
      {
         l = Min;
         u = Max;
      }
      else if(lbgt && ubgt)
      {
         l = lb.extOrTrunc(bw, true);
         u = ub.extOrTrunc(bw, true);
      }
      else if(lblt && ublt)
      {
         l = ub.extOrTrunc(bw, true);
         u = lb.extOrTrunc(bw, true);
      }
      else if(!lblt && ubgt)
      {
         const auto ubnew = ub.extOrTrunc(bw, true);
         if(ubnew >= (lb - MinDelta))
         {
            l = Min;
            u = Max;
         }
         else
         {
            type = Anti;
            l = ubnew + MinDelta;
            u = lb - MinDelta;
         }
      }
      else if(lblt && !ubgt)
      {
         const auto lbnew = lb.extOrTrunc(bw, true);
         if(lbnew <= (ub + MinDelta))
         {
            l = Min;
            u = Max;
         }
         else
         {
            type = Anti;
            l = ub + MinDelta;
            u = lbnew - MinDelta;
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

bw_t Range::neededBits(const APInt& a, const APInt& b, bool sign)
{
   return std::max(a.minBitwidth(sign), b.minBitwidth(sign));
}

RangeRef Range::getAnti() const
{
   if(type == Anti)
   {
      return RangeRef(new Range(Regular, bw, l, u));
   }
   if(type == Regular)
   {
      return RangeRef(new Range(Anti, bw, l, u));
   }
   if(type == Empty)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(type == Unknown)
   {
      return RangeRef(this->clone());
   }
   if(type == Real)
   {
      THROW_UNREACHABLE("Real range is a storage class only");
   }
   THROW_UNREACHABLE("unexpected condition");
   return nullptr;
}

bw_t Range::getBitWidth() const
{
   return bw;
}

const APInt &Range::getLower() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isAnti(), "Lower bound not valid for Anti range");
   return l;
}

const APInt &Range::getUpper() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isAnti(), "Upper bound not valid for Anti range");
   return u;
}

APInt Range::getSignedMax() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isUnknown() && !isEmpty(), "Max not valid for Unknown/Empty range");
   const auto maxS = APInt::getSignedMaxValue(bw);
   if(type == Regular)
   {
      return u > maxS ? maxS : u;
   }
   return maxS;
}

APInt Range::getSignedMin() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isUnknown() && !isEmpty(), "Min not valid for Unknown/Empty range");
   const auto minS = APInt::getSignedMinValue(bw);
   if(type == Regular)
   {
      return l < minS ? minS : l;
   }
   return minS;
}

APInt Range::getUnsignedMax() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isUnknown() && !isEmpty(), "UMax not valid for Unknown/Empty range");
   if(isAnti())
   {
      auto lb = l - MinDelta;
      auto ub = u + MinDelta;
      return (lb >= 0 || ub < 0) ? APInt::getMaxValue(bw) : lb.extOrTrunc(bw, false);
   }
   return (u < 0 || l >= 0) ? u.extOrTrunc(bw, false) : APInt::getMaxValue(bw);
}

APInt Range::getUnsignedMin() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   THROW_ASSERT(!isUnknown() && !isEmpty(), "UMin not valid for Unknown/Empty range");
   if(isAnti())
   {
      return (l > 0 || u < 0) ? APInt::getMinValue(bw) : (u + MinDelta);
   }
   return (l > 0 || u < 0) ? l.extOrTrunc(bw, false) : APInt::getMinValue(bw);
}

bool Range::isUnknown() const
{
   return type == Unknown;
}

void Range::setUnknown()
{
   type = Unknown;
}

bool Range::isRegular() const
{
   return type == Regular;
}

bool Range::isAnti() const
{
   return type == Anti;
}

bool Range::isEmpty() const
{
   return type == Empty;
}

bool Range::isReal() const
{
   return false;
}

bool Range::isSameType(RangeConstRef other) const
{
   return type == other->type;
}

bool Range::isSameRange(RangeConstRef other) const
{
   return this->bw == other->bw && isSameType(other) && (l == other->l) && (u == other->u);
}

bool Range::isFullSet() const
{
   THROW_ASSERT(!isUnknown(), "");
   if(isEmpty() || isAnti())
   {
      return false;
   }
   return (APInt::getSignedMaxValue(bw) <= getUpper() && APInt::getSignedMinValue(bw) >= getLower())
      || (APInt::getMaxValue(bw) <= getUpper() && APInt::getMinValue(bw) >= getLower());
}

bool Range::isSingleElement() const
{
   if(isUnknown() || isEmpty())
   {
      return false;
   }
   return l == u;
}

bool Range::isConstant() const
{
   return isRegular() && l == u;
}

#define RETURN_EMPTY_ON_EMPTY(b) if(this->isEmpty() || other->isEmpty()){return RangeRef(new Range(Empty, b));}
#define RETURN_UNKNOWN_ON_UNKNOWN(b) if(this->isUnknown() || other->isUnknown()){return RangeRef(new Range(Unknown, b));}

/// Add and Mul are commutative. So, they are a little different
/// than the other operations.
/// Many Range reductions are done by exploiting ConstantRange code
RangeRef Range::add(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(ol >= (Max - u))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Anti, bw, l + ol, u + ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         return RangeRef(new Range(Regular, bw, l, u + ol));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         return RangeRef(new Range(Regular, bw, l + ol, u));
      }

      return RangeRef(new Range(Regular, bw, l + ol, u + ol));
   }

   auto sMin = std::clamp(getSignedMin() + other->getSignedMin(), APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw));
   auto sMax = std::clamp(getSignedMax() + other->getSignedMax(), APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw));
   auto uMin = std::min({getUnsignedMin() + other->getUnsignedMin(), APInt::getMaxValue(bw)});
   auto uMax = std::min({getUnsignedMax() + other->getUnsignedMax(), APInt::getMaxValue(bw)});

   if(neededBits(uMin, uMax, false) < neededBits(sMin, sMax, true))
   {
      return RangeRef(new Range(Regular, bw, uMin, uMax));
   }
   return RangeRef(new Range(Regular, bw, sMin, sMax));
}

RangeRef Range::sub(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(l <= (Min + ol))
      {
         return RangeRef(new Range(Regular, bw));
      }

      return RangeRef(new Range(Anti, bw, l - ol, u - ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         auto minValue = APInt::getSignedMinValue(bw);
         auto upper = u - ol;
         if(minValue > upper)
         {
            return RangeRef(new Range(Regular, bw));
         }

         return RangeRef(new Range(Regular, bw, l, upper));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         auto maxValue = APInt::getSignedMaxValue(bw);
         auto lower = l - ol;
         if(maxValue < lower)
         {
            return RangeRef(new Range(Regular, bw));
         }

         return RangeRef(new Range(Regular, bw, l - ol, u));
      }

      auto lower = (l - ol).extOrTrunc(bw, true);
      auto upper = (u - ol).extOrTrunc(bw, true);
      if(lower <= upper)
      {
         return RangeRef(new Range(Regular, bw, lower, upper));
      }
      return RangeRef(new Range(Anti, bw, upper + 1, lower - 1));
   }

   auto sMin = std::clamp(getSignedMin() - other->getSignedMax(), APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw));
   auto sMax = std::clamp(getSignedMax() - other->getSignedMin(), APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw));
   auto uMin = getUnsignedMin() - other->getUnsignedMax();
   auto uMax = getUnsignedMax() - other->getUnsignedMin();

   if(neededBits(uMin, uMax, false) < neededBits(sMin, sMax, true))
   {
      return RangeRef(new Range(Regular, bw, uMin, uMax));
   }
   return RangeRef(new Range(Regular, bw, sMin, sMax));
}

RangeRef Range::mul(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   // Multiplication is signedness-independent. However different ranges can be
   // obtained depending on how the input ranges are treated. These different
   // ranges are all conservatively correct, but one might be better than the
   // other. We calculate two ranges; one treating the inputs as unsigned
   // and the other signed, then return the smallest of these ranges.

   // Unsigned range first.
   auto this_min = getUnsignedMin();
   auto this_max = getUnsignedMax();
   auto Other_min = other->getUnsignedMin();
   auto Other_max = other->getUnsignedMax();

   const auto Result_zext = Range(Regular, static_cast<bw_t>(bw * 2), this_min * Other_min, this_max * Other_max);
   const auto UR = Result_zext.truncate(bw);

   // Now the signed range. Because we could be dealing with negative numbers
   // here, the lower bound is the smallest of the Cartesian product of the
   // lower and upper ranges; for example:
   //   [-1,4] * [-2,3] = min(-1*-2, -1*-3, 4*-2, 4*3) = -8.
   // Similarly for the upper bound, swapping min for max.

   this_min = getSignedMin();
   this_max = getSignedMax();
   Other_min = other->getSignedMin();
   Other_max = other->getSignedMax();

   const auto [min, max] = std::minmax({this_min * Other_min, this_min * Other_max, this_max * Other_min, this_max * Other_max});
   const auto Result_sext = Range(Regular, static_cast<bw_t>(bw * 2), min, max);
   const auto SR = Result_sext.truncate(bw);
   if(SR->isFullSet())
   {
      return UR;
   }
   if(UR->isFullSet())
   {
      return SR;
   }
   
   const auto uSpan = UR->isAnti() ? Max : (UR->getUpper() - UR->getLower()).abs();
   const auto sSpan = SR->isAnti() ? Max : (SR->getUpper() - SR->getLower()).abs();
   return uSpan < sSpan ? UR : SR;
}

RangeRef Range::udiv(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   
   auto a = getUnsignedMin();
   auto b = getUnsignedMax();
   auto c = other->getUnsignedMin();
   auto d = other->getUnsignedMax();

   // Deal with division by 0 exception
   if((c == 0) && (d == 0))
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(c == 0)
   {
      c = 1;
   }
   RangeRef res(new Range(Regular, bw, a / d, b / c));
   return res;
}

#define DIV_HELPER(x, y) (x == Max) ? ((y < 0) ? Min : ((y == 0) ? 0 : Max)) : ((y == Max) ? 0 : ((x == Min) ? ((y < 0) ? Max : ((y == 0) ? 0 : Min)) : ((y == Min) ? 0 : ((x) / (y)))))

RangeRef Range::sdiv(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || this->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const APInt& a = this->getLower();
   const APInt& b = this->getUpper();
   APInt c1, d1, c2, d2;
   bool is_zero_in = false;
   if(other->isAnti())
   {
      auto antiRange = other->getAnti();
      c1 = Min;
      d1 = antiRange->getLower() - 1;
      if(d1 == 0)
      {
         d1 = -1;
      }
      else if(d1 > 0)
      {
         return RangeRef(new Range(Regular, bw)); /// could be improved
      }
      c2 = antiRange->getUpper() + 1;
      if(c2 == 0)
      {
         c2 = 1;
      }
      else if(c2 < 0)
      {
         return RangeRef(new Range(Regular, bw)); /// could be improved
      }
      d2 = Max;
   }
   else
   {
      c1 = other->getLower();
      d1 = other->getUpper();
      // Deal with division by 0 exception
      if((c1 == 0) && (d1 == 0))
      {
         return RangeRef(new Range(Regular, bw));
      }
      is_zero_in = (c1 < 0) && (d1 > 0);
      if(is_zero_in)
      {
         d1 = -1;
         c2 = 1;
      }
      else
      {
         c2 = other->getLower();
         if(c2 == 0)
         {
            c1 = c2 = 1;
         }
      }
      d2 = other->getUpper();
      if(d2 == 0)
      {
         d1 = d2 = -1;
      }
   }
   auto n_iters = (is_zero_in || other->isAnti()) ? 8u : 4u;

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
   auto [min, max] = std::minmax_element(candidates, candidates + n_iters);
   return RangeRef(new Range(Regular, bw, *min, *max));
}

RangeRef Range::urem(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      if(other->getLower() == 0)
      {
         return RangeRef(new Range(Empty, bw));
      }
      else if(other->getUnsignedMin() == 1)
      {
         return RangeRef(new Range(Regular, bw, 0, 0));
      }
   }

   const APInt& a = this->getUnsignedMin();
   const APInt& b = this->getUnsignedMax();
   APInt c = other->getUnsignedMin();
   const APInt& d = other->getUnsignedMax();

   // Deal with mod 0 exception
   if((c == 0) && (d == 0))
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(c == 0)
   {
      c = 1;
   }

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
   auto [min, max] = std::minmax_element(candidates, candidates + 8);
   RangeRef res(new Range(Regular, bw, *min, *max));
   return res;
}

RangeRef Range::srem(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      if(other->getSignedMin() == 0)
      {
         return RangeRef(new Range(Empty, bw));
      }
      else if(other->getSignedMin() == 1 || other->getSignedMin() == -1)
      {
         return RangeRef(new Range(Regular, bw, 0, 0));
      }
   }

   const auto& a = this->getLower();
   const auto& b = this->getUpper();
   auto c = other->getLower();
   const auto& d = other->getUpper();

   // Deal with mod 0 exception
   if((c == 0) && (d == 0))
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(c == 0)
   {
      c = 1;
   }

   APInt candidates[4];
   candidates[0] = Min;
   candidates[1] = Min;
   candidates[2] = Max;
   candidates[3] = Max;
   if((a != Min) && (c != Min))
   {
      candidates[0] = a % c; // lower lower
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
   auto [min, max] = std::minmax_element(candidates, candidates + 4);
   RangeRef res(new Range(Regular, bw, *min, *max));
   return res;
}

RangeRef Range::shl(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isConstant() && other->isConstant())
   {
      const auto c = (this->getLower() << other->getLower()).extOrTrunc(bw, true);
      return RangeRef(new Range(Regular, bw, c, c));
   }

   const auto a = this->getLower();
   const auto b = this->getUpper();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();

   if(c >= bw)
   {
      return RangeRef(new Range(Regular, bw, 0, 0));
   }
   if(d >= bw)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(a < 0 && b < 0)
   {
      if(d > a.leadingOnes(bw))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Regular, bw, a << d, b << c));
   }
   if(a < 0 && b >= 0)
   {
      if(d > std::min(a.leadingOnes(bw), b.leadingZeros(bw)))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Regular, bw, a << d, b << d));
   }
   if(d > b.leadingZeros(bw))
   {
      return RangeRef(new Range(Regular, bw));
   }
   return RangeRef(new Range(Regular, bw, a << c, b << d));
}

RangeRef Range::shr(RangeConstRef other, bool sign) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   
   if(sign)
   {
      const auto a = getSignedMin();
      const auto b = getSignedMax();
      const auto min = a >= 0 ? a >> d : a >> c;
      const auto max = b >= 0 ? b >> c : b >> d;

      return RangeRef(new Range(Regular, bw, min, max));
   }
   else
   {
      const auto a = getUnsignedMin();
      const auto b = getUnsignedMax();

      return RangeRef(new Range(Regular, bw, a >> d, b >> c));
   }
}


/*
 * 	This and operations are coded following Hacker's Delight algorithm.
 * 	According to the author, they provide tight results.
 */
namespace
{
   APInt minOR(APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(~a & c & m)
         {
            temp = (a | m) & (~m+1);
            if(temp <= b) { a = temp; break; }
         }
         else if(a & ~c & m)
         {
            temp = (c | m) & (~m+1);
            if(temp <= d) { c = temp; break; }
         }
         m = m >> 1;
      }
      return a | c;
   }

   APInt maxOR(const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(b & d & m)
         {
            temp = (b - m) | (m - 1);
            if(temp >= a) { b = temp; break; }
            temp = (d - m) | (m - 1);
            if(temp >= c) { d = temp; break; }
         }
         m = m >> 1;
      }
      return b | d;
   }

   std::pair<APInt,APInt> OR(const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      auto abcd = (a >= 0) << 3 | (b >= 0) << 2 | (c >= 0) << 1 | (d >= 0);

      APInt res_l, res_u;
      switch(abcd)
      {
         case 0:
         case 3:
         case 12:
         case 15:
            res_l = minOR(a, b, c, d);
            res_u = maxOR(a, b, c, d);
            break;
         case 1:
            return std::make_pair(a, -1);
         case 4:
            return std::make_pair(c, -1);
         case 5:
            res_l = std::min({a,c});
            res_u = maxOR(0, b, 0, d);
            break;
         case 7:
            res_l = minOR(a, -1, c, d);
            res_u = maxOR(0, b, c, d);
            break;
         case 13:
            res_l = minOR(a, b, c, -1);
            res_u = maxOR(a, b, 0, d);
            break;
         default:
            THROW_UNREACHABLE("OR unreachable state " + STR(abcd));
      }
      return std::make_pair(res_l, res_u);
   }

   APInt minAND(APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(~a & ~c & m)
         {
            temp = (a | m) & (~m+1);
            if(temp <= b) { a = temp; break; }
            temp = (c | m) & (~m+1);
            if(temp <= d) { c = temp; break; }
         }
         m = m >> 1;
      }
      return a & c;
   }

   APInt maxAND(const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(b & ~d & m)
         {
            temp = (b & ~m) | (m - 1);
            if(temp >= a) { b = temp; break; }
         }
         else if(~b & d & m)
         {
            temp = (d & ~m) | (m - 1);
            if(temp >= c) { d = temp; break; }
         }
         m = m >> 1;
      }
      return b & d;
   }

   std::pair<APInt,APInt> AND(const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      auto abcd = (a >= 0) << 3 | (b >= 0) << 2 | (c >= 0) << 1 | (d >= 0);

      APInt res_l, res_u;
      switch(abcd)
      {
         case 0:
         case 3:
         case 12:
         case 15:
            res_l = minAND(a, b, c, d);
            res_u = maxAND(a, b, c, d);
            break;
         case 1:
            res_l = minAND(a, b, c, -1);
            res_u = maxAND(a, b, 0, d);
            break;
         case 4:
            res_l = minAND(a, -1, c, d);
            res_u = maxAND(0, b, c, d);
            break;
         case 5:
            res_l = minAND(a, -1, c, -1);
            res_u = std::max({b, d});
            break;
         case 7:
            return std::make_pair(0, d);
         case 13:
            return std::make_pair(0, b);
         default:
            THROW_UNREACHABLE("AND unreachable state " + STR(abcd));
      }
      return std::make_pair(res_l, res_u);
   }

   APInt minXOR(APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(~a & c & m)
         {
            temp = (a | m) & (~m+1);
            if(temp <= b) { a = temp; }
         }
         else if(a & ~c & m)
         {
            temp = (c | m) & (~m+1);
            if(temp <= d) { c = temp; }
         }
         m = m >> 1;
      }
      return a ^ c;
   }

   APInt maxXOR(const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (MAX_BIT_INT - 1);
      while(m != 0)
      {
         if(b & d & m)
         {
            temp = (b - m) | (m - 1);
            if(temp >= a) { b = temp; }
            else {
               temp = (d - m) | (m - 1);
               if(temp >= c) { d = temp; }
            }
         }
         m = m >> 1;
      }
      return b ^ d;
   }

   std::pair<APInt,APInt> uXOR(const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      return std::make_pair(minXOR(a,b,c,d), maxXOR(a,b,c,d));
   }

} // namespace

RangeRef Range::Or(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();
   
   const auto [res_l, res_u] = OR(a, b, c, d);
   return RangeRef(new Range(Regular, bw, res_l, res_u));
}

RangeRef Range::And(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();

   const auto [res_l, res_u] = AND(a, b, c, d);
   return RangeRef(new Range(Regular, bw, res_l, res_u));
}

RangeRef Range::Xor(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();
   
   if(a >= 0 && b >= 0 && c >= 0 && d >= 0)
   {
      const auto [res_l,res_u] = uXOR(a, b, c, d);
      return RangeRef(new Range(Regular, bw, res_l, res_u));
   }
   else if(a == -1 && b == -1 && c >= 0 && d >= 0)
   {
      return this->sub(other);
   }
   else if(c == -1 && d == -1 && a >= 0 && b >= 0)
   {
      return other->sub(RangeRef(this->clone()));
   }
   return RangeRef(new Range(Regular, bw));
}

RangeRef Range::Not() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   if(isEmpty() || isUnknown())
   {
      return RangeRef(new Range(this->type, bw));
   }
   
   const auto min = ~this->u;
   const auto max = ~this->l;

   return RangeRef(new Range(this->type, bw, min, max));
}

RangeRef Range::Eq(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(!isAnti() && !other->isAnti())
   {
      if((l == Min) || (u == Max) || (other->l == Min) || (other->u == Max))
      {
         return RangeRef(new Range(Regular, _bw, 0, 1));
      }
   }
   bool areTheyEqual = !this->intersectWith(other)->isEmpty();
   bool areTheyDifferent = !((l == u) && this->isSameRange(other));

   if(areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(areTheyEqual && !areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(!areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   THROW_UNREACHABLE("condition unexpected");
   return nullptr;
}

RangeRef Range::Ne(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(!isAnti() && !other->isAnti())
   {
      if((l == Min) || (u == Max) || (other->l == Min) || (other->u == Max))
      {
         return RangeRef(new Range(Regular, _bw, 0, 1));
      }
   }
   bool areTheyEqual = !this->intersectWith(other)->isEmpty();
   bool areTheyDifferent = !((l == u) && this->isSameRange(other));
   if(areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(areTheyEqual && !areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }
   if(!areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }

   THROW_UNREACHABLE("condition unexpected");
   return nullptr;
}

RangeRef Range::Ugt(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(a > d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c >= b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Uge(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(a >= d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c > b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Ult(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(b < c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d <= a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Ule(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(b <= c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d < a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sgt(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(a > d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c >= b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sge(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(a >= d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c > b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Slt(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(b < c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d <= a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sle(RangeConstRef other, bw_t _bw) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(b <= c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d < a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::SMin(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMin = this->Slt(other, 1);
   if(thisMin->isConstant())
   {
      return thisMin->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::min({this->getSignedMin(), other->getSignedMin()});
   const auto max = std::min({this->getSignedMax(), other->getSignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::SMax(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMax = this->Sgt(other, 1);
   if(thisMax->isConstant())
   {
      return thisMax->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::max({this->getSignedMin(), other->getSignedMin()});
   const auto max = std::max({this->getSignedMax(), other->getSignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::UMin(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMin = this->Ult(other, 1);
   if(thisMin->isConstant())
   {
      return thisMin->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::min({this->getUnsignedMin(), other->getUnsignedMin()});
   const auto max = std::min({this->getUnsignedMax(), other->getUnsignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::UMax(RangeConstRef other) const
{
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMax = this->Ugt(other, 1);
   if(thisMax->isConstant())
   {
      return thisMax->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::max({this->getUnsignedMin(), other->getUnsignedMin()});
   const auto max = std::max({this->getUnsignedMax(), other->getUnsignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::abs() const
{
   if(isEmpty() || isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(isAnti())
   {
      if(u < 0)
      {
         if(l == APInt::getSignedMinValue(bw))
         {
            return RangeRef(new Range(Regular, bw, 0, APInt::getSignedMaxValue(bw)));
         }
         return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, -1));
      }
      if(l < 0)
      {
         if(l == APInt::getSignedMinValue(bw))
         {
            return RangeRef(new Range(Regular, bw, u + 1, APInt::getSignedMaxValue(bw)));
         }
         const auto min = std::min({-l, u});
         return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, min));
      }
      return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, l == 0 ? 0 : -1));
   }

   const auto smin = getSignedMin();
   const auto smax = getSignedMax();
   if(smax < 0)
   {
      if(smin == APInt::getSignedMinValue(bw))
      {
         return RangeRef(new Range(Anti, bw, smin + 1, -smax - 1));
      }
      return RangeRef(new Range(Regular, bw, -smax, -smin));
   }
   if(smin < 0)
   {
      if(smin == APInt::getSignedMinValue(bw))
      {
         return RangeRef(new Range(Anti, bw, smin + 1, -1));
      }
      const auto max = std::max({smax, -smin});
      return RangeRef(new Range(Regular, bw, 0, max));
   }
   return RangeRef(this->clone());
}

RangeRef Range::negate() const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   if(isEmpty() || isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(isAnti())
   {
      return RangeRef(new Range(Anti, bw, -u, -l));
   }
   return RangeRef(new Range(Regular, bw, -u, -l));
}

// Truncate
// - if the source range is entirely inside max bit range, it is the result
// - else, the result is the max bit range
RangeRef Range::truncate(bw_t bitwidth) const
{
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }
   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   if(isFullSet() || isAnti() || (b - a).abs() > APInt::getMaxValue(bitwidth))
   {
      return RangeRef(new Range(Regular, bitwidth));
   }
   if(bitwidth == bw)
   {
      return RangeRef(this->clone());
   }
   
   auto stmin = a.extOrTrunc(bitwidth, true);
   auto stmax = b.extOrTrunc(bitwidth, true);
   
   if(this->getSignedMin() < 0 && this->getSignedMax() >= 0)   // Zero crossing range
   {
      if(stmax < 0)  // overflow
      {
         if(stmax >= stmin)
         {
            return RangeRef(new Range(Regular, bitwidth));
         }
         return RangeRef(new Range(Anti, bitwidth, stmax + 1, (stmin < 0 ? stmin : 0) - 1));
      }
      if(stmin > 0)  // underflow
      {
         if(stmax >= stmin)
         {
            return RangeRef(new Range(Regular, bitwidth));
         }
         return RangeRef(new Range(Anti, bitwidth, stmax + 1, stmin - 1));
      }
   }
   else if((a.sign() != stmin.sign()) ^ (b.sign() != stmax.sign()))   // Non zero crossing range
   {
      return RangeRef(new Range(Anti, bitwidth, stmax + 1, stmin - 1));
   }
   
   if(stmin > stmax)
   {
      return RangeRef(new Range(Anti, bitwidth, stmax + 1, stmin - 1));
   }
   return RangeRef(new Range(Regular, bitwidth, stmin, stmax));
}

RangeRef Range::sextOrTrunc(bw_t bitwidth) const
{
   if(bitwidth <= bw)
   {
      return truncate(bitwidth);
   }
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }

   const auto this_min = this->getSignedMin();
   const auto this_max = this->getSignedMax();
   
   const auto [min, max] = std::minmax({this_min.extOrTrunc(bitwidth, true), this_max.extOrTrunc(bitwidth, true)});
   RangeRef sextRes(new Range(Regular, bitwidth, min, max));
   if(sextRes->isFullSet())
   {
      return RangeRef(new Range(Regular, bitwidth));
   }
   return sextRes;
}

RangeRef Range::zextOrTrunc(bw_t bitwidth) const
{
   if(bitwidth <= bw)
   {
      return truncate(bitwidth);
   }
   THROW_ASSERT(!isReal(), "Real range is a storage class only");
   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }
   if(this->getSignedMin() < 0 && this->getSignedMax() >= 0)
   {
      return RangeRef(new Range(Regular, bitwidth, 0, APInt::getMaxValue(bw)));
   }

   if(this->getSignedMax() < 0)
   {
      return RangeRef(new Range(Regular, bitwidth, this->getSignedMax().extOrTrunc(bw, false), this->getSignedMin().extOrTrunc(bw, false)));
   }
   return RangeRef(new Range(Regular, bitwidth, this->getSignedMin().extOrTrunc(bw, false), this->getSignedMax().extOrTrunc(bw, false)));
}

RangeRef Range::intersectWith(RangeConstRef other) const
{
   #ifdef DEBUG_RANGE_OP
   PRINT_MSG("intersectWith-this: " << *this << std::endl << "intersectWith-other: " << *other);
   #endif
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   if(!this->isAnti() && !other->isAnti())
   {
      APInt res_l = getLower() > other->getLower() ? getLower() : other->getLower();
      APInt res_u = getUpper() < other->getUpper() ? getUpper() : other->getUpper();
      if(res_u < res_l)
      {
         return RangeRef(new Range(Empty, bw));
      }

      return RangeRef(new Range(Regular, bw, res_l, res_u));
   }
   if(this->isAnti() && !other->isAnti())
   {
      auto antiRange = this->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      if(antil <= other->getLower())
      {
         if(other->getUpper() <= antiu)
         {
            return RangeRef(new Range(Empty, bw));
         }
         APInt res_l = other->getLower() > antiu ? other->getLower() : antiu + 1;
         APInt res_u = other->getUpper();
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(antiu >= other->getUpper())
      {
         THROW_ASSERT(other->getLower() < antil, "");
         APInt res_l = other->getLower();
         APInt res_u = other->getUpper() < antil ? other->getUpper() : antil - 1;
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(other->getLower() == Min && other->getUpper() == Max)
      {
         return RangeRef(this->clone());
      }
      if(antil > other->getUpper() || antiu < other->getLower())
      {
         return RangeRef(other->clone());
      }

      // we approximate to the range of other
      return RangeRef(other->clone());
   }
   if(!this->isAnti() && other->isAnti())
   {
      auto antiRange = other->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      if(antil <= this->getLower())
      {
         if(this->getUpper() <= antiu)
         {
            return RangeRef(new Range(Empty, bw));
         }
         APInt res_l = this->getLower() > antiu ? this->getLower() : antiu + 1;
         APInt res_u = this->getUpper();
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(antiu >= this->getUpper())
      {
         THROW_ASSERT(this->getLower() < antil, "");
         APInt res_l = this->getLower();
         APInt res_u = this->getUpper() < antil ? this->getUpper() : antil - 1;
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(this->getLower() == Min && this->getUpper() == Max)
      {
         return RangeRef(other->clone());
      }
      if(antil > this->getUpper() || antiu < this->getLower())
      {
         return RangeRef(this->clone());
      }

      // we approximate to the range of this
      return RangeRef(this->clone());
   }

   auto antiRange_a = this->getAnti();
   auto antiRange_b = other->getAnti();
   auto antil_a = antiRange_a->getLower();
   auto antiu_a = antiRange_a->getUpper();
   auto antil_b = antiRange_b->getLower();
   auto antiu_b = antiRange_b->getUpper();
   if(antil_a > antil_b)
   {
      std::swap(antil_a, antil_b);
      std::swap(antiu_a, antiu_b);
   }
   if(antil_b > (antiu_a + 1))
   {
      return RangeRef(new Range(Anti, bw, antil_a, antiu_a));
   }
   auto res_l = antil_a;
   auto res_u = antiu_a > antiu_b ? antiu_a : antiu_b;
   if(res_l == Min && res_u == Max)
   {
      return RangeRef(new Range(Empty, bw));
   }

   return RangeRef(new Range(Anti, bw, res_l, res_u));
}

RangeRef Range::unionWith(RangeConstRef other) const
{
   #ifdef DEBUG_RANGE_OP
   PRINT_MSG("unionWith-this: " << *this << std::endl << "unionWith-other: " << *other);
   #endif
   THROW_ASSERT(!isReal() && !other->isReal(), "Real range is a storage class only");
   if(this->isEmpty() || this->isUnknown())
   {
      return RangeRef(other->clone());
   }
   if(other->isEmpty() || other->isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(!this->isAnti() && !other->isAnti())
   {
      APInt res_l = getLower() < other->getLower() ? getLower() : other->getLower();
      APInt res_u = getUpper() > other->getUpper() ? getUpper() : other->getUpper();
      return RangeRef(new Range(Regular, bw, res_l, res_u));
   }
   if(this->isAnti() && !other->isAnti())
   {
      auto antiRange = this->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      THROW_ASSERT(antil != Min, "");
      THROW_ASSERT(antiu != Max, "");
      if(antil > other->getUpper() || antiu < other->getLower())
      {
         return RangeRef(this->clone());
      }
      if(antil > other->getLower() && antiu < other->getUpper())
      {
         return RangeRef(new Range(Regular, bw));
      }
      if(antil >= other->getLower() && antiu > other->getUpper())
      {
         return RangeRef(new Range(Anti, bw, other->getUpper() + 1, antiu));
      }
      if(antil < other->getLower() && antiu <= other->getUpper())
      {
         return RangeRef(new Range(Anti, bw, antil, other->getLower() - 1));
      }

      return RangeRef(new Range(Regular, bw)); // approximate to the full set
   }
   if(!this->isAnti() && other->isAnti())
   {
      auto antiRange = other->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      THROW_ASSERT(antil != Min, "");
      THROW_ASSERT(antiu != Max, "");
      if(antil > this->getUpper() || antiu < this->getLower())
      {
         return RangeRef(other->clone());
      }
      if(antil > this->getLower() && antiu < this->getUpper())
      {
         return RangeRef(new Range(Regular, bw));
      }
      if(antil >= this->getLower() && antiu > this->getUpper())
      {
         return RangeRef(new Range(Anti, bw, this->getUpper() + 1, antiu));
      }
      if(antil < this->getLower() && antiu <= this->getUpper())
      {
         return RangeRef(new Range(Anti, bw, antil, this->getLower() - 1));
      }

      return RangeRef(new Range(Regular, bw)); // approximate to the full set
   }

   auto antiRange_a = this->getAnti();
   auto antiRange_b = other->getAnti();
   auto antil_a = antiRange_a->getLower();
   auto antiu_a = antiRange_a->getUpper();
   THROW_ASSERT(antil_a != Min, "");
   THROW_ASSERT(antiu_a != Max, "");
   auto antil_b = antiRange_b->getLower();
   auto antiu_b = antiRange_b->getUpper();
   THROW_ASSERT(antil_b != Min, "");
   THROW_ASSERT(antiu_b != Max, "");
   if(antil_a > antiu_b || antiu_a < antil_b)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(antil_a > antil_b && antiu_a < antiu_b)
   {
      return RangeRef(this->clone());
   }
   if(antil_b > antil_a && antiu_b < antiu_a)
   {
      return RangeRef(this->clone());
   }
   if(antil_a >= antil_b && antiu_b <= antiu_a)
   {
      return RangeRef(new Range(Anti, bw, antil_a, antiu_b));
   }
   if(antil_b >= antil_a && antiu_a <= antiu_b)
   {
      return RangeRef(new Range(Anti, bw, antil_b, antiu_a));
   }

   THROW_UNREACHABLE("unsupported condition");
   return nullptr;
}

void Range::print(std::ostream& OS) const
{
   if(this->isUnknown())
   {
      OS << "[Unknown," << +bw << "]";
   }
   else if(this->isEmpty())
   {
      OS << "[Empty," << +bw << "]";
   }
   else if(this->isAnti())
   {
      if(l == Min)
      {
         OS << ")-inf,";
      }
      else
      {
         OS << ")" << l.str() << ",";
      }
      OS << +bw << ",";
      if(u == Max)
      {
         OS << "+inf(";
      }
      else
      {
         OS << u.str() << "(";
      }
   }
   else
   {
      if(l == Min)
      {
         OS << "[-inf,";
      }
      else
      {
         OS << "[" << l.str() << ",";
      }
      OS << +bw << ",";
      if(u == Max)
      {
         OS << "+inf]";
      }
      else
      {
         OS << u.str() << "]";
      }
   }
}

std::string Range::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

std::ostream& operator<<(std::ostream& OS, const Range& R)
{
   R.print(OS);
   return OS;
}

RangeRef Range::makeSatisfyingCmpRegion(kind pred, RangeConstRef Other)
{
   const auto bw = Other->bw;
   if(Other->isUnknown() || Other->isEmpty())
   {
      return RangeRef(Other->clone());
   }
   if(Other->isAnti() && pred != eq_expr_K && pred != ne_expr_K && pred != uneq_expr_K)
   {
      THROW_UNREACHABLE("Invalid request " + tree_node::GetString(pred) + " " + Other->ToString());
      return RangeRef(new Range(Empty, bw));
   }
   if(Other->isReal() && pred != eq_expr_K && pred != ne_expr_K && pred != uneq_expr_K)
   {
      THROW_UNREACHABLE("Compare region for real range not handled (" + tree_node::GetString(pred) + ")");
   }

   switch (pred)
   {
      case ge_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getSignedMax(), APInt::getSignedMaxValue(bw)));
      case gt_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getSignedMax() + MinDelta, APInt::getSignedMaxValue(bw)));
      case le_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw), Other->getSignedMin()));
      case lt_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw), Other->getSignedMin() - MinDelta));
      case unge_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getUnsignedMax(), APInt::getMaxValue(bw)));
      case ungt_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getUnsignedMax() + MinDelta, APInt::getMaxValue(bw)));
      case unle_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getMinValue(bw), Other->getUnsignedMin()));
      case unlt_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getMinValue(bw), Other->getUnsignedMin() - MinDelta));
      case uneq_expr_K:
      case eq_expr_K:
         return RangeRef(Other->clone());
      case ne_expr_K:
         return Other->getAnti();
   
      case assert_expr_K:case bit_and_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         break;
   }
   THROW_UNREACHABLE("Unhandled compare operation (" + STR(pred) + ")");
   return nullptr;
}

// ========================================================================== //
// RealRange
// ========================================================================== //
RealRange::RealRange(const Range& s, const Range& e, const Range& f) : Range(Real, static_cast<bw_t>(s.getBitWidth() + e.getBitWidth() + f.getBitWidth())), sign(s.clone()), exponent(e.clone()), fractional(f.clone())
{
   THROW_ASSERT(getBitWidth() == 32 || getBitWidth() == 64, "Composed range bitwidth not valid [" + s.ToString() + " " + e.ToString() + " " + f.ToString() + "]<" + STR(getBitWidth()) + ">");
   THROW_ASSERT(!s.isReal() && !e.isReal() && !f.isReal(), "Real range components shouldn't be real ranges");
}

RealRange::RealRange(RangeConstRef s, RangeConstRef e, RangeConstRef f) : Range(Real, static_cast<bw_t>(s->getBitWidth() + e->getBitWidth() + f->getBitWidth())), sign(s->clone()), exponent(e->clone()), fractional(f->clone())
{
   THROW_ASSERT(getBitWidth() == 32 || getBitWidth() == 64, "Composed range bitwidth not valid [" + s->ToString() + " " + e->ToString() + " " + f->ToString() + "]<" + STR(getBitWidth()) + ">");
   THROW_ASSERT(!s->isReal() && !e->isReal() && !f->isReal(), "Real range components shouldn't be real ranges");
}

RealRange::RealRange(RangeConstRef vc) : Range(Real, vc->getBitWidth()), sign(vc->Sgt(RangeRef(new Range(Regular, 1, 0, 0)), 1))
{
   if(vc->getBitWidth() == 32)
   {
      exponent = vc->shr(RangeRef(new Range(Regular, MAX_BIT_INT, 23, 23)), false)->zextOrTrunc(8);
      fractional = vc->zextOrTrunc(23);
   }
   else if(vc->getBitWidth() == 64)
   {
      exponent = vc->shr(RangeRef(new Range(Regular, MAX_BIT_INT, 52, 52)), false)->zextOrTrunc(11);
      fractional = vc->zextOrTrunc(52);
   }
   else
   {
      THROW_UNREACHABLE("Unhandled view convert bitwidth: could not transform " + vc->ToString() + " into real range");
   }
}

RangeRef RealRange::getRange() const
{
   const auto _bw = getBitWidth();
   if(_bw == 32)
   {
      auto s = sign->zextOrTrunc(32)->shl(RangeRef(new Range(Regular, MAX_BIT_INT, 31, 31)));
      auto e = exponent->zextOrTrunc(32)->shl(RangeRef(new Range(Regular, MAX_BIT_INT, 23, 23)));
      return fractional->zextOrTrunc(32)->Or(e)->Or(s);
   }
   else if(_bw == 64)
   {
      auto s = sign->zextOrTrunc(64)->shl(RangeRef(new Range(Regular, MAX_BIT_INT, 63, 63)));
      auto e = exponent->zextOrTrunc(64)->shl(RangeRef(new Range(Regular, MAX_BIT_INT, 52, 52)));
      return fractional->zextOrTrunc(64)->Or(e)->Or(s);
   }
   THROW_UNREACHABLE("Unhandled view convert bitwidth");
   return nullptr;
}

RangeRef RealRange::getSign() const
{
   return sign;
}

RangeRef RealRange::getExponent() const
{
   return exponent;
}

RangeRef RealRange::getFractional() const
{
   return fractional;
}

RangeRef RealRange::getAnti() const
{
   return RangeRef(new RealRange(sign->getAnti(), exponent->getAnti(), fractional->getAnti()));
}

void RealRange::setSign(RangeConstRef s)
{
   sign.reset(s->clone());
}

void RealRange::setExponent(RangeConstRef e)
{
   exponent.reset(e->clone());
}

void RealRange::setFractional(RangeConstRef f)
{
   fractional.reset(f->clone());
}

bool RealRange::isSameRange(RangeConstRef other) const
{
   if(other->isReal())
   {
      auto rOther = RefcountCast<const RealRange>(other);
      return this->getBitWidth() == other->getBitWidth() && sign->isSameRange(rOther->sign) && exponent->isSameRange(rOther->exponent) && fractional->isSameRange(rOther->fractional);
   }
   return false;
}

bool RealRange::isEmpty() const
{
   return sign->isEmpty() || exponent->isEmpty() || fractional->isEmpty();
}

bool RealRange::isUnknown() const
{
   return sign->isUnknown() || exponent->isUnknown() || fractional->isUnknown();
}

void RealRange::setUnknown()
{
   sign->setUnknown();
   exponent->setUnknown();
   fractional->setUnknown();
}

bool RealRange::isFullSet() const
{
   return sign->isFullSet() && exponent->isFullSet() && fractional->isFullSet();
}

bool RealRange::isSingleElement() const
{
   return sign->isSingleElement() && exponent->isSingleElement() && fractional->isSingleElement();
}

bool RealRange::isConstant() const
{
   return sign->isConstant() && exponent->isConstant() && fractional->isConstant();
}

bool RealRange::isReal() const
{
   return true;
}

void RealRange::print(std::ostream& OS) const
{
   OS << "[ ";
   sign->print(OS);
   OS << ", ";
   exponent->print(OS);
   OS << ", ";
   fractional->print(OS);
   OS << ", " << STR(getBitWidth()) << "]";
}

Range* RealRange::clone() const
{
   return new RealRange(sign, exponent, fractional);
}

RangeRef RealRange::abs() const
{
   return RangeRef(new RealRange(RangeRef(new Range(Regular, 1, 0, 0)), exponent, fractional));
}

RangeRef RealRange::negate() const
{
   if(sign->isAnti() || sign->isConstant())
   {
      const auto s = sign->getUnsignedMin() ? 0 : 1;
      return RangeRef(new RealRange(RangeRef(new Range(Regular, 1, s, s)), exponent, fractional));
   }
   return RangeRef(this->clone());
}

RangeRef RealRange::Eq(RangeConstRef other, bw_t _bw) const
{
   if(const auto rOther = RefcountCast<const RealRange>(other))
   {
      return sign->Eq(rOther->sign, _bw)->intersectWith(exponent->Eq(rOther->exponent, _bw))->intersectWith(fractional->Eq(rOther->fractional, _bw));
   }
   return RangeRef(new Range(Regular, _bw, 0, 0));
}

RangeRef RealRange::Ne(RangeConstRef other, bw_t _bw) const
{
   if(const auto rOther = RefcountCast<const RealRange>(other))
   {
      return sign->Ne(rOther->sign, _bw)->unionWith(exponent->Ne(rOther->exponent, _bw))->unionWith(fractional->Ne(rOther->fractional, _bw));
   }
   return RangeRef(new Range(Regular, _bw, 1, 1));
}

RangeRef RealRange::intersectWith(RangeConstRef other) const
{
   #ifdef DEBUG_RANGE_OP
   PRINT_MSG("intersectWith-this: " << *this << std::endl << "intersectWith-other: " << *other);
   #endif
   THROW_ASSERT(other->isReal(), "Real range should intersect with real range only");
   auto rrOther = RefcountCast<const RealRange>(other);
   return RangeRef(new RealRange(sign->intersectWith(rrOther->sign), exponent->intersectWith(rrOther->exponent), fractional->intersectWith(rrOther->fractional)));
}

RangeRef RealRange::unionWith(RangeConstRef other) const
{
   #ifdef DEBUG_RANGE_OP
   PRINT_MSG("unionWith-this: " << *this << std::endl << "unionWith-other: " << *other);
   #endif
   THROW_ASSERT(other->isReal(), "Real range should unite to real range only");
   auto rrOther = RefcountCast<const RealRange>(other);
   return RangeRef(new RealRange(sign->unionWith(rrOther->sign), exponent->unionWith(rrOther->exponent), fractional->unionWith(rrOther->fractional)));
}

RangeRef RealRange::toFloat64() const
{
   if(getBitWidth() == 64)
   {
      return RangeRef(this->clone());
   }

   RangeRef exponent64;
   // Denormalized case
   if(exponent->isConstant() && exponent->getUnsignedMin() == 0)
   {
      exponent64.reset(new Range(Regular, 11, 0, 0));
   }
   else if(exponent->isFullSet())
   {
      exponent64.reset(new Range(Regular, 11));
   }
   else
   {
      exponent64.reset(new Range(Regular, 11, (exponent->getUnsignedMin() + 896).extOrTrunc(11, true), (exponent->getUnsignedMax() + 896).extOrTrunc(11, true)));
   }

   return RangeRef(new RealRange(sign, 
      exponent64, 
      fractional->zextOrTrunc(52)->shl(RangeRef(new Range(Regular, 52, 29, 29)))));
}

RangeRef RealRange::toFloat32() const
{
   if(getBitWidth() == 32)
   {
      return RangeRef(this->clone());
   }

   RangeRef exponent32;
   RangeRef fractional32 = fractional->shr(RangeRef(new Range(Regular, 52, 29, 29)), false)->zextOrTrunc(23);
   const auto min = exponent->getUnsignedMin() - 896;
   const auto max = exponent->getUnsignedMax() - 896;
   if(min < 0 && max > APInt::getMaxValue(8))
   {
      exponent32.reset(new Range(Regular, 8));
   }
   else if(exponent->isConstant() && min < 0)
   {
      exponent32.reset(new Range(Regular, 8, 0, 0));
   }
   else if(exponent->isConstant() && max >= APInt::getMaxValue(8))
   {
      exponent32.reset(new Range(Regular, 8, -1, -1));
      fractional32.reset(new Range(Regular, 23, 0, 0));
   }
   else
   {
      exponent32.reset(new Range(Regular, 8, std::max({min, APInt(0)}).extOrTrunc(8, true), std::min({max, APInt(255)}).extOrTrunc(8, true)));
   }

   return RangeRef(new RealRange(sign, 
      exponent32,
      fractional32));
}

namespace 
{
   // ========================================================================== //
   // Static global functions and definitions
   // ========================================================================== //

   // Used to print pseudo-edges in the Constraint Graph dot
   std::string pestring;
   std::stringstream pseudoEdgesString(pestring);

   std::tuple<bool, uint8_t, uint32_t> float_view_convert(float fp)
   {
      union vcFloat _flo;
      _flo.flt = (fp);
      #if __BYTE_ORDER == __BIG_ENDIAN
         uint32_t f = _flo.bits.frac;
         uint8_t e = _flo.bits.exp;
         bool s = _flo.bits.sign;
      #else
         uint32_t f = (_flo.bits.coded) & 0b00000000011111111111111111111111;
         uint8_t e = static_cast<uint8_t>(((_flo.bits.coded) << 1) >> 24);
         bool s = (_flo.bits.coded) & 0x80000000;
      #endif
      return {s, e, f};
   }

   std::tuple<bool, uint16_t, uint64_t> double_view_convert(double d)
   {
      union vcDouble _d;
      _d.dub = (d);
      #if __BYTE_ORDER == __BIG_ENDIAN
         uint64_t f = _d.bits.frac;
         uint16_t e = _d.bits.exp;
         bool s = _d.bits.sign;
      #else
         uint64_t f = (_d.bits.coded) & 0b0000000000001111111111111111111111111111111111111111111111111111;
         uint16_t e = static_cast<uint16_t>(((_d.bits.coded) << 1) >> 53);
         bool s = (_d.bits.coded) & 0x8000000000000000;
      #endif
      return {s, e, f};
   }

   #ifdef BITVALUE_UPDATE
   std::string range_to_bits(const long long min, const long long max, bw_t bw)
   {
      long long mix = min | max;
      long long mask = 1LL << std::min(std::numeric_limits<long long>::digits, static_cast<int>(bw - 1));
      std::stringstream bits;
      while (bw)
      {
         if(mix & mask)
         {
            break;
         }
         bits << '0';
         mask >>= 1;
         --bw;
      }
      for(;bw > 0; --bw)
      {
         bits << 'U';
      }
      return bits.str();
   }
   #endif

   kind op_unsigned(kind op)
   {
      switch (op)
      {
         case ge_expr_K:
            return unge_expr_K;
         case gt_expr_K:
            return ungt_expr_K;
         case le_expr_K:
            return unle_expr_K;
         case lt_expr_K:
            return unlt_expr_K;
         case eq_expr_K:
            return uneq_expr_K;
         case unge_expr_K:
         case ungt_expr_K:
         case unle_expr_K:
         case unlt_expr_K:
         case uneq_expr_K:
         case ne_expr_K:
            return op;

         case assert_expr_K:case bit_and_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
         case CASE_UNARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_GIMPLE_NODES:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_MISCELLANEOUS:
         default:
            break;
      }
      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   kind op_inv(kind op)
   {
      switch (op)
      {
      case ge_expr_K:
         return lt_expr_K;
      case gt_expr_K:
         return le_expr_K;
      case le_expr_K:
         return gt_expr_K;
      case lt_expr_K:
         return ge_expr_K;
      case unge_expr_K:
         return unlt_expr_K;
      case ungt_expr_K:
         return unle_expr_K;
      case unle_expr_K:
         return ungt_expr_K;
      case unlt_expr_K:
         return unge_expr_K;
      case eq_expr_K:
      case uneq_expr_K:
         return ne_expr_K;
      case ne_expr_K:
         return eq_expr_K;

      case assert_expr_K:case bit_and_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         break;
      }

      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   kind op_swap(kind op)
   {
      switch (op)
      {
         case ge_expr_K:
            return le_expr_K;
         case gt_expr_K:
            return lt_expr_K;
         case le_expr_K:
            return ge_expr_K;
         case lt_expr_K:
            return gt_expr_K;
         case unge_expr_K:
            return unle_expr_K;
         case ungt_expr_K:
            return unlt_expr_K;
         case unle_expr_K:
            return unge_expr_K;
         case unlt_expr_K:
            return ungt_expr_K;

         case bit_and_expr_K:
         case bit_ior_expr_K:
         case bit_xor_expr_K:
         case eq_expr_K:
         case ne_expr_K:
         case uneq_expr_K:
            return op;
         
         case assert_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
         case CASE_UNARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_GIMPLE_NODES:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_MISCELLANEOUS:
         default:
            break;
      }

      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   bool isCompare(kind c_type)
   {
      return c_type == eq_expr_K || c_type == ne_expr_K || c_type == ltgt_expr_K || c_type == uneq_expr_K
         || c_type == gt_expr_K || c_type == lt_expr_K || c_type == ge_expr_K || c_type == le_expr_K 
         || c_type == unlt_expr_K || c_type == ungt_expr_K || c_type == unle_expr_K || c_type == unge_expr_K;
}

   bool isCompare(const struct binary_expr* condition)
   {
      return isCompare(condition->get_kind());
   }

   tree_nodeConstRef branchOpRecurse(const tree_nodeConstRef op)
   {
      if(const auto* nop = GetPointer<const nop_expr>(op))
      {
         return branchOpRecurse(nop->op);
      }
      else if(const auto* ce = GetPointer<const convert_expr>(op))
      {
         return branchOpRecurse(ce->op);
      }
      else if(const auto* ssa = GetPointer<const ssa_name>(op))
      {
         const auto DefStmt = GET_CONST_NODE(ssa->CGetDefStmt());
         if(const auto* gp = GetPointer<const gimple_phi>(DefStmt))
         {
            const auto& defEdges = gp->CGetDefEdgesList();
            THROW_ASSERT(not defEdges.empty(), "Branch variable definition from nowhere");
            return defEdges.size() > 1 ? DefStmt : branchOpRecurse(defEdges.front().first);
         }
         else if(const auto* ga = GetPointer<const gimple_assign>(DefStmt))
         {
            return branchOpRecurse(ga->op1);
         }
         else if(GetPointer<const gimple_nop>(DefStmt) != nullptr)
         {
            // Branch variable is a function parameter
            return DefStmt;
         }
         THROW_UNREACHABLE("Branch var definition statement not handled (" + DefStmt->get_kind_text() + " " + DefStmt->ToString() + ")");
      }
      else if(op->get_kind() == tree_reindex_K)
      {
         return branchOpRecurse(GET_CONST_NODE(op));
      }
      return op;
   }

   // Print name of variable according to its type
   void printVarName(const tree_nodeConstRef V, std::ostream& OS)
   {
      OS << GET_CONST_NODE(V)->ToString();
   }

   bool isValidType(const tree_nodeConstRef tn) 
   {
      switch(tn->get_kind())
      {
         case boolean_type_K:
         case enumeral_type_K:
         case integer_type_K:
         #ifdef INTEGER_PTR
         case pointer_type_K:
         #endif
         case real_type_K:
            return true;
         case array_type_K:
         case CharType_K:
         case nullptr_type_K:
         case type_pack_expansion_K:
         case complex_type_K:
         case function_type_K:
         case lang_type_K:
         case method_type_K:
         case offset_type_K:
         #ifndef INTEGER_PTR
         case pointer_type_K:
         #endif
         case qual_union_type_K:
         case record_type_K:
         case reference_type_K:
         case set_type_K:
         case template_type_parm_K:
         case typename_type_K:
         case type_argument_pack_K:
         case union_type_K:
         case vector_type_K:
         case void_type_K:
            return false;
         case tree_reindex_K:
            return isValidType(GET_CONST_NODE(tn));
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case ssa_name_K:
            return isValidType(tree_helper::CGetType(tn));
         case aggr_init_expr_K:case case_label_expr_K:case lut_expr_K:case target_expr_K:case target_mem_ref_K:case target_mem_ref461_K:case binfo_K:case block_K:case constructor_K:case error_mark_K:case identifier_node_K:case statement_list_K:case tree_list_K:case tree_vec_K:case call_expr_K:
         case last_tree_K:case none_K:case placeholder_expr_K:
         case CASE_UNARY_EXPRESSION:
         case CASE_BINARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_GIMPLE_NODES:
         default:
            THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
      }
      return false;
   }

   bool isValidInstruction(const tree_nodeConstRef stmt, const FunctionBehaviorConstRef FB, const tree_managerConstRef TM)
   {
      tree_nodeConstRef Type = nullptr;
      switch(GET_CONST_NODE(stmt)->get_kind())
      {
         case gimple_assign_K:
         {
            auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
            if(tree_helper::IsLoad(TM, stmt, FB->get_function_mem()))
            {
               Type = tree_helper::CGetType(GET_CONST_NODE(ga->op0));
               break;
            }
            else if(tree_helper::IsStore(TM, stmt, FB->get_function_mem()))
            {
               Type = tree_helper::CGetType(GET_CONST_NODE(ga->op1));
               break;
            }
            Type = tree_helper::CGetType(GET_CONST_NODE(ga->op0));
            
            switch(GET_CONST_NODE(ga->op1)->get_kind())
            {
               /// unary_expr cases
               case nop_expr_K:
               case abs_expr_K:
               case bit_not_expr_K:
               case convert_expr_K:
               case negate_expr_K:
               case view_convert_expr_K:
               {
                  const auto* ue = GetPointer<const unary_expr>(GET_CONST_NODE(ga->op1));
                  if(GetPointer<const expr_node>(GET_CONST_NODE(ue->op)))
                  {
                     // Nested operations not supported
                     return false;
                  }
                  break;
               }

               /// binary_expr cases
               case plus_expr_K:
               case minus_expr_K:
               case mult_expr_K:
               case trunc_div_expr_K:
               case trunc_mod_expr_K:
               case lshift_expr_K:
               case rshift_expr_K:
               case bit_and_expr_K:
               case bit_ior_expr_K:
               case bit_xor_expr_K:
               case eq_expr_K:
               case ne_expr_K:
               case unge_expr_K:
               case ungt_expr_K:
               case unlt_expr_K:
               case unle_expr_K:
               case gt_expr_K:
               case ge_expr_K:
               case lt_expr_K:
               case le_expr_K:
               #ifdef INTEGER_PTR
               case pointer_plus_expr_K:
               #endif
               case min_expr_K:
               case max_expr_K:
               {
                  const auto bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
                  if(!isValidType(bin_op->op0) || !isValidType(bin_op->op1))
                  {
                     return false;
                  }
                  break;
               }

               /// ternary_expr case
               case cond_expr_K:
                  break;

               case ssa_name_K:
               {
                  if(!isValidType(GET_CONST_NODE(ga->op1)))
                  {
                     return false;
                  }
                  break;
               }

               // Unary case
               case addr_expr_K:case paren_expr_K:case arrow_expr_K:case buffer_ref_K:case card_expr_K:case cleanup_point_expr_K:case conj_expr_K:case exit_expr_K:case fix_ceil_expr_K:case fix_floor_expr_K:case fix_round_expr_K:case fix_trunc_expr_K:case float_expr_K:case imagpart_expr_K:case indirect_ref_K:case misaligned_indirect_ref_K:case loop_expr_K:case non_lvalue_expr_K:case realpart_expr_K:case reference_expr_K:case reinterpret_cast_expr_K:case sizeof_expr_K:case static_cast_expr_K:case throw_expr_K:case truth_not_expr_K:case unsave_expr_K:case va_arg_expr_K:case reduc_max_expr_K:case reduc_min_expr_K:case reduc_plus_expr_K:case vec_unpack_hi_expr_K:case vec_unpack_lo_expr_K:case vec_unpack_float_hi_expr_K:case vec_unpack_float_lo_expr_K:
               // Binary case
               #ifndef INTEGER_PTR
               case pointer_plus_expr_K:
               #endif
               case assert_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case mem_ref_K:case modify_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case set_le_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case uneq_expr_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
               // Ternary case
               case component_ref_K:case bit_field_ref_K:case bit_ior_concat_expr_K:case vtable_ref_K:case with_cleanup_expr_K:case obj_type_ref_K:case save_expr_K:case vec_cond_expr_K:case vec_perm_expr_K:case dot_prod_expr_K:case ternary_plus_expr_K:case ternary_pm_expr_K:case ternary_mp_expr_K:case ternary_mm_expr_K:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_GIMPLE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_CPP_NODES:
               case aggr_init_expr_K:case case_label_expr_K:case lut_expr_K:case target_expr_K:case target_mem_ref_K:case target_mem_ref461_K:case binfo_K:case block_K:case constructor_K:case error_mark_K:case identifier_node_K:case statement_list_K:case tree_list_K:case tree_vec_K:case call_expr_K:
               default:
                  return false;
            }
         }
         break;

         case gimple_phi_K:
         {
            const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
            Type = tree_helper::CGetType(GET_CONST_NODE(phi->res));
         }
         break;

         case gimple_asm_K:
         case gimple_bind_K:
         case gimple_call_K:
         case gimple_cond_K:
         case gimple_for_K:
         case gimple_goto_K:
         case gimple_label_K:
         case gimple_multi_way_if_K:
         case gimple_nop_K:
         case gimple_pragma_K:
         case gimple_predict_K:
         case gimple_resx_K:
         case gimple_return_K:
         case gimple_switch_K:
         case gimple_while_K:
         case CASE_UNARY_EXPRESSION:
         case CASE_BINARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_MISCELLANEOUS:
         default:
            return false;
      }
      return isValidType(Type);
   }

   bool isSignedType(const tree_nodeConstRef tn)
   {
      switch(tn->get_kind())
      {
         case enumeral_type_K:
            return !GetPointer<const enumeral_type>(tn)->unsigned_flag;
         case integer_type_K:
            return !GetPointer<const integer_type>(tn)->unsigned_flag;
         case boolean_type_K:
         case array_type_K:
         case CharType_K:
         case nullptr_type_K:
         case type_pack_expansion_K:
         case complex_type_K:
         case function_type_K:
         case lang_type_K:
         case method_type_K:
         case offset_type_K:
         case pointer_type_K:
         case qual_union_type_K:
         case real_type_K:
         case record_type_K:
         case reference_type_K:
         case set_type_K:
         case template_type_parm_K:
         case typename_type_K:
         case union_type_K:
         case vector_type_K:
         case void_type_K:
         case type_argument_pack_K:
            return false;
         case tree_reindex_K:
            return isSignedType(GET_CONST_NODE(tn));
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case ssa_name_K:
            return isSignedType(tree_helper::CGetType(tn));
         case aggr_init_expr_K:case case_label_expr_K:case lut_expr_K:case target_expr_K:case target_mem_ref_K:case target_mem_ref461_K:case binfo_K:case block_K:case constructor_K:case error_mark_K:case identifier_node_K:case statement_list_K:case tree_list_K:case tree_vec_K:case call_expr_K:
         case last_tree_K:case none_K:case placeholder_expr_K:
         case CASE_UNARY_EXPRESSION:
         case CASE_BINARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_GIMPLE_NODES:
         default:
            THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
      }
      return true;
   }

   tree_nodeConstRef getGIMPLE_Type(const tree_nodeConstRef tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getGIMPLE_Type(GET_CONST_NODE(tn));
      }
      if(const auto* gp = GetPointer<const gimple_phi>(tn))
      {
         return getGIMPLE_Type(GET_CONST_NODE(gp->res));
      }
      if(const auto* gr = GetPointer<const gimple_return>(tn))
      {
         return getGIMPLE_Type(GET_CONST_NODE(gr->op));
      }
      return tree_helper::CGetType(tn);
   }

   bw_t getGIMPLE_BW(const tree_nodeConstRef tn)
   {
      const auto type = getGIMPLE_Type(tn);
      auto size = tree_helper::Size(type);
      THROW_ASSERT(static_cast<bool>(size), "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
      return static_cast<bw_t>(size);
   }

   RangeRef getUnknownFor(const tree_nodeConstRef tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getUnknownFor(GET_CONST_NODE(tn));
      }

      const auto type = getGIMPLE_Type(tn);
      bw_t bw = static_cast<bw_t>(tree_helper::Size(type));

      if(GetPointer<const real_type>(type) != nullptr)
      {
         return RangeRef(new RealRange(RangeRef(new Range(Unknown, bw))));
      }
      return RangeRef(new Range(Unknown, bw));
   }

   RangeRef getEmptyFor(const tree_nodeConstRef tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getEmptyFor(GET_CONST_NODE(tn));
      }

      const auto type = getGIMPLE_Type(tn);
      bw_t bw = static_cast<bw_t>(tree_helper::Size(type));

      if(GetPointer<const real_type>(type) != nullptr)
      {
         return RangeRef(new RealRange(RangeRef(new Range(Empty, bw))));
      }
      return RangeRef(new Range(Empty, bw));
   }

   RangeRef getFullRangeFor(const tree_nodeConstRef tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getFullRangeFor(GET_CONST_NODE(tn));
      }

      const auto type = getGIMPLE_Type(tn);
      bw_t bw = static_cast<bw_t>(tree_helper::Size(type));

      if(GetPointer<const real_type>(type) != nullptr)
      {
         return RangeRef(new RealRange(RangeRef(new Range(Regular, bw))));
      }
      return RangeRef(new Range(Regular, bw));
   }

   RangeRef getGIMPLE_range(const tree_nodeConstRef tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getGIMPLE_range(GET_CONST_NODE(tn));
      }

      const auto type = getGIMPLE_Type(tn);
      bw_t bw = static_cast<bw_t>(tree_helper::Size(type));
      #ifdef BITVALUE_UPDATE
      bool sign = false;
      #endif
      THROW_ASSERT(static_cast<bool>(bw), "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
      APInt min, max;
      if(const auto* ic = GetPointer<const integer_cst>(tn))
      {
         min = max = tree_helper::get_integer_cst_value(ic);
         return RangeRef(new Range(Regular, bw, min, max));
      }
      else if(const auto* sc = GetPointer<const string_cst>(tn))
      {
         bw = 8;
         RangeRef r(new Range(Empty, bw));
         for(const auto& c : sc->strg)
         {
            r = r->unionWith(RangeRef(new Range(Regular, bw, c, c)));
         }
         return r;
      }
      else if(const auto* it = GetPointer<const integer_type>(type))
      {
         #ifdef BITVALUE_UPDATE
         sign = !it->unsigned_flag;
         #endif
         min = it->unsigned_flag ? APInt::getMinValue(bw) : APInt::getSignedMinValue(bw);
         max = it->unsigned_flag ? APInt::getMaxValue(bw) : APInt::getSignedMaxValue(bw);
      }
      else if(const auto* et = GetPointer<const enumeral_type>(type))
      {
         #ifdef BITVALUE_UPDATE
         sign = !et->unsigned_flag;
         #endif
         min = et->unsigned_flag ? APInt::getMinValue(bw) : APInt::getSignedMinValue(bw);
         max = et->unsigned_flag ? APInt::getMaxValue(bw) : APInt::getSignedMaxValue(bw);
      }
      else if(type->get_kind() == boolean_type_K)
      {
         min = 0;
         max = 1;
         bw = 1;
      }
      else if(const auto* rc = GetPointer<const real_cst>(tn))
      {
         auto val = strtof64x(rc->valr.data(), nullptr);
         if(bw == 32)
         {
            auto [s, e, f] = float_view_convert(static_cast<float>(val));
            return RangeRef(new RealRange(Range(Regular, 1, s, s), Range(Regular, 8, e, e), Range(Regular, 23, f, f)));
         }
         if(bw == 64)
         {
            auto [s, e, f] = double_view_convert(static_cast<double>(val));
            return RangeRef(new RealRange(Range(Regular, 1, s, s), Range(Regular, 11, e, e), Range(Regular, 52, f, f)));
         }
         THROW_UNREACHABLE("Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
      }
      else if(GetPointer<const real_type>(type) != nullptr)
      {
         if(bw == 32)
         {
            return RangeRef(new RealRange(Range(Regular, 1), Range(Regular, 8), Range(Regular, 23)));
         }
         if(bw == 64)
         {
            return RangeRef(new RealRange(Range(Regular, 1), Range(Regular, 11), Range(Regular, 52)));
         }
         THROW_UNREACHABLE("Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
      }
      #ifdef INTEGER_PTR
      else if(GetPointer<const pointer_type>(type) != nullptr)
      {
         min = APInt::getMinValue(bw);
         max = APInt::getMaxValue(bw);
      }
      #endif
      else 
      {
         THROW_UNREACHABLE("Unable to define range for type " + type->get_kind_text() + " of " + tn->ToString());
      }

      #ifdef BITVALUE_UPDATE
      if(const auto* ssa = GetPointer<const ssa_name>(tn))
      {
         if(!ssa->bit_values.empty())
         {
            const auto bitValues = string_to_bitstring(ssa->bit_values);
            THROW_ASSERT(bitValues.size() > std::numeric_limits<bw_t>::max(), "Bitwidth too wide for the implementation");
            for(bw_t i = 0U; i < bitValues.size(); ++i)
            {
               switch(bitValues.at(bitValues.size() - i - 1))
               {
                  case bit_lattice::ZERO:
                     min.bit_clr(i);
                     max.bit_clr(i);
                     break;
                  case bit_lattice::ONE:
                     min.bit_set(i);
                     max.bit_set(i);
                     break;
                  case bit_lattice::U:
                  case bit_lattice::X:
                  default:
                     break;
               }
            }
            min = min.extOrTrunc(bw, sign);
            max = max.extOrTrunc(bw, sign);
         }
      }
      #endif
      return RangeRef(new Range(Regular, bw, min, max));
   }
}


// ========================================================================== //
// VarNode
// ========================================================================== //
class VarNode
{
   /// The program variable
   const tree_nodeConstRef V;
   /// ID of the associated function
   unsigned int function_id;
   /// A Range associated to the variable, that is,
   /// its interval inferred by the analysis.
   RangeRef interval;
   /// Used by the crop meet operator
   char abstractState;

 public:
   explicit VarNode(const tree_nodeConstRef& _V, unsigned int _function_id);
   ~VarNode() = default;
   VarNode(const VarNode&) = delete;
   VarNode(VarNode&&) = delete;
   VarNode& operator=(const VarNode&) = delete;
   VarNode& operator=(VarNode&&) = delete;

   /// Initializes the value of the node.
   void init(bool outside);
   /// Returns the range of the variable represented by this node.
   RangeRef getRange() const
   {
      return interval;
   }
   /// Returns the variable represented by this node.
   const tree_nodeConstRef& getValue() const
   {
      return V;
   }
   unsigned int getFunctionId() const
   {
      return function_id;
   }
   bw_t getBitWidth() const
   {
      return interval->getBitWidth();
   }
   /// Changes the status of the variable represented by this node.
   void setRange(const RangeConstRef& newInterval)
   {
      interval.reset(newInterval->clone());
   }

   RangeRef getMaxRange() const
   {
      return getFullRangeFor(V);
      }

   char getAbstractState()
   {
      return abstractState;
   }
   // The possible states are '0', '+', '-' and '?'.
   void storeAbstractState();

   bool updateIR(tree_managerRef TM, tree_manipulationRef tree_man
   #ifndef NDEBUG
   , int debug_level
   #endif
   );

   /// Pretty print.
   void print(std::ostream& OS) const;
   std::string ToString() const;
};

/// The ctor.
VarNode::VarNode(const tree_nodeConstRef& _V, unsigned int _function_id) : V(_V), function_id(_function_id), abstractState(0)
{
   THROW_ASSERT(_V != nullptr, "Variable cannot be null");
   THROW_ASSERT(_V->get_kind() == tree_reindex_K, "Variable should be a tree_reindex node");
   interval = getUnknownFor(_V);
}

/// Initializes the value of the node.
void VarNode::init(bool outside)
{
   const auto bw = getGIMPLE_BW(V);
   THROW_ASSERT(bw, "Bitwidth not valid");
   if(GET_CONST_NODE(V)->get_kind() == integer_cst_K || GET_CONST_NODE(V)->get_kind() == real_cst_K)
   {
      interval = getGIMPLE_range(V);
   }
   else
   {
      if(getGIMPLE_Type(V)->get_kind() == real_type_K)
      {
         THROW_ASSERT(bw == 64 || bw == 32, "Bitwidth not allowed for floating point variable");
         if(!outside)
         {
            // Initialize with a basic, unknown, interval.
            interval.reset(new RealRange(RangeRef(new Range(Unknown, bw))));
         }
         else
         {
            interval.reset(new RealRange(RangeRef(new Range(Regular, bw))));
         }
      }
      else
      {
         if(!outside)
         {
            // Initialize with a basic, unknown, interval.
            interval.reset(new Range(Unknown, bw));
         }
         else
         {
            interval.reset(new Range(Regular, bw));
         }
      }
   }
}

bool VarNode::updateIR(tree_managerRef TM, tree_manipulationRef tree_man
   #ifndef NDEBUG
   , int debug_level
   #endif
   )
{
   const auto ssa_node = TM->GetTreeReindex(GET_INDEX_CONST_NODE(V));
   auto* SSA = GetPointer<ssa_name>(GET_NODE(ssa_node));
   if(SSA == nullptr)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Constant node " + GET_CONST_NODE(V)->ToString());
      return false;
   }
   if(interval->isUnknown())
   {
      return false;
   }

   const bool isSigned = isSignedType(SSA->type);
   if(SSA->range != nullptr)
   {
      if(SSA->range->isSameRange(interval))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Fixed range " + SSA->range->ToString() + " for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
         return false;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         "Modified range " + SSA->range->ToString() + " to " + interval->ToString() + " for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
   }
   else
   {
      bw_t newBW = interval->getBitWidth();
      if(interval->isFullSet())
      {
         return false;
      }
      else if(interval->isConstant())
      {
         newBW = 0U;
      }
      else if(!interval->isReal())
      {
         if(interval->isRegular())
         {
            newBW = isSigned ? Range::neededBits(interval->getSignedMin(), interval->getSignedMax(), true) : Range::neededBits(interval->getUnsignedMin(), interval->getUnsignedMax(), false);
            const auto currentBW = getGIMPLE_BW(V);
            if(newBW >= currentBW)
            {
               return false;
            }
         }
         else if(interval->isAnti() || interval->isEmpty())
         {
            return false;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Added range " + interval->ToString() + "<" + STR(newBW) + "> for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
   }

   auto getConstNode = [&] (RangeRef range) {
      long long cst_val;
      tree_nodeRef cst;
      if(range->isReal())
      {
         const auto rRange = RefcountCast<const RealRange>(range);
         if(rRange->getBitWidth() == 32)
         {
            union vcFloat vc;
            #if __BYTE_ORDER == __BIG_ENDIAN
            vc.bits.sign = rRange->getSign()->getLower().to<bool>();
            vc.bits.exp = rRange->getExponent()->getLower().to<uint8_t>();
            vc.bits.frac = rRange->getFractional()->getLower().to<uint32_t>();
            #else
            vc.bits.coded = rRange->getSign()->getUnsignedMax().to<uint32_t>() << 31;
            vc.bits.coded += rRange->getExponent()->getUnsignedMax().to<uint32_t>() << 23;
            vc.bits.coded += rRange->getFractional()->getUnsignedMax().to<uint32_t>();
            #endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating point constant from range is " + STR(vc.flt));
            cst_val = static_cast<int64_t>(vc.bits.coded);
            cst = tree_man->CreateRealCst(SSA->type, static_cast<long double>(vc.flt), TM->new_tree_node_id());
         }
         else
         {
            union vcDouble vc;
            #if __BYTE_ORDER == __BIG_ENDIAN
            vc.bits.sign = rRange->getSign()->getLower().to<bool>();
            vc.bits.exp = rRange->getExponent()->getLower().to<uint16_t>();
            vc.bits.frac = rRange->getFractional()->getLower().to<uint64_t>();
            #else
            vc.bits.coded = rRange->getSign()->getUnsignedMax().to<uint64_t>() << 63;
            vc.bits.coded += rRange->getExponent()->getUnsignedMax().to<uint64_t>() << 52;
            vc.bits.coded += rRange->getFractional()->getUnsignedMax().to<uint64_t>();
            #endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Double precision constant from range is " + STR(vc.dub));
            cst_val = static_cast<int64_t>(vc.bits.coded);
            cst = tree_man->CreateRealCst(SSA->type, static_cast<long double>(vc.dub), TM->new_tree_node_id());
         }
      }
      else
      {
         const auto cst_value = isSigned ? range->getSignedMax().to<int64_t>() : static_cast<int64_t>(range->getUnsignedMax().to<uint64_t>());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + (isSigned ? ("Signed int " + STR(cst_value)) : ("Unsigned int " + STR(static_cast<uint64_t>(cst_value)))));
         cst_val = cst_value;
         cst = tree_man->CreateIntegerCst(SSA->type, cst_value, TM->new_tree_node_id());
      }
      return std::make_pair(cst_val, cst);
   };

   auto resetRange = [] (ssa_name* ssa, bw_t
      #ifdef BITVALUE_UPDATE
      bw
      #endif
      )
   {
      ssa->max.reset();
      ssa->min.reset();
      ssa->range.reset();
      #ifdef BITVALUE_UPDATE
      ssa->bit_values = bitstring_to_string(create_u_bitstring(bw));
      #endif
   };

   if(interval->isConstant())
   {
      const auto [cst_val, cst_node] = getConstNode(interval);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing variable with " + GET_CONST_NODE(cst_node)->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      const auto useStmts = SSA->CGetUseStmts();
      for(const auto& use : useStmts)
      {
         #ifndef NDEBUG
         auto dbg_conversion = GET_CONST_NODE(use.first)->ToString() + " -> ";
         #endif
         tree_nodeRef lhs = nullptr;
         if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(use.first)))
         {
            lhs = ga->op0;
         }
         else if(const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(use.first)))
         {
            lhs = gp->res;
         }
         if(lhs != nullptr && static_cast<bool>(tree_helper::ComputeSsaUses(lhs).count(ssa_node)))
         {
            // TODO: maybe remove statement with constant variable on lhs
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, dbg_conversion + "Left hand side variable can't be replaced by a constant");
            continue;
         }
         TM->ReplaceTreeNode(use.first, ssa_node, cst_node);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, dbg_conversion + GET_CONST_NODE(use.first)->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      #ifdef BITVALUE_UPDATE
      SSA->bit_values = bitstring_to_string(create_bitstring_from_constant(cst_val, interval->getBitWidth(), isSigned));
      #endif
   }
   else if(interval->isReal())
   {
      // Nothing to set for real variables
   }
   else if(interval->isAnti() || interval->isEmpty())
   {
      THROW_ASSERT(SSA->range, "");
      if(SSA->range->isRegular())
      {
         const auto bw = getGIMPLE_BW(ssa_node);
         resetRange(SSA, bw);
      }
   }
   else if(interval->isFullSet())
   {
      const auto bw = getGIMPLE_BW(ssa_node);
      resetRange(SSA, bw);
   }
   else
   {
      const auto type_id = GET_INDEX_CONST_NODE(SSA->type);
      long long min, max;
      if(isSigned)
      {
         min = interval->getSignedMin().to<long long>();
         max = interval->getSignedMax().to<long long>();
      }
      else
      {
         min = interval->getUnsignedMin().to<long long>();
         max = interval->getUnsignedMax().to<long long>();
      }
      #ifdef BITVALUE_UPDATE
      if(SSA->bit_values.empty())
      {
         SSA->bit_values = range_to_bits(min, max, interval->getBitWidth());
      }
      #endif
      SSA->min = TM->CreateUniqueIntegerCst(min, type_id);
      SSA->max = TM->CreateUniqueIntegerCst(max, type_id);
   }

   SSA->range = interval;
   return true;
}

/// Pretty print.
void VarNode::print(std::ostream& OS) const
{
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << C->value;
   }
   else
   {
      printVarName(V, OS);
   }
   OS << " ";
   this->getRange()->print(OS);
}

std::string VarNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

void VarNode::storeAbstractState()
{
   THROW_ASSERT(!this->interval->isUnknown(), "storeAbstractState doesn't handle empty set");

   if(this->interval->getLower() == Min)
   {
      if(this->interval->getUpper() == Max)
      {
         this->abstractState = '?';
      }
      else
      {
         this->abstractState = '-';
      }
   }
   else if(this->interval->getUpper() == Max)
   {
      this->abstractState = '+';
   }
   else
   {
      this->abstractState = '0';
   }
}

std::ostream& operator<<(std::ostream& OS, const VarNode* VN)
{
   VN->print(OS);
   return OS;
}

// ========================================================================== //
// ValueRange
// ========================================================================== //
enum ValueRangeType
{
   ValueRangeId,
   SymbRangeId
};

REF_FORWARD_DECL(ValueRange);
CONSTREF_FORWARD_DECL(ValueRange);

class ValueRange
{
 private:
   RangeConstRef range;

 public:
   explicit ValueRange(const RangeConstRef& range);
   virtual ~ValueRange() = default;
   ValueRange(const ValueRange&) = delete;
   ValueRange(ValueRange&&) = delete;
   ValueRange& operator=(const ValueRange&) = delete;
   ValueRange& operator=(ValueRange&&) = delete;

   // Methods for RTTI
   virtual ValueRangeType getValueId() const
   {
      return ValueRangeId;
   }
   static bool classof(ValueRange const* /*unused*/)
   {
      return true;
   }

   /// Returns the range of this interval.
   RangeConstRef getRange() const
   {
      return this->range;
   }
   /// Sets the range of this interval to another range.
   void setRange(const RangeConstRef& newRange)
   {
      THROW_ASSERT(this->range->isReal() == newRange->isReal(), "New range must have same type of previous range");
      this->range.reset(newRange->clone());
   }

   /// Pretty print.
   virtual void print(std::ostream& OS) const;
   std::string ToString() const;
};

ValueRange::ValueRange(const RangeConstRef& _range) : range(_range->clone())
{
}

/// Pretty print.
void ValueRange::print(std::ostream& OS) const
{
   this->getRange()->print(OS);
}

std::string ValueRange::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

std::ostream& operator<<(std::ostream& OS, const ValueRange* BI)
{
   BI->print(OS);
   return OS;
}

// ========================================================================== //
// SymbRange
// ========================================================================== //

/// This is an interval that contains a symbolic limit, which is
/// given by the bounds of a program name, e.g.: [-inf, ub(b) + 1].
class SymbRange : public ValueRange
{
   private:
   /// The bound. It is a node which limits the interval of this range.
   const tree_nodeConstRef bound;
   /// The predicate of the operation in which this interval takes part.
   /// It is useful to know how we can constrain this interval
   /// after we fix the intersects.
   kind pred;

   public:
   SymbRange(const RangeConstRef& range, const tree_nodeConstRef& bound, kind pred);
   ~SymbRange() = default;
   SymbRange(const SymbRange&) = delete;
   SymbRange(SymbRange&&) = delete;
   SymbRange& operator=(const SymbRange&) = delete;
   SymbRange& operator=(SymbRange&&) = delete;

   // Methods for RTTI
   ValueRangeType getValueId() const override
   {
      return SymbRangeId;
   }
   static bool classof(SymbRange const* /*unused*/)
   {
      return true;
   }
   static bool classof(ValueRange const* BI)
   {
      return BI->getValueId() == SymbRangeId;
   }

   /// Returns the opcode of the operation that create this interval.
   kind getOperation() const
   {
      return this->pred;
   }
   /// Returns the node which is the bound of this interval.
   const tree_nodeConstRef& getBound() const
   {
      return this->bound;
   }
   /// Replace symbolic bound with hard-wired constants.
   RangeConstRef solveFuture(const VarNode* bound, const VarNode* sink) const;

   /// Prints the content of the interval.
   void print(std::ostream& OS) const override;
};

SymbRange::SymbRange(const RangeConstRef& _range, const tree_nodeConstRef& _bound, kind _pred) : ValueRange(_range), bound(_bound), pred(_pred)
{
}

RangeConstRef SymbRange::solveFuture(const VarNode* _bound, const VarNode* _sink) const
{
   // Get the lower and the upper bound of the
   // node which bounds this intersection.
   const auto boundRange = _bound->getRange();
   const auto sinkRange = _sink->getRange();
   THROW_ASSERT(!boundRange->isEmpty(), "Bound range should not be empty");
   THROW_ASSERT(!sinkRange->isEmpty(), "Sink range should not be empty");

   auto IsAnti = _bound->getRange()->isAnti() || sinkRange->isAnti();
   const auto l = IsAnti ? (boundRange->isUnknown() ? Min : boundRange->getUnsignedMin()) : boundRange->getLower();
   const auto u = IsAnti ? (boundRange->isUnknown() ? Max : boundRange->getUnsignedMax()) : boundRange->getUpper();

   // Get the lower and upper bound of the interval of this operation
   const auto lower = IsAnti ? (sinkRange->isUnknown() ? Min : sinkRange->getUnsignedMin()) : sinkRange->getLower();
   const auto upper = IsAnti ? (sinkRange->isUnknown() ? Max : sinkRange->getUnsignedMax()) : sinkRange->getUpper();

   const auto bw = getRange()->getBitWidth();
   switch(this->getOperation())
   {
      case uneq_expr_K:
      case eq_expr_K: // equal
         return RangeRef(new Range(Regular, bw, l, u));
      case le_expr_K: // signed less or equal
         if(lower > u)
         {
            return RangeRef(new Range(Empty, bw));
         }
         else
         {
            return RangeRef(new Range(Regular, bw, lower, u));
         }
      case lt_expr_K: // signed less than
         if(u != Max)
         {
            if(lower > (u - 1))
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, lower, u - 1));
         }
         else
         {
            if(lower > u)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, lower, u));
         }
      case ge_expr_K: // signed greater or equal
         if(l > upper)
         {
            return RangeRef(new Range(Empty, bw));
         }
         else
         {
            return RangeRef(new Range(Regular, bw, l, upper));
         }
      case gt_expr_K: // signed greater than
         if(l != Min)
         {
            if((l + 1) > upper)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, l + 1, upper));
         }
         else
         {
            if(l > upper)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, l, upper));
         }
      case ne_expr_K:
      {
         if(boundRange->isSingleElement())
         {
            return boundRange->getAnti();
         }
         break;
      }
      case unge_expr_K:case ungt_expr_K:case unle_expr_K:case unlt_expr_K:case assert_expr_K:case bit_and_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         break;
   }
   return getFullRangeFor(_sink->getValue());
}

/// Pretty print.
void SymbRange::print(std::ostream& OS) const
{
   const auto bnd = getBound();
   switch(this->getOperation())
   {
      case uneq_expr_K:
      case eq_expr_K: // equal
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << "), ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case unle_expr_K:
         OS << "[0, ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case le_expr_K: // sign less or equal
         OS << "[-inf, ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case unlt_expr_K:
         OS << "[0, ub(";
         printVarName(bnd, OS);
         OS << ") - 1]";
         break;
      case lt_expr_K: // sign less than
         OS << "[-inf, ub(";
         printVarName(bnd, OS);
         OS << ") - 1]";
         break;
      case unge_expr_K:
      case ge_expr_K: // sign greater or equal
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << "), +inf]";
         break;
      case ungt_expr_K:
      case gt_expr_K: // sign greater than
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << " - 1), +inf]";
         break;
      case ne_expr_K:
         OS << ")b(";
         printVarName(bnd, OS);
         OS << ")(";
         break;
      case assert_expr_K:case bit_and_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case rshift_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         OS << "Unknown Instruction.";
   }
}

// ========================================================================== //
// ConditionalValueRange
// ========================================================================== //
class ConditionalValueRange
{
 private:
   const tree_nodeConstRef V;
   std::map<unsigned int, ValueRangeRef> BBsuccs;

 public:
   ConditionalValueRange(const tree_nodeConstRef& _V, const std::map<unsigned int, ValueRangeRef>& _BBsuccs) : V(_V), BBsuccs(_BBsuccs) {}
   ConditionalValueRange(const tree_nodeConstRef& _V, unsigned int TrueBBI, unsigned int FalseBBI, const ValueRangeRef& TrueVR, const ValueRangeRef& FalseVR) : V(_V), BBsuccs({{FalseBBI, FalseVR}, {TrueBBI, TrueVR}}) {}
   ~ConditionalValueRange() = default;
   ConditionalValueRange(const ConditionalValueRange&) = default;
   ConditionalValueRange(ConditionalValueRange&&) = default;

   /// Get the interval associated to the switch case idx
   const std::map<unsigned int, ValueRangeRef>& getVR() const
   {
      return BBsuccs;
   }
   /// Get the value associated to the switch.
   const tree_nodeConstRef& getVar() const
   {
      return V;
   }
   /// Change the interval associated to the true side of the branch
   void setVR(unsigned int bbi, const ValueRangeRef& Itv)
   {
      THROW_ASSERT(static_cast<bool>(BBsuccs.count(bbi)), "Index out of bound");
      this->BBsuccs.at(bbi) = Itv;
   }
};

using ConditionalValueRanges = std::map<tree_nodeConstRef, ConditionalValueRange, tree_reindexCompare>;

// ========================================================================== //
// OpNode
// ========================================================================== //

/// This class represents a generic operation in our analysis.
class OpNode
{
 private:
   /// The range of the operation. Each operation has a range associated to it.
   /// This range is obtained by inspecting the branches in the source program
   /// and extracting its condition and intervals.
   ValueRangeRef intersect;
   // The target of the operation, that is, the node which
   // will store the result of the operation.
   VarNode* sink;
   // The instruction that originated this op node
   const tree_nodeConstRef inst;

 protected:
   /// We do not want people creating objects of this class,
   /// but we want to inherit from it.
   OpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);

 public:
   enum class OperationId
   {
      UnaryOpId,
      SigmaOpId,
      BinaryOpId,
      TernaryOpId,
      PhiOpId,
      ControlDepId,
      LoadOpId,
      StoreOpId
   };

   #ifndef NDEBUG
   static int debug_level;
   #endif

   /// The dtor. It's virtual because this is a base class.
   virtual ~OpNode() = default;
   // We do not want people creating objects of this class.
   OpNode(const OpNode&) = delete;
   OpNode(OpNode&&) = delete;
   OpNode& operator=(const OpNode&) = delete;
   OpNode& operator=(OpNode&&) = delete;

   // Methods for RTTI
   virtual OperationId getValueId() const = 0;
   static bool classof(OpNode const* /*unused*/)
   {
      return true;
   }

   /// Given the input of the operation and the operation that will be
   /// performed, evaluates the result of the operation.
   virtual RangeConstRef eval() const = 0;
   /// Return the instruction that originated this op node
   const tree_nodeConstRef& getInstruction() const
   {
      return inst;
   }
   /// Replace symbolic intervals with hard-wired constants.
   void solveFuture(VarNode* future);
   /// Returns the range of the operation.
   ValueRangeConstRef getIntersect() const
   {
      return intersect;
   }
   /// Changes the interval of the operation.
   void setIntersect(const RangeConstRef& newIntersect)
   {
      this->intersect->setRange(newIntersect);
   }
   /// Returns the target of the operation, that is,
   /// where the result will be stored.
   const VarNode* getSink() const
   {
      return sink;
   }
   /// Returns the target of the operation, that is,
   /// where the result will be stored.
   VarNode* getSink()
   {
      return sink;
   }

   virtual std::vector<tree_nodeConstRef> getSources() const = 0;

   /// Prints the content of the operation.
   virtual void print(std::ostream& OS) const = 0;
   std::string ToString() const;
};

#ifndef NDEBUG
int OpNode::debug_level = DEBUG_LEVEL_NONE;
#endif

/// We can not want people creating objects of this class,
/// but we want to inherit of it.
OpNode::OpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : intersect(_intersect), sink(_sink), inst(_inst)
{
}

void OpNode::solveFuture(VarNode* future)
{
   if(const auto SI = RefcountCast<const SymbRange>(getIntersect()))
   {
      this->setIntersect(SI->solveFuture(future, getSink()));
   }
}

std::string OpNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

// ========================================================================== //
// NodeContainer
// ========================================================================== //
// The VarNodes type.
using VarNodes = std::map<tree_nodeConstRef, VarNode*, tree_reindexCompare>;
// The Operations type.
using OpNodes = CustomSet<OpNode*>;
// A map from varnodes to the operation in which this variable is defined
using DefMap = std::map<tree_nodeConstRef, OpNode*, tree_reindexCompare>;
// A map from variables to the operations where these variables are used.
using UseMap = std::map<tree_nodeConstRef, CustomSet<OpNode*>, tree_reindexCompare>;
// Map varnodes to their view_converted float ancestor if any; pair contains original float variable and view_convert mask
using VCMap = CustomMap<VarNode*, std::pair<VarNode*, uint64_t>>;

class NodeContainer
{
 private:
   static const std::vector<std::function<
      std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&,unsigned int,const FunctionBehaviorConstRef&,const tree_managerConstRef&)>> 
   _opCtorGenerators;

   VarNodes _varNodes;

   OpNodes _opNodes;

   DefMap _defMap;

   UseMap _useMap;

   ConditionalValueRanges _cvrMap;

   VCMap _vcMap;

 protected:
   VarNodes& getVarNodes()
   {
      return _varNodes;
   }

   OpNodes& getOpNodes()
   {
      return _opNodes;
   }

   DefMap& getDefs()
   {
      return _defMap;
   }

   UseMap& getUses()
   {
      return _useMap;
   }
   
 public:
   virtual ~NodeContainer() = default;

   const VarNodes& getVarNodes() const
   {
      return _varNodes;
   }

   VarNode* addVarNode(const tree_nodeConstRef& V, unsigned int function_id)
   {
      THROW_ASSERT(V, "Can't insert nullptr as variable");
      auto vit = _varNodes.find(V);
      if(vit != _varNodes.end())
      {
         return vit->second;
      }

      auto* node = new VarNode(V, function_id);
      _varNodes.insert(std::make_pair(V, node));

      // Inserts the node in the use map list.
      CustomSet<OpNode*> useList;
      _useMap.insert(std::make_pair(V, useList));
      return node;
   }

   const ConditionalValueRanges& getCVR() const
   {
      return _cvrMap;
   }

   void addConditionalValueRange(const ConditionalValueRange&& cvr)
   {
      THROW_ASSERT(!static_cast<bool>(_cvrMap.count(cvr.getVar())), "Conditional value-ranges already present for " + GET_CONST_NODE(cvr.getVar())->ToString());
      _cvrMap.insert({cvr.getVar(), cvr});
   }

   const VCMap& getVCMap() const
   {
      return _vcMap;
   }

   void addViewConvertMask(VarNode* var, VarNode* realVar, uint64_t mask)
   {
      THROW_ASSERT(!static_cast<bool>(_vcMap.count(var)), "View-convert mask already present for " + GET_CONST_NODE(var->getValue())->ToString());
      _vcMap.insert({var, {realVar, mask}});
   }

   const OpNodes& getOpNodes() const
   {
      return _opNodes;
   }

   OpNode* pushOperation(OpNode* op)
   {
      if(op)
      {
         _opNodes.insert(op);
         _defMap.insert({op->getSink()->getValue(), op});
         for(const auto& tn : op->getSources())
         {
            _useMap[tn].insert(op);
         }
      }
      return op;
   }

   OpNode* addOperation(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM)
   {
      for(const auto& generateCtorFor : _opCtorGenerators)
      {
         if(auto generateOpFor = generateCtorFor(stmt, function_id, FB, TM))
         {
            return pushOperation(generateOpFor(this));
         }
      }
      return nullptr;
   }

   const DefMap& getDefs() const
   {
      return _defMap;
   }

   const UseMap& getUses() const
   {
      return _useMap;
   }

   #ifndef NDEBUG
   static int debug_level;
   #endif
};

#ifndef NDEBUG
int NodeContainer::debug_level = DEBUG_LEVEL_NONE;
#endif

// ========================================================================== //
// PhiOp
// ========================================================================== //

/// A constraint like sink = phi(src1, src2, ..., srcN)
class PhiOpNode : public OpNode
{
 private:
   // Vector of sources
   std::vector<const VarNode*> sources;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeConstRef eval() const override;

 public:
   PhiOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);
   ~PhiOpNode() override = default;
   PhiOpNode(const PhiOpNode&) = delete;
   PhiOpNode(PhiOpNode&&) = delete;
   PhiOpNode& operator=(const PhiOpNode&) = delete;
   PhiOpNode& operator=(PhiOpNode&&) = delete;

   /// Add source to the vector of sources
   void addSource(const VarNode* newsrc)
   {
      sources.push_back(newsrc);
   }
   /// Return source identified by index
   const VarNode* getSource(size_t index) const
   {
      return sources[index];
   }
   /// return the number of sources
   size_t getNumSources() const
   {
      return sources.size();
   }

   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> tSources;
      for(const auto& v : sources)
      {
         tSources.push_back(v->getValue());
      }
      return tSources;
   }

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::PhiOpId;
   }
   static bool classof(PhiOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::PhiOpId;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(
      const tree_nodeConstRef& stmt,unsigned int,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM);
};

// The ctor.
PhiOpNode::PhiOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_intersect, _sink, _inst)
{
}

/// Computes the interval of the sink based on the interval of the sources.
/// The result of evaluating a phi-function is the union of the ranges of
/// every variable used in the phi.
RangeConstRef PhiOpNode::eval() const
{
   THROW_ASSERT(sources.size() > 0, "Phi operation sources list empty");
   auto result = this->getSource(0)->getRange();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, getSink()->ToString() + " = PHI");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Iterate over the sources of the phiop
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "->" + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--= " + result->ToString());
   
   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "res = " + result->ToString());
   return result;
}

std::function<OpNode*(NodeContainer*)> PhiOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&)
{
   const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!phi || phi->CGetDefEdgesList().size() <= 1)
   {
      return nullptr;
   }
   return [stmt,phi,function_id](NodeContainer* NC) {
      if(phi->virtual_flag)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---This is a virtual phi, skipping...");
         return static_cast<PhiOpNode*>(nullptr);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing phi operation " + phi->ToString());

      // Create the sink.
      VarNode* sink = NC->addVarNode(phi->res, function_id);
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      PhiOpNode* phiOp = new PhiOpNode(BI, sink, stmt);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added PhiOp with range " + BI->ToString() + " and " + STR(phi->CGetDefEdgesList().size()) + " sources");

      // Create the sources.
      for(const auto& operandBBI : phi->CGetDefEdgesList())
      {
         VarNode* source = NC->addVarNode(operandBBI.first, function_id);
         phiOp->addSource(source);
      }
      return phiOp;
   };
}

/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void PhiOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";
   OS << "phi";
   OS << "\"]\n";
   for(const VarNode* varNode : sources)
   {
      const auto V = varNode->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }
   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// UnaryOp
// ========================================================================== //
/// A constraint like sink = operation(source) \intersec [l, u]
/// Examples: unary instructions such as truncation, sign extensions,
/// zero extensions.
class UnaryOpNode : public OpNode
{
 private:
   // The source node of the operation.
   VarNode* source;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeConstRef eval() const override;

 public:
   UnaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source, kind opcode);
   ~UnaryOpNode() override = default;
   UnaryOpNode(const UnaryOpNode&) = delete;
   UnaryOpNode(UnaryOpNode&&) = delete;
   UnaryOpNode& operator=(const UnaryOpNode&) = delete;
   UnaryOpNode& operator=(UnaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::UnaryOpId;
   }
   static bool classof(UnaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::UnaryOpId || BO->getValueId() == OperationId::SigmaOpId;
   }

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the source of the operation.
   VarNode* getSource() const
   {
      return source;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source->getValue()};
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(
      const tree_nodeConstRef& stmt,unsigned int,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM);
};

UnaryOpNode::UnaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source, kind _opcode) 
   : OpNode(_intersect, _sink, _inst), source(_source), opcode(_opcode)
{
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeConstRef UnaryOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         GET_CONST_NODE(getSink()->getValue())->ToString() + " = " + tree_node::GetString(this->getOpcode()) + "( " + GET_CONST_NODE(getSource()->getValue())->ToString() + " )");

   auto bw = getSink()->getBitWidth();
   RangeRef oprnd = source->getRange();
   const auto resultType = getGIMPLE_Type(getSink()->getValue());
   bool oprndSigned = isSignedType(source->getValue());
   RangeRef result = getUnknownFor(getSink()->getValue());
   if(oprnd->isRegular() || oprnd->isAnti())
   {
      switch(this->getOpcode())
      {
         case abs_expr_K:
         {
            THROW_ASSERT(oprndSigned, "Absolute value of unsigned operand should not happen");
            result = RESULT_DISABLED_OPTION(abs, getSink()->getValue(), oprnd->abs());
            break;
         }
         case bit_not_expr_K:
         {
            result = oprnd->Not();
            break;
         }
         case convert_expr_K:
         case nop_expr_K:
         {
            if(oprndSigned)
            {
               result = RESULT_DISABLED_OPTION(sext, getSink()->getValue(), oprnd->sextOrTrunc(bw));
            }
            else
            {
               result = RESULT_DISABLED_OPTION(zext, getSink()->getValue(), oprnd->zextOrTrunc(bw));
            }
            break;
         }
         case negate_expr_K:
         {
            result = RESULT_DISABLED_OPTION(negate, getSink()->getValue(), oprnd->negate());
            break;
         }
         case view_convert_expr_K:
         {
            if(resultType->get_kind() == real_type_K)
            {
               result = RangeRef(new RealRange(oprnd));
            }
            else
            {
               result = oprnd->sextOrTrunc(bw);
            }
            break;
         }
         case addr_expr_K:case paren_expr_K:case arrow_expr_K:case buffer_ref_K:case card_expr_K:case cleanup_point_expr_K:case conj_expr_K:case exit_expr_K:case fix_ceil_expr_K:case fix_floor_expr_K:case fix_round_expr_K:case fix_trunc_expr_K:case float_expr_K:case imagpart_expr_K:case indirect_ref_K:case misaligned_indirect_ref_K:case loop_expr_K:case non_lvalue_expr_K:case realpart_expr_K:case reference_expr_K:case reinterpret_cast_expr_K:case sizeof_expr_K:case static_cast_expr_K:case throw_expr_K:case truth_not_expr_K:case unsave_expr_K:case va_arg_expr_K:case reduc_max_expr_K:case reduc_min_expr_K:case reduc_plus_expr_K:case vec_unpack_hi_expr_K:case vec_unpack_lo_expr_K:case vec_unpack_float_hi_expr_K:case vec_unpack_float_lo_expr_K:
         case CASE_BINARY_EXPRESSION:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_GIMPLE_NODES:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_MISCELLANEOUS:
         default:
            THROW_UNREACHABLE("Unhandled unary operation");
            break;
      }
   }
   else if(oprnd->isEmpty())
   {
      if(resultType->get_kind() == real_type_K)
      {
         result = RangeRef(new RealRange(RangeRef(new Range(Empty, bw))));
      }
      else
      {
         result = RangeRef(new Range(Empty, bw));
      }
   }
   else if(oprnd->isReal())
   {
      auto rr = RefcountCast<RealRange>(oprnd);
      switch (this->getOpcode())
      {
         case bit_and_expr_K:
         {
            result = rr->getFractional()->zextOrTrunc(bw);
            break;
         }
         case rshift_expr_K:
         {
            result = rr->getExponent()->zextOrTrunc(bw);
            break;
         }
         case lt_expr_K:
         {
            result = rr->getSign()->zextOrTrunc(bw);
            break;
         }
         case view_convert_expr_K:
         {
            result = rr->getRange()->zextOrTrunc(bw);
            break;
         }
         case nop_expr_K:
         {
            THROW_ASSERT(bw == 32 || bw == 64, "Unhandled floating point bitwidth (" + STR(bw) + ")");
            result = bw == 32 ? rr->toFloat32() : rr->toFloat64();
            break;
         }
         case abs_expr_K:
         {
            result = RESULT_DISABLED_OPTION(abs, getSink()->getValue(), oprnd->abs());
            break;
         }
         case negate_expr_K:
         {
            result = RESULT_DISABLED_OPTION(negate, getSink()->getValue(), oprnd->negate());
            break;
         }
         case addr_expr_K:case paren_expr_K:case arrow_expr_K:case bit_not_expr_K:case buffer_ref_K:case card_expr_K:case cleanup_point_expr_K:case conj_expr_K:case convert_expr_K:case exit_expr_K:case fix_ceil_expr_K:case fix_floor_expr_K:case fix_round_expr_K:case fix_trunc_expr_K:case float_expr_K:case imagpart_expr_K:case indirect_ref_K:case misaligned_indirect_ref_K:case loop_expr_K:case non_lvalue_expr_K:case realpart_expr_K:case reference_expr_K:case reinterpret_cast_expr_K:case sizeof_expr_K:case static_cast_expr_K:case throw_expr_K:case truth_not_expr_K:case unsave_expr_K:case va_arg_expr_K:case reduc_max_expr_K:case reduc_min_expr_K:case reduc_plus_expr_K:case vec_unpack_hi_expr_K:case vec_unpack_lo_expr_K:case vec_unpack_float_hi_expr_K:case vec_unpack_float_lo_expr_K:
         case assert_expr_K:case bit_ior_expr_K:case bit_xor_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case eq_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case ge_expr_K:case gt_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case le_expr_K:case lrotate_expr_K:case lshift_expr_K:case max_expr_K:case mem_ref_K:case min_expr_K:case minus_expr_K:case modify_expr_K:case mult_expr_K:case mult_highpart_expr_K:case ne_expr_K:case ordered_expr_K:case plus_expr_K:case pointer_plus_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case set_le_expr_K:case trunc_div_expr_K:case trunc_mod_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case uneq_expr_K:case ltgt_expr_K:case unge_expr_K:case ungt_expr_K:case unle_expr_K:case unlt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
         case CASE_TERNARY_EXPRESSION:
         case CASE_QUATERNARY_EXPRESSION:
         case CASE_TYPE_NODES:
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case CASE_FAKE_NODES:
         case CASE_GIMPLE_NODES:
         case CASE_PRAGMA_NODES:
         case CASE_CPP_NODES:
         case CASE_MISCELLANEOUS:
         default:
            THROW_UNREACHABLE("Unhandled unary operation for real case");
      }
   }
   THROW_ASSERT(result, "Result should be set now");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + tree_node::GetString(this->getOpcode()) + "( " + oprnd->ToString() + " )");

   auto test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> UnaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef&,const tree_managerConstRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   if(GetPointer<const ssa_name>(GET_CONST_NODE(assign->op1)) != nullptr)
   {
      return [function_id,stmt,assign](NodeContainer* NC) {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing assign operation " + assign->ToString());

         VarNode* sink = NC->addVarNode(assign->op0, function_id);
         VarNode* _source = NC->addVarNode(assign->op1, function_id);

         auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added assign operation with range " + BI->ToString());
         return new UnaryOpNode(BI, sink, stmt, _source, nop_expr_K);
      };
   }
   const auto* un_op = GetPointer<const unary_expr>(GET_CONST_NODE(assign->op1));
   if(un_op == nullptr)
   {
      return nullptr;
   }
   return [stmt,assign,un_op,function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing unary operation " + un_op->get_kind_text() + " " + assign->ToString());

      // Create the sink.
      auto* sink = NC->addVarNode(assign->op0, function_id);
      // Create the source.
      auto* _source = NC->addVarNode(un_op->op, function_id);
      const auto sourceType = getGIMPLE_Type(_source->getValue());
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));

      const auto fromVC = NC->getVCMap().find(_source);
      if(fromVC != NC->getVCMap().end()) 
      {
         const auto& [f, mask] = fromVC->second;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source->getValue())->ToString() + " is view_convert from " + f->ToString() + "&mask(" + STR(mask) + ")");
         // Detect exponent trucantion after right shift for 32 bit floats
         if(un_op->get_kind() == nop_expr_K && f->getBitWidth() == 32 && (mask & 4286578688U) && sink->getBitWidth() == 8)
         {
            NC->addViewConvertMask(sink, f, 2139095040U);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for exponent view_convert");
            return new UnaryOpNode(BI, sink, stmt, f, rshift_expr_K);
         }

         NC->addViewConvertMask(sink, f, mask);
      }
      // Store active bitmask for variable initialized from float view_convert operation
      if(un_op->get_kind() == view_convert_expr_K && sourceType->get_kind() == real_type_K)
      {
         NC->addViewConvertMask(sink, _source, static_cast<uint64_t>(-1));
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for " + un_op->get_kind_text() + " with range " + BI->ToString());
      return new UnaryOpNode(BI, sink, stmt, _source, un_op->get_kind());
   };
}

/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void UnaryOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";

   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   const bool oprndSigned = isSignedType(source->getValue());

   if(opcode == nop_expr_K || opcode == convert_expr_K)
   {
      if(bw < getSource()->getBitWidth())
      {
         OS << "trunc i" << bw;
      }
      else
      {
         if(getGIMPLE_Type(getSource()->getValue())->get_kind() == pointer_type_K)
         {
            OS << "ptr_cast i" << bw;
         }
         else
         {
            if(oprndSigned)
            {
               OS << "sext i" << bw;
            }
            else
            {
               OS << "zext i" << bw;
            }
         }
      }
   }
   else if(opcode == fix_trunc_expr_K)
   {
      const auto type = getGIMPLE_Type(getSink()->getValue());
      if(const auto* int_type = GetPointer<const integer_type>(type))
      {
         if(int_type->unsigned_flag)
         {
            OS << "fptoui i" << bw;
         }
         else
         {
            OS << "fptosi i" << bw;
         }
      }
      else
      {
         THROW_UNREACHABLE("Sink should be of type integer");
      }
   }
   else
   {
      // Phi functions, Loads and Stores are handled here.
      this->getIntersect()->print(OS);
   }

   OS << "\"]\n";

   const auto V = this->getSource()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }

   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// SigmaOp
// ========================================================================== //
/// Specific type of UnaryOp used to represent sigma functions
class SigmaOpNode : public UnaryOpNode
{
 private:
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeConstRef eval() const override;

   // The symbolic source node of the operation.
   VarNode* SymbolicSource;

   bool unresolved;

 public:
   SigmaOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source, VarNode* SymbolicSource, kind opcode);
   ~SigmaOpNode() override = default;
   SigmaOpNode(const SigmaOpNode&) = delete;
   SigmaOpNode(SigmaOpNode&&) = delete;
   SigmaOpNode& operator=(const SigmaOpNode&) = delete;
   SigmaOpNode& operator=(SigmaOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::SigmaOpId;
   }
   static bool classof(SigmaOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(UnaryOpNode const* UO)
   {
      return UO->getValueId() == OperationId::SigmaOpId;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::SigmaOpId;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> s;
      if(SymbolicSource != nullptr)
      {
         s.push_back(SymbolicSource->getValue());
      }
      return s;
   }

   bool isUnresolved() const
   {
      return unresolved;
   }
   void markResolved()
   {
      unresolved = false;
   }
   void markUnresolved()
   {
      unresolved = true;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM);
};

SigmaOpNode::SigmaOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source, VarNode* _SymbolicSource, kind _opcode)
      : UnaryOpNode(_intersect, _sink, _inst, _source, _opcode), SymbolicSource(_SymbolicSource), unresolved(false)
{
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeConstRef SigmaOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_CONST_NODE(getSink()->getValue())->ToString() + " = SIGMA< " + GET_CONST_NODE(getSource()->getValue())->ToString() + " >");

   auto result = this->getSource()->getRange();
   auto aux = this->getIntersect()->getRange();
   if(!aux->isUnknown())
   {
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = SIGMA< " + getSource()->getRange()->ToString() + " >");
   return result;
}

std::function<OpNode*(NodeContainer*)> SigmaOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef&,const tree_managerConstRef&)
{
   const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!phi || phi->CGetDefEdgesList().size() != 1)
   {
      return nullptr;
   }
   return [stmt,phi,function_id](NodeContainer* NC) {
      const auto BBI = phi->bb_index;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing sigma operation " + phi->ToString());
      const auto& sourceTN = phi->CGetDefEdgesList().front().first;

      // Create the sink.
      VarNode* sink = NC->addVarNode(phi->res, function_id);
      VarNode* source = NC->addVarNode(sourceTN, function_id);

      auto vsmit = NC->getCVR().find(sourceTN);
      if(vsmit == NC->getCVR().end())
      {
         return static_cast<SigmaOpNode*>(nullptr);
      }

      auto condRangeIt = vsmit->second.getVR().find(BBI);
      if(condRangeIt != vsmit->second.getVR().end())
      {
         const auto& CondRange = condRangeIt->second;
         VarNode* SymbSrc = nullptr;
         if(auto symb = RefcountCast<SymbRange>(CondRange))
         {
            const auto& bound = symb->getBound();
            SymbSrc = NC->addVarNode(bound, function_id);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added SigmaOp with " + std::string(SymbSrc ? "symbolic " : "") + "range " + CondRange->ToString());
         return new SigmaOpNode(CondRange, sink, stmt, source, SymbSrc, phi->get_kind());
      }
      else
      {
         auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added SigmaOp with range " + BI->ToString());
         return new SigmaOpNode(BI, sink, stmt, source, nullptr, phi->get_kind());
      }
   };
}

/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void SigmaOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")"
      << "SigmaOp:";
   this->getIntersect()->print(OS);
   OS << "\"]\n";
   const auto V = this->getSource()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   if(SymbolicSource)
   {
      const auto _V = SymbolicSource->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(_V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(_V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }

   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// BinaryOp
// ========================================================================== //
/// A constraint like sink = source1 operation source2 intersect [l, u].
class BinaryOpNode : public OpNode
{
 private:
   // The first operand.
   VarNode* source1;
   // The second operand.
   VarNode* source2;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeConstRef eval() const override;

 public:
   BinaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source1, VarNode* source2, kind opcode);
   ~BinaryOpNode() override = default;
   BinaryOpNode(const BinaryOpNode&) = delete;
   BinaryOpNode(BinaryOpNode&&) = delete;
   BinaryOpNode& operator=(const BinaryOpNode&) = delete;
   BinaryOpNode& operator=(BinaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::BinaryOpId;
   }
   static bool classof(BinaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::BinaryOpId;
   }

   static RangeConstRef evaluate(kind opcode, bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2, bool isSigned);

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the first operand of this operation.
   VarNode* getSource1() const
   {
      return source1;
   }
   /// Returns the second operand of this operation.
   VarNode* getSource2() const
   {
      return source2;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source1->getValue(), source2->getValue()};
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(
      const tree_nodeConstRef& stmt,unsigned int,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM);
};

// The ctor.
BinaryOpNode::BinaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source1, VarNode* _source2, kind _opcode) 
   : OpNode(_intersect, _sink, _inst), source1(_source1), source2(_source2), opcode(_opcode)
{
   THROW_ASSERT(isValidType(_sink->getValue()), "Binary operation sink should be of valid type (" + GET_CONST_NODE(_sink->getValue())->ToString() + ")");
}

RangeConstRef BinaryOpNode::evaluate(kind opcode, bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2, bool isSigned)
{
   switch(opcode)
   {
      #ifdef INTEGER_PTR
      case pointer_plus_expr_K:
      #endif
      case plus_expr_K:
         RETURN_DISABLED_OPTION(add, bw);
         return op1->add(op2);
      case minus_expr_K:
         RETURN_DISABLED_OPTION(sub, bw);
         return op1->sub(op2);
      case mult_expr_K:
         RETURN_DISABLED_OPTION(mul, bw);
         return op1->mul(op2);
      case trunc_div_expr_K:
         if(isSigned)
         {
            RETURN_DISABLED_OPTION(sdiv, bw);
            return op1->sdiv(op2);
         }
         else
         {
            RETURN_DISABLED_OPTION(udiv, bw);
            return op1->udiv(op2);
         }
      case trunc_mod_expr_K:
         if(isSigned)
         {
            RETURN_DISABLED_OPTION(srem, bw);
            return op1->srem(op2);
         }
         else
         {
            RETURN_DISABLED_OPTION(urem, bw);
            return op1->urem(op2);
         }
      case lshift_expr_K:
         RETURN_DISABLED_OPTION(shl, bw);
         return isSigned ? op1->sextOrTrunc(bw)->shl(op2) : op1->zextOrTrunc(bw)->shl(op2);
      case rshift_expr_K:
         RETURN_DISABLED_OPTION(shr, bw);
         return isSigned ? op1->shr(op2, true)->sextOrTrunc(bw) : op1->shr(op2, false)->zextOrTrunc(bw);
      case bit_and_expr_K:
         RETURN_DISABLED_OPTION(and, bw);
         return op1->And(op2);
      case bit_ior_expr_K:
         RETURN_DISABLED_OPTION(or, bw);
         return op1->Or(op2);
      case bit_xor_expr_K:
         RETURN_DISABLED_OPTION(xor, bw);
         return op1->Xor(op2);
      case uneq_expr_K:
      case eq_expr_K:
         return op1->Eq(op2, bw);
      case ne_expr_K:
         return op1->Ne(op2, bw);
      case unge_expr_K:
         return op1->Uge(op2, bw);
      case ungt_expr_K:
         return op1->Ugt(op2, bw);
      case unlt_expr_K:
         return op1->Ult(op2, bw);
      case unle_expr_K:
         return op1->Ule(op2, bw);
      case gt_expr_K:
         return op1->Sgt(op2, bw);
      case ge_expr_K:
         return op1->Sge(op2, bw);
      case lt_expr_K:
         return op1->Slt(op2, bw);
      case le_expr_K:
         return op1->Sle(op2, bw);
      case min_expr_K:
         RETURN_DISABLED_OPTION(min, bw);
         return isSigned ? op1->SMin(op2) : op1->UMin(op2);
      case max_expr_K:
         RETURN_DISABLED_OPTION(max, bw);
         return isSigned ? op1->SMax(op2) : op1->UMax(op2);

      #ifndef INTEGER_PTR
      case pointer_plus_expr_K:
      #endif
      case assert_expr_K:case catch_expr_K:case ceil_div_expr_K:case ceil_mod_expr_K:case complex_expr_K:case compound_expr_K:case eh_filter_expr_K:case exact_div_expr_K:case fdesc_expr_K:case floor_div_expr_K:case floor_mod_expr_K:case goto_subroutine_K:case in_expr_K:case init_expr_K:case lrotate_expr_K:case mem_ref_K:case modify_expr_K:case mult_highpart_expr_K:case ordered_expr_K:case postdecrement_expr_K:case postincrement_expr_K:case predecrement_expr_K:case preincrement_expr_K:case range_expr_K:case rdiv_expr_K:case round_div_expr_K:case round_mod_expr_K:case rrotate_expr_K:case set_le_expr_K:case truth_and_expr_K:case truth_andif_expr_K:case truth_or_expr_K:case truth_orif_expr_K:case truth_xor_expr_K:case try_catch_expr_K:case try_finally_K:case ltgt_expr_K:case unordered_expr_K:case widen_sum_expr_K:case widen_mult_expr_K:case with_size_expr_K:case vec_lshift_expr_K:case vec_rshift_expr_K:case widen_mult_hi_expr_K:case widen_mult_lo_expr_K:case vec_pack_trunc_expr_K:case vec_pack_sat_expr_K:case vec_pack_fix_trunc_expr_K:case vec_extracteven_expr_K:case vec_extractodd_expr_K:case vec_interleavehigh_expr_K:case vec_interleavelow_expr_K:case extract_bit_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         THROW_UNREACHABLE("Unhandled binary operation (" + tree_node::GetString(opcode) + ")");
         break;
   }
   return nullptr;
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
/// Basically, this function performs the operation indicated in its opcode
/// taking as its operands the source1 and the source2.
RangeConstRef BinaryOpNode::eval() const
{
   const auto op1 = this->getSource1()->getRange();
   const auto op2 = this->getSource2()->getRange();
   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   RangeConstRef result = getUnknownFor(getSink()->getValue());

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         GET_CONST_NODE(getSink()->getValue())->ToString() + " = (" + GET_CONST_NODE(getSource1()->getValue())->ToString() + ")" + tree_node::GetString(this->getOpcode()) + "(" + GET_CONST_NODE(getSource2()->getValue())->ToString() + ")");

   // only evaluate if all operands are Regular
   if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()))
   {
      bool isSigned = isSignedType(getSource1()->getValue());

      result = evaluate(this->getOpcode(), bw, op1, op2, isSigned);

      if(result->getBitWidth() != bw)
      {
         result = result->zextOrTrunc(bw);
      }
   }
   else
   {
      if(op1->isEmpty() || op2->isEmpty())
      {
         result = getEmptyFor(getSink()->getValue());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + op1->ToString() + " " + tree_node::GetString(this->getOpcode()) + " " + op2->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> BinaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef&,const tree_managerConstRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   const auto* bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(assign->op1));
   if(bin_op == nullptr)
   {
      return nullptr;
   }
   return [stmt,assign,bin_op,function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing binary operation " + bin_op->get_kind_text() + " " + assign->ToString());
      
      // Create the sink.
      auto* sink = NC->addVarNode(assign->op0, function_id);
      auto op_kind = bin_op->get_kind();

      // Create the sources.
      auto* _source1 = NC->addVarNode(bin_op->op0, function_id);
      auto* _source2 = NC->addVarNode(bin_op->op1, function_id);
      
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));

      if((op_kind == rshift_expr_K || op_kind == lshift_expr_K || op_kind == bit_and_expr_K) && GET_CONST_NODE(_source2->getValue())->get_kind() == integer_cst_K)
      {
         const auto fromVC = NC->getVCMap().find(_source1);
         if(fromVC != NC->getVCMap().end())
         {
            const auto& [f, mask] = fromVC->second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source1->getValue())->ToString() + " is view_convert from " + f->ToString() + "&mask(" + STR(mask) + ")");
            const auto cst_int = tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(_source2->getValue())));
            auto new_mask = mask;

            if(op_kind == rshift_expr_K)
            {
               new_mask = (mask >> cst_int) << cst_int;
            }
            else if(op_kind == lshift_expr_K)
            {
               new_mask = (mask << cst_int) >> cst_int;
            }
            else if(op_kind == bit_and_expr_K)
            {
               // Trailing zeros indicates a right shift has already been applied, so they are not relevant
               const auto shift = APInt(mask).trailingZeros(std::numeric_limits<decltype(new_mask)>::digits);
               new_mask = mask & static_cast<decltype(new_mask)>(cst_int << shift);
            }
            NC->addViewConvertMask(sink, f, new_mask);
            
            const auto addSignViewConvert = [&](VarNode* fpVar)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for sign view_convert");
               return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, lt_expr_K));
            };
            const auto addExponentViewConvert = [&](VarNode* fpVar)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for exponent view_convert");
               return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, rshift_expr_K));
            };
            const auto addFractionalViewConvert = [&](VarNode* fpVar)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for significand view_convert");
               return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, bit_and_expr_K));
            };

            if(f->getBitWidth() == 32)
            {
               switch(new_mask)
               {
                  case 4294967296U:
                     return addSignViewConvert(f);
                  case 2139095040U:
                     return addExponentViewConvert(f);
                  case 8388607U:
                     return addFractionalViewConvert(f);
                  default:
                     break;
               }
            }
            else
            {
               switch(new_mask)
               {
                  case 9223372036854775808ULL:
                     return addSignViewConvert(f);
                  case 9218868437227405312ULL:
                     return addExponentViewConvert(f);
                  case 4503599627370495ULL:
                     return addFractionalViewConvert(f);
                  default:
                     break;
               }
            }
         }
      }
      else if(op_kind == lt_expr_K && GET_CONST_NODE(_source2->getValue())->get_kind() == integer_cst_K && tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(_source2->getValue()))) == 0)
      {
         const auto fromVC = NC->getVCMap().find(_source1);
         if(fromVC != NC->getVCMap().end())
         {
            auto& [f, mask] = fromVC->second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source1->getValue())->ToString() + " is view_convert from " + f->ToString() + "&mask(" + STR(mask) + ")");
            NC->addViewConvertMask(sink, f, f->getBitWidth() == 32 ? 4294967296U : 9223372036854775808ULL);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for sign view_convert");
            return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, f, lt_expr_K));
         }
      }

      if(isCompare(op_kind))
      {
         const auto opSigned = isSignedType(bin_op->op0);
         if(!opSigned)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Compare operation flagged as unsigned");
            op_kind = op_unsigned(op_kind);
         }
      }

      // Create the operation using the intersect to constrain sink's interval.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added BinaryOp for " + tree_node::GetString(op_kind) + " with range " + BI->ToString());
      return static_cast<OpNode*>(new BinaryOpNode(BI, sink, stmt, _source1, _source2, op_kind));
   };
}

/// Pretty print.
void BinaryOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   std::string opcodeName = tree_node::GetString(opcode);
   OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";
   const auto V1 = this->getSource1()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V1)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V1, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto V2 = this->getSource2()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V2)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V2, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

unsigned int evaluateBranch(const tree_nodeRef br_op, const blocRef branchBB
#ifndef NDEBUG
   , int debug_level
#endif
   )
{
   // Evaluate condition variable if possible
   if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(br_op)))
   {
      const auto branchValue = tree_helper::get_integer_cst_value(ic);
      if(branchValue)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable value is " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
         return branchBB->false_edge;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable value is " + STR(branchValue) + ", true edge BB" + STR(branchBB->true_edge) + " to be removed");
         return branchBB->true_edge;
      }
   }
   else if(const auto* bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(br_op)))
   {
      const auto* l = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op0));
      const auto* r = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op1));
      if(l != nullptr && r != nullptr)
      {
         const auto lc = tree_helper::get_integer_cst_value(l);
         const auto rc = tree_helper::get_integer_cst_value(r);
         RangeRef lhs(new Range(Regular, MAX_BIT_INT, lc, lc));
         RangeRef rhs(new Range(Regular, MAX_BIT_INT, rc, rc));
         const auto branchValue = BinaryOpNode::evaluate(bin_op->get_kind(), 1, lhs, rhs, isSignedType(bin_op->op0));
         THROW_ASSERT(branchValue->isConstant(), "Constant binary operation should resolve to either true or false");
         if(branchValue->getUnsignedMax())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->false_edge;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->true_edge;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition has non-integer cst_node operands, skipping...");
      return bloc::EXIT_BLOCK_ID;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a non-integer cst_node, skipping...");
   return bloc::EXIT_BLOCK_ID;
}

// ========================================================================== //
// TernaryOp
// ========================================================================== //
class TernaryOpNode : public OpNode
{
 private:
   // The first operand.
   VarNode* source1;
   // The second operand.
   VarNode* source2;
   // The third operand.
   VarNode* source3;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeConstRef eval() const override;

 public:
   TernaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source1, VarNode* source2, VarNode* source3, kind opcode);
   ~TernaryOpNode() override = default;
   TernaryOpNode(const TernaryOpNode&) = delete;
   TernaryOpNode(TernaryOpNode&&) = delete;
   TernaryOpNode& operator=(const TernaryOpNode&) = delete;
   TernaryOpNode& operator=(TernaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::TernaryOpId;
   }
   static bool classof(TernaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::TernaryOpId;
   }

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the first operand of this operation.
   VarNode* getSource1() const
   {
      return source1;
   }
   /// Returns the second operand of this operation.
   VarNode* getSource2() const
   {
      return source2;
   }
   /// Returns the third operand of this operation.
   VarNode* getSource3() const
   {
      return source3;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source1->getValue(), source2->getValue(), source3->getValue()};
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef&,unsigned int,const FunctionBehaviorConstRef&,const tree_managerConstRef&);
};

// The ctor.
TernaryOpNode::TernaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source1, VarNode* _source2, VarNode* _source3, kind _opcode)
      : OpNode(_intersect, _sink, _inst), source1(_source1), source2(_source2), source3(_source3), opcode(_opcode)
{
   #if HAVE_ASSERTS
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(_inst));
   THROW_ASSERT(ga, "TernaryOp associated statement should be a gimple_assign " + GET_CONST_NODE(_inst)->ToString());
   const auto* I = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
   THROW_ASSERT(I, "TernaryOp operator should be a ternary_expr");
   THROW_ASSERT(_sink->getBitWidth() >= _source2->getBitWidth(), "Operator bitwidth overflow (sink= " + STR(_sink->getBitWidth()) + ", op2= " + STR(_source2->getBitWidth()) + ")");
   THROW_ASSERT(_sink->getBitWidth() >= _source3->getBitWidth(), "Operator bitwidth overflow (sink= " + STR(_sink->getBitWidth()) + ", op3= " + STR(_source3->getBitWidth()) + ")");
   #endif
}

RangeConstRef TernaryOpNode::eval() const
{
   auto op1 = this->getSource1()->getRange();
   auto op2 = this->getSource2()->getRange();
   auto op3 = this->getSource3()->getRange();
   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   RangeConstRef result = getUnknownFor(getSink()->getValue());

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         GET_CONST_NODE(getSink()->getValue())->ToString() + " = " + GET_CONST_NODE(getSource1()->getValue())->ToString() + " ? " + GET_CONST_NODE(getSource2()->getValue())->ToString() + " : " + GET_CONST_NODE(getSource3()->getValue())->ToString());

   // only evaluate if all operands are Regular
   if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()) && (op3->isRegular() || op3->isAnti()))
   {
      if(this->getOpcode() == cond_expr_K)
      {
         // Source1 is the selector
         if(op1->isSameRange(RangeRef(new Range(Regular, op1->getBitWidth(), 1, 1))))
         {
            result = op2;
         }
         else if(op1->isSameRange(RangeRef(new Range(Regular, op1->getBitWidth(), 0, 0))))
         {
            result = op3;
         }
         else
         {
            const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(getInstruction()));
            const auto* I = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
            const auto BranchVar = branchOpRecurse(I->op0);
            std::vector<const struct binary_expr*> BranchConds;
            // Check if branch variable is correlated with op1 or op2
            if(GetPointer<const gimple_phi>(BranchVar) != nullptr)
            {
               // TODO: find a way to propagate range from all phi edges when phi->res is one of the two result of the cond_expr 
            }
            else if(const auto* BranchExpr = GetPointer<const binary_expr>(BranchVar))
            {
               BranchConds.push_back(BranchExpr);
            }

            for(const auto* be : BranchConds)
            {
               if(isCompare(be))
               {
                  const tree_nodeConstRef CondOp0 = be->op0;
                  const tree_nodeConstRef CondOp1 = be->op1;
                  if(GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K || GET_CONST_NODE(CondOp1)->get_kind() == integer_cst_K)
                  {
                     const auto variable = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp1 : CondOp0;
                     const auto* constant = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? GetPointer<const integer_cst>(GET_CONST_NODE(CondOp0)) : GetPointer<const integer_cst>(GET_CONST_NODE(CondOp1));
                     auto opV1 = I->op1;
                     auto opV2 = I->op2;
                     if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV1) || GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV2))
                     {
                        RangeRef CR(new Range(Regular, bw, constant->value, constant->value));
                        kind pred = isSignedType(CondOp0) ? be->get_kind() : op_unsigned(be->get_kind());
                        kind swappred = op_swap(pred);

                        auto tmpT = (variable == CondOp0) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
                        THROW_ASSERT(!tmpT->isFullSet(), "");

                        if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV2))
                        {
                           RangeRef FValues(new Range(*tmpT->getAnti()));
                           op3 = op3->intersectWith(FValues);
                        }
                        else
                        {
                           op2 = op2->intersectWith(tmpT);
                        }
                     }
                  }
               }
            }
            result = op2->unionWith(op3);
         }
      }
   }
   else
   {
      if(op1->isEmpty() || op2->isEmpty() || op3->isEmpty())
      {
         result = getEmptyFor(getSink()->getValue());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + op1->ToString() + " ? " + op2->ToString() + " : " + op3->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> TernaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef&,const tree_managerConstRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   const auto* ter_op = GetPointer<const ternary_expr>(GET_CONST_NODE(assign->op1));
   if(ter_op == nullptr)
   {
      return nullptr;
   }
   return [stmt,assign,ter_op,function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing ternary operation " + ter_op->get_kind_text() + " " + assign->ToString());
      // Create the sink.
      VarNode* sink = NC->addVarNode(assign->op0, function_id);

      // Create the sources.
      VarNode* _source1 = NC->addVarNode(ter_op->op0, function_id);
      VarNode* _source2 = NC->addVarNode(ter_op->op1, function_id);
      VarNode* _source3 = NC->addVarNode(ter_op->op2, function_id);

      // Create the operation using the intersect to constrain sink's interval.
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added TernaryOp for " + ter_op->get_kind_text() + " with range " + BI->ToString());
      return new TernaryOpNode(BI, sink, stmt, _source1, _source2, _source3, ter_op->get_kind());
   };
}

/// Pretty print.
void TernaryOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   std::string opcodeName = tree_node::GetString(this->getOpcode());
   OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";

   const auto V1 = this->getSource1()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V1)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V1, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto V2 = this->getSource2()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V2)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V2, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }

   const auto V3 = this->getSource3()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V3)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V3, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// LoadOp
// ========================================================================== //
class LoadOpNode : public OpNode
{
 private:
   /// reference to the memory access operand
   std::vector<const VarNode*> sources;
   RangeConstRef eval() const override;

 public:
   LoadOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);
   ~LoadOpNode() override = default;
   LoadOpNode(const LoadOpNode&) = delete;
   LoadOpNode(LoadOpNode&&) = delete;
   LoadOpNode& operator=(const LoadOpNode&) = delete;
   LoadOpNode& operator=(LoadOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::LoadOpId;
   }
   static bool classof(LoadOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::LoadOpId;
   }

   /// Add source to the vector of sources
   void addSource(const VarNode* newsrc)
   {
      sources.push_back(newsrc);
   }
   /// Return source identified by index
   const VarNode* getSource(size_t index) const
   {
      return sources[index];
   }
   /// return the number of sources
   size_t getNumSources() const
   {
      return sources.size();
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> sourceTNs;
      for(const auto& s : sources)
      {
         sourceTNs.push_back(s->getValue());
      }
      return sourceTNs;
   }

   void print(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM);
};

LoadOpNode::LoadOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_intersect, _sink, _inst)
{
}

RangeConstRef LoadOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, getSink()->ToString() + " = LOAD");

   if(getNumSources() == 0)
   {
      THROW_ASSERT(getSink()->getBitWidth() == getIntersect()->getRange()->getBitWidth(), "Sink (" + GET_CONST_NODE(getSink()->getValue())->ToString() + ") has bitwidth " + STR(getSink()->getBitWidth()) + " while intersect has bitwidth " + STR(getIntersect()->getRange()->getBitWidth()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "= " + getIntersect()->getRange()->ToString());
      return getIntersect()->getRange();
   }

   // Iterate over the sources of the load
   RangeConstRef result = this->getSource(0)->getRange();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "->" + varNode->getRange()->ToString() + " " + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--= " + result->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> LoadOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,unsigned int function_id,const FunctionBehaviorConstRef& FB,const tree_managerConstRef& TM)
{
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   if(!tree_helper::IsLoad(TM, stmt, FB->get_function_mem()))
   {
      return nullptr;
   }
   return [stmt,ga,function_id,TM](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing load operation " + ga->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
      auto bw = getGIMPLE_BW(ga->op0);
      VarNode* sink = NC->addVarNode(ga->op0, function_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Sink variable is " + GET_CONST_NODE(ga->op0)->get_kind_text() + " (size = " + STR(bw) + ")");

      RangeRef intersection = getEmptyFor(sink->getValue());
      CustomOrderedSet<unsigned int> res_set;
      bool pointToConstants = tree_helper::is_fully_resolved(TM, GET_INDEX_CONST_NODE(ga->op1), res_set);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Pointer is fully resolved");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
      for(auto indexIt = res_set.begin(); indexIt != res_set.end() && pointToConstants; ++indexIt)
      {
         const auto TN = TM->CGetTreeNode(*indexIt);
         if(const auto* vd = GetPointer<const var_decl>(TN))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Points to " + TN->ToString() + 
               " (readonly = " + STR(vd->readonly_flag) + 
               ", defs = " + STR(vd->defs.size()) + 
               ", full-size = " + STR(GetPointer<const integer_cst>(GET_CONST_NODE(vd->size))->value) + ")");
            if(!vd->readonly_flag || vd->init == nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Pointed variable is not constant " + TN->ToString());
               pointToConstants = false;
               break;
            }
            if(const auto* constr = GetPointer<const constructor>(GET_CONST_NODE(vd->init)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->Initializer has " + STR(constr->list_of_idx_valu.size()) + " values:");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
               THROW_ASSERT(static_cast<bool>(constr->list_of_idx_valu.size()), "At least one initializer should be there");
               for(const auto& idxValue : constr->list_of_idx_valu)
               {
                  const auto& value = idxValue.second;
                  if(tree_helper::is_constant(TM, GET_INDEX_CONST_NODE(value)) && isValidType(value))
                  {
                     #ifndef NDEBUG
                     const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(value));
                     if(ic && bw == 8)
                     {
                        auto asciiChar = tree_helper::get_integer_cst_value(ic);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, GET_CONST_NODE(value)->ToString() + " '" + static_cast<char>(asciiChar) + "'");
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, GET_CONST_NODE(value)->ToString());
                     }
                     #endif
                     intersection = intersection->unionWith(getGIMPLE_range(value));
                  }
                  else
                  {
                     pointToConstants = false;
                     break;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--");
            }
            #ifndef NDEBUG
            else if(const auto* cst_val = GetPointer<const cst_node>(GET_CONST_NODE(vd->init)))
            #else
            else if(GetPointer<const cst_node>(GET_CONST_NODE(vd->init)) != nullptr)
            #endif
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value is " + cst_val->ToString());
               intersection = getGIMPLE_range(vd->init);
               THROW_ASSERT(intersection->getBitWidth() == bw, "Initializer bitwidth should be the same as the initialized variable's one (" + intersection->ToString() + ")");
            }
            else if(GetPointer<const addr_expr>(GET_CONST_NODE(vd->init)) != nullptr)
            {
               pointToConstants = false;  // TODO: put all in the else branch and remove throw_unreachable
            }
            else
            {
               THROW_UNREACHABLE("Unhandled initializer " + GET_CONST_NODE(vd->init)->get_kind_text() + " " + GET_CONST_NODE(vd->init)->ToString());
               pointToConstants = false;
            }
         }
         else
         {
            THROW_UNREACHABLE("Unknown tree node " + TN->get_kind_text() + " " + TN->ToString());
            pointToConstants = false;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--");

      if(!pointToConstants)
      {
         intersection = getGIMPLE_range(stmt);
      }
      THROW_ASSERT(intersection->getBitWidth() <= bw, "Pointed variables range should have bitwidth contained in sink bitwidth");
      if(intersection->getBitWidth() < bw)
      {
         intersection = intersection->zextOrTrunc(bw);
      }
      auto BI = ValueRangeRef(new ValueRange(intersection));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--Added LoadOp with range " + BI->ToString());
      return new LoadOpNode(BI, sink, stmt);
   };
}

void LoadOpNode::print(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";
   OS << "LoadOp\"]\n";

   for(auto src : sources)
   {
      const auto V = src->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }

   const auto VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

const std::vector<std::function<
   std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&,unsigned int,const FunctionBehaviorConstRef&,const tree_managerConstRef&)>> 
NodeContainer::_opCtorGenerators = {
   LoadOpNode::opCtorGenerator,
   UnaryOpNode::opCtorGenerator,
   BinaryOpNode::opCtorGenerator,
   PhiOpNode::opCtorGenerator,
   SigmaOpNode::opCtorGenerator,
   TernaryOpNode::opCtorGenerator
};

// ========================================================================== //
// StoreOp
// ========================================================================== //
//    class StoreOp : public BasicOp
//    {
//     private:
//       /// reference to the memory access operand
//       std::vector<const VarNode*> sources;
//       /// union of the values at which the variable is initialized
//       RangeRef init;
//       RangeRef eval() const override;
//    
//     public:
//       StoreOp(VarNode* sink, const tree_nodeConstRef inst, RangeConstRef _init);
//       ~StoreOp() override;
//       StoreOp(const StoreOp&) = delete;
//       StoreOp(StoreOp&&) = delete;
//       StoreOp& operator=(const StoreOp&) = delete;
//       StoreOp& operator=(StoreOp&&) = delete;
//    
//       // Methods for RTTI
//       OperationId getValueId() const override
//       {
//          return OperationId::StoreOpId;
//       }
//       static bool classof(StoreOp const* /*unused*/)
//       {
//          return true;
//       }
//       static bool classof(BasicOp const* BO)
//       {
//          return BO->getValueId() == OperationId::StoreOpId;
//       }
//    
//       /// Add source to the vector of sources
//       void addSource(const VarNode* newsrc)
//       {
//          sources.push_back(newsrc);
//       }
//       /// Return source identified by index
//       const VarNode* getSource(size_t index) const
//       {
//          return sources[index];
//       }
//       /// return the number of sources
//       size_t getNumSources() const
//       {
//          return sources.size();
//       }
//    
//       void print(std::ostream& OS) const override;
//    };
//    
//    StoreOp::StoreOp(VarNode* _sink, const tree_nodeConstRef _inst, RangeConstRef _init) : BasicOp(refcount<BasicInterval>(new BasicInterval(_sink->getMaxRange())), _sink, _inst), init(_init->clone())
//    {
//    }
//    
//    StoreOp::~StoreOp() = default;
//    
//    RangeRef StoreOp::eval() const
//    {
//       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, getSink()->ToString() + " = LOAD");
//       RangeRef result = init;
//       // Iterate over the sources of the Store
//       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
//       for(const VarNode* varNode : sources)
//       {
//          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "->" + varNode->getRange()->ToString() + " " + varNode->ToString());
//          result = result->unionWith(varNode->getRange());
//       }
//       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--= " + result->ToString());
//       return result;
//    }
//    
//    void StoreOp::print(std::ostream& OS) const
//    {
//       const char* quot = R"(")";
//       OS << " " << quot << this << quot << R"( [label=")";
//       OS << "StoreOp\"]\n";
//    
//       for(auto src : sources)
//       {
//          const auto V = src->getValue();
//          if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
//          {
//             OS << " " << C->value << " -> " << quot << this << quot << "\n";
//          }
//          else
//          {
//             OS << " " << quot;
//             printVarName(V, OS);
//             OS << quot << " -> " << quot << this << quot << "\n";
//          }
//       }
//    
//       const auto VS = this->getSink()->getValue();
//       OS << " " << quot << this << quot << " -> " << quot;
//       printVarName(VS, OS);
//       OS << quot << "\n";
//    }

// ========================================================================== //
// ControlDep
// ========================================================================== //
/// Specific type of OpNode used in Nuutila's strongly connected
/// components algorithm.
class ControlDepNode : public OpNode
{
 private:
   VarNode* source;
   RangeConstRef eval() const override;

 public:
   ControlDepNode(VarNode* sink, VarNode* source);
   ~ControlDepNode() override = default;
   ControlDepNode(const ControlDepNode&) = delete;
   ControlDepNode(ControlDepNode&&) = delete;
   ControlDepNode& operator=(const ControlDepNode&) = delete;
   ControlDepNode& operator=(ControlDepNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::ControlDepId;
   }
   static bool classof(ControlDepNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::ControlDepId;
   }

   /// Returns the source of the operation.
   VarNode* getSource() const
   {
      return source;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source->getValue()};
   }

   void print(std::ostream& OS) const override;


};

ControlDepNode::ControlDepNode(VarNode* _sink, VarNode* _source) : OpNode(ValueRangeRef(new ValueRange(_sink->getMaxRange())), _sink, nullptr), source(_source)
{
}

RangeConstRef ControlDepNode::eval() const
{
   return RangeRef(new Range(Regular, MAX_BIT_INT));
}

void ControlDepNode::print(std::ostream& /*OS*/) const
{
}

// ========================================================================== //
// Nuutila
// ========================================================================== //

// A map from variables to the operations where these
// variables are present as bounds
using SymbMap = std::map<tree_nodeConstRef, CustomSet<OpNode*>, tree_reindexCompare>;

class Nuutila
{
   const VarNodes& variables;
   int index;
   std::map<tree_nodeConstRef, int, tree_reindexCompare> dfs;
   std::map<tree_nodeConstRef, tree_nodeConstRef, tree_reindexCompare> root;
   std::set<tree_nodeConstRef, tree_reindexCompare> inComponent;
   std::map<tree_nodeConstRef, CustomSet<VarNode*>, tree_reindexCompare> components;
   std::deque<tree_nodeConstRef> worklist;

   #ifdef SCC_DEBUG
   bool checkWorklist() const;
   bool checkComponents() const;
   bool checkTopologicalSort(const UseMap& useMap) const;
   bool hasEdge(const CustomSet<VarNode*>& componentFrom, const CustomSet<VarNode*>& componentTo, const UseMap& useMap) const;
   #endif
 
 public:
   Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap);
   ~Nuutila() = default;
   Nuutila(const Nuutila&) = delete;
   Nuutila(Nuutila&&) = delete;
   Nuutila& operator=(const Nuutila&) = delete;
   Nuutila& operator=(Nuutila&&) = delete;

   void addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars);
   void delControlDependenceEdges(UseMap& useMap);
   void visit(const tree_nodeConstRef& V, std::stack<tree_nodeConstRef>& stack, const UseMap& useMap);

   const CustomSet<VarNode*>& getComponent(const tree_nodeConstRef& n) const
   {
      THROW_ASSERT(static_cast<bool>(components.count(n)), "Required component not found (" + GET_CONST_NODE(n)->ToString() + ")");
      return components.at(n);
   }

   using iterator = std::deque<tree_nodeConstRef>::reverse_iterator;
   using const_iterator = std::deque<tree_nodeConstRef>::const_reverse_iterator;
   iterator begin()
   {
      return worklist.rbegin();
   }
   const_iterator cbegin() const
   {
      return worklist.crbegin();
   }
   iterator end()
   {
      return worklist.rend();
   }
   const_iterator cend() const
   {
      return worklist.crend();
   }
};

/*
 *  Finds the strongly connected components in the constraint graph formed
 * by Variables and UseMap. The class receives the map of futures to insert
 * the control dependence edges in the constraint graph. These edges are removed
 * after the class is done computing the SCCs.
 */
Nuutila::Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap) : variables(varNodes)
{
   // Copy structures
   this->index = 0;

   // Iterate over all varnodes of the constraint graph
   for(const auto& vNode : varNodes)
   {
      // Initialize DFS control variable for each Value in the graph
      dfs[vNode.first] = -1;
   }
   addControlDependenceEdges(useMap, symbMap, varNodes);
   // Iterate again over all varnodes of the constraint graph
   for(const auto& vNode : varNodes)
   {
      // If the Value has not been visited yet, call visit for him
      if(dfs[vNode.first] < 0)
      {
         std::stack<tree_nodeConstRef> pilha;
         visit(vNode.first, pilha, useMap);
      }
   }
   delControlDependenceEdges(useMap);

   #ifdef SCC_DEBUG
   THROW_ASSERT(checkWorklist(), "An inconsistency in SCC worklist have been found");
   THROW_ASSERT(checkComponents(), "A component has been used more than once");
   THROW_ASSERT(checkTopologicalSort(useMap), "Topological sort is incorrect");
   #endif
}

/*
   *  Adds the edges that ensure that we solve a future before fixing its
   *  interval. I have created a new class: ControlDep edges, to represent
   *  the control dependencies. In this way, in order to delete these edges,
   *  one just need to go over the map of uses removing every instance of the
   *  ControlDep class.
   */
void Nuutila::addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars)
{
   for(const auto& [var, ops] : symbMap)
   {
      for(const auto& op : ops)
      {
         THROW_ASSERT(static_cast<bool>(vars.count(var)), "Variable should be stored in VarNodes map");
         auto* source = vars.at(var);
         auto* cdedge = new ControlDepNode(op->getSink(), source);
         useMap[var].insert(cdedge);
      }
   }
}

/*
 *  Removes the control dependence edges from the constraint graph.
 */
void Nuutila::delControlDependenceEdges(UseMap& useMap)
{
   for(auto& varOps : useMap)
   {
      std::deque<ControlDepNode*> cds;
      for(auto sit : varOps.second)
      {
         if(auto* cd = dynamic_cast<ControlDepNode*>(sit))
         {
            cds.push_back(cd);
         }
      }

      for(auto* cd : cds)
      {
         // Add pseudo edge to the string
         const auto& V = cd->getSource()->getValue();
         if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
         {
            pseudoEdgesString << " " << C->value << " -> ";
         }
         else
         {
            pseudoEdgesString << " " << '"';
            printVarName(V, pseudoEdgesString);
            pseudoEdgesString << '"' << " -> ";
         }
         const auto& VS = cd->getSink()->getValue();
         pseudoEdgesString << '"';
         printVarName(VS, pseudoEdgesString);
         pseudoEdgesString << '"';
         pseudoEdgesString << " [style=dashed]\n";
         // Remove pseudo edge from the map
         varOps.second.erase(cd);
      }
   }
}

/*
   *  Finds SCCs using Nuutila's algorithm. This algorithm is divided in
   *  two parts. The first calls the recursive visit procedure on every node
   *  in the constraint graph. The second phase revisits these nodes,
   *  grouping them in components.
   */
void Nuutila::visit(const tree_nodeConstRef& V, std::stack<tree_nodeConstRef>& stack, const UseMap& useMap)
{
   dfs[V] = index;
   ++index;
   root[V] = V;

   // Visit every node defined in an instruction that uses V
   for(const auto& op : useMap.at(V))
   {
      auto name = op->getSink()->getValue();
      if(dfs[name] < 0)
      {
         visit(name, stack, useMap);
      }
      if((!static_cast<bool>(inComponent.count(name))) && (dfs[root[V]] >= dfs[root[name]]))
      {
         root[V] = root[name];
      }
   }

   // The second phase of the algorithm assigns components to stacked nodes
   if(GET_INDEX_CONST_NODE(root[V]) == GET_INDEX_CONST_NODE(V))
   {
      // Neither the worklist nor the map of components is part of Nuutila's
      // original algorithm. We are using these data structures to get a
      // topological ordering of the SCCs without having to go over the root
      // list once more.
      worklist.push_back(V);
      components[V].insert(variables.at(V));
      inComponent.insert(V);
      while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
      {
         auto node = stack.top();
         stack.pop();
         inComponent.insert(node);
         components[V].insert(variables.at(node));
      }
   }
   else
   {
      stack.push(V);
   }
}

#ifdef SCC_DEBUG
bool Nuutila::checkWorklist() const
{
   bool consistent = true;
   for(auto nit = cbegin(), nend = cend(); nit != nend;)
   {
      auto v1 = *nit;
      for(const auto& v2 : boost::make_iterator_range(++nit, cend()))
      {
         if(GET_INDEX_CONST_NODE(v1) == GET_INDEX_CONST_NODE(v2))
         {
            PRINT_MSG("[Nuutila::checkWorklist] Duplicated entry in worklist" << std::endl << GET_CONST_NODE(v1)->ToString());
            consistent = false;
         }
      }
   }
   return consistent;
}

bool Nuutila::checkComponents() const
{
   bool isConsistent = true;
   for(auto n1it = cbegin(), n1end = cend(); n1it != n1end;)
   {
      const auto& component1 = components.at(*n1it);
      for(const auto& n2 : boost::make_iterator_range(++n1it, cend()))
      {
         const auto& component2 = components.at(n2);
         if(&component1 == &component2)
         {
            PRINT_MSG("[Nuutila::checkComponent] Component [" << &component1 << ", " << component1.size() << "]");
            isConsistent = false;
         }
      }
   }
   return isConsistent;
}

/**
 * Check if a component has an edge to another component
 */
bool Nuutila::hasEdge(const CustomSet<VarNode*>& componentFrom, const CustomSet<VarNode*>& componentTo, const UseMap& useMap) const
{
   for(const auto& v : componentFrom)
   {
      const auto source = v->getValue();
      THROW_ASSERT(static_cast<bool>(useMap.count(source)), "Variable should be in use map");
      for(const auto& op : useMap.at(source))
      {
         if(static_cast<bool>(componentTo.count(op->getSink())))
         {
            return true;
         }
      }
   }
   return false;
}

bool Nuutila::checkTopologicalSort(const UseMap& useMap) const
{
   bool isConsistent = true;
   for(auto n1it = cbegin(), nend = cend(); n1it != nend; ++n1it)
   {
      const auto& curr_component = components.at(*n1it);
      // check if this component points to another component that has already
      // been visited
      for(const auto& n2 : boost::make_iterator_range(cbegin(), n1it))
      {
         const auto& prev_component = components.at(n2);
         if(hasEdge(curr_component, prev_component, useMap))
         {
            isConsistent = false;
         }
      }
   }
   return isConsistent;
}

#endif

// ========================================================================== //
// Meet
// ========================================================================== //
class Meet
{
 private:
   static const APInt& getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val);
   static const APInt& getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val);

 public:
   static bool widen(OpNode* op, const std::vector<APInt>& constantvector);
   static bool narrow(OpNode* op, const std::vector<APInt>& constantvector);
   static bool crop(OpNode* op);
   static bool growth(OpNode* op);
   static bool fixed(OpNode* op);

   #ifndef NDEBUG
   static int debug_level;
   #endif
};

#ifndef NDEBUG
int Meet::debug_level = DEBUG_LEVEL_NONE;
#endif

/*
   * Get the first constant from vector greater than val
   */
const APInt& Meet::getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(const auto& vapint : constantvector)
   {
      if(vapint >= val)
      {
         return vapint;
      }
   }
   return Max;
}

/*
   * Get the first constant from vector less than val
   */
const APInt& Meet::getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
   {
      const auto& vapint = *vit;
      if(vapint <= val)
      {
         return vapint;
      }
   }
   return Min;
}

bool Meet::fixed(OpNode* op)
{
   const auto oldInterval = op->getSink()->getRange();
   const auto newInterval = op->eval();

   op->getSink()->setRange(newInterval);
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldInterval->ToString() + " -> " + newInterval->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED::%artificial phi : " + oldInterval->ToString() + " -> " + newInterval->ToString());
   }
   return !oldInterval->isSameRange(newInterval);
}

/// This is the meet operator of the growth analysis. The growth analysis
/// will change the bounds of each variable, if necessary. Initially, each
/// variable is bound to either the undefined interval, e.g. [., .], or to
/// a constant interval, e.g., [3, 15]. After this analysis runs, there will
/// be no undefined interval. Each variable will be either bound to a
/// constant interval, or to [-, c], or to [c, +], or to [-, +].
bool Meet::widen(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalWiden = [&](RangeConstRef oldInterval, RangeConstRef newInterval)
   {
      const auto bw = oldInterval->getBitWidth();
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() || newInterval->isAnti())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oldLower = oldAnti->getLower();
            const auto& oldUpper = oldAnti->getUpper();
            const auto& newLower = newAnti->getLower();
            const auto& newUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(newLower > oldLower || newUpper < oldUpper)
            {
               return RangeRef(new Range(Anti, bw, newLower > oldLower ? nlconstant : oldLower, newUpper < oldUpper ? nuconstant : oldUpper));
            }
         }
         
            return RangeRef(newInterval->clone());
         }
      else
      {
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         // Jump-set
         const auto& nlconstant = getFirstLessFromVector(constantvector, newLower);
         const auto& nuconstant = getFirstGreaterFromVector(constantvector, newUpper);

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? nlconstant : oldLower, newUpper > oldUpper ? nuconstant : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::widen unreachable state");
      return RangeRef(oldInterval->clone());
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = RefcountCast<const RealRange>(oldRange);
      const auto newRR = RefcountCast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getFractional()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getFractional()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalWiden(oldIntervals[i], newIntervals[i]);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalWiden(oldRange, newRange));
   }
   
   const auto sinkRange = op->getSink()->getRange();

   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "WIDEN::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + newRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "WIDEN::%artificial phi : " + oldRange->ToString() + " -> " + newRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::growth(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalGrowth = [](RangeConstRef oldInterval, RangeConstRef newInterval)
   {
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() || newInterval->isAnti())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         auto bw = oldInterval->getBitWidth();
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? Min : oldLower, newUpper > oldUpper ? Max : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::growth unreachable state");
      return RangeRef(oldInterval->clone());
   }; 

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = RefcountCast<const RealRange>(oldRange);
      const auto newRR = RefcountCast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getFractional()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getFractional()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalGrowth(oldIntervals[i], newIntervals[i]);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalGrowth(oldRange, newRange));
   }
   
   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "GROWTH::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "GROWTH::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

/// This is the meet operator of the cropping analysis. Whereas the growth
/// analysis expands the bounds of each variable, regardless of intersections
/// in the constraint graph, the cropping analysis shrinks these bounds back
/// to ranges that respect the intersections.
bool Meet::narrow(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalNarrow = [&](RangeConstRef oldInterval, RangeConstRef newInterval)
   {
      auto bw = oldInterval->getBitWidth();
      RangeRef sinkInterval(oldInterval->clone());
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oLower = oldAnti->getLower();
            const auto& oUpper = oldAnti->getUpper();
            const auto& nLower = newAnti->getLower();
            const auto& nUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, nLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, nUpper);
            const auto smin = std::max({oLower, nlconstant});
            const auto smax = std::min({oUpper, nuconstant});
            THROW_ASSERT(oLower != Min, "");
            THROW_ASSERT(oUpper != Max, "");

            if(oLower != smin)
            {
               sinkInterval = RangeRef(new Range(Anti, bw, smin, oUpper));
            }
            if(oUpper != smax)
            {
               if(sinkInterval->isAnti())
               {
                  const auto sinkAnti = sinkInterval->getAnti();
                  sinkInterval = RangeRef(new Range(Anti, bw, sinkAnti->getLower(), smax));
               }
               else
               {
                  sinkInterval = RangeRef(new Range(Anti, bw, sinkInterval->getLower(), smax));
               }
            }
         }
         else
         {
            sinkInterval = RangeRef(newInterval->clone());
         }
      }
      else
      {
         const auto& oLower = oldInterval->getLower();
         const auto& oUpper = oldInterval->getUpper();
         const auto& nLower = newInterval->getLower();
         const auto& nUpper = newInterval->getUpper();
         if((oLower == Min) && (nLower == Min))
         {
            sinkInterval = RangeRef(new Range(Regular, bw, nLower, oUpper));
         }
         else
         {
            const auto smin = std::min({oLower, nLower});
            if(oLower != smin)
            {
               sinkInterval = RangeRef(new Range(Regular, bw, smin, oUpper));
            }
         }
         if(!sinkInterval->isAnti())
         {
            if((oUpper == Max) && (nUpper == Max))
            {
               sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), nUpper));
            }
            else
            {
               const auto smax = std::max({oUpper, nUpper});
               if(oUpper != smax)
               {
                  sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), smax));
               }
            }
         }
      }
      return sinkInterval;
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = RefcountCast<const RealRange>(oldRange);
      const auto newRR = RefcountCast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getFractional()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getFractional()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalNarrow(oldIntervals[i], newIntervals[i]);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalNarrow(oldRange, newRange));
   }
   
   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NARROW::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NARROW::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::crop(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();
   const char _abstractState = op->getSink()->getAbstractState();

   auto intervalCrop = [](RangeConstRef oldInterval, RangeConstRef newInterval, char abstractState)
   {
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         const auto bw = oldInterval->getBitWidth();
         if((abstractState == '-' || abstractState == '?') && (newInterval->getLower() > oldInterval->getLower()))
         {
            return RangeRef(new Range(Regular, bw, newInterval->getLower(), oldInterval->getUpper()));
         }

         if((abstractState == '+' || abstractState == '?') && (newInterval->getUpper() < oldInterval->getUpper()))
         {
            return RangeRef(new Range(Regular, bw, oldInterval->getLower(), newInterval->getUpper()));
         }
         return RangeRef(oldInterval->clone());
      }
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = RefcountCast<const RealRange>(oldRange);
      const auto newRR = RefcountCast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getFractional()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getFractional()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalCrop(oldIntervals[i], newIntervals[i], _abstractState);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalCrop(oldRange, newRange, _abstractState));
   }
   
   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "CROP::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "CROP::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

// ========================================================================== //
// ConstraintGraph
// ========================================================================== //
using CallMap = CustomMap<unsigned int, std::list<tree_nodeConstRef>>;

using ParmMap = CustomMap<unsigned int, std::pair<bool, std::vector<tree_nodeConstRef>>>;

class ConstraintGraph : public NodeContainer
{
 protected:

   // Perform the widening and narrowing operations
   void update(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& actv, std::function<bool(OpNode*,const std::vector<APInt>&)> meet)
   {
      while(!actv.empty())
      {
         const auto V = *actv.begin();
         actv.erase(V);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-> update: " + GET_CONST_NODE(V)->ToString());

         // The use list.
         const auto& L = compUseMap.at(V);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
         for(auto* op : L)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "> " + op->getSink()->ToString());
            if(meet(op, constantvector))
            {
               // I want to use it as a set, but I also want
               // keep an order or insertions and removals.
               const auto val = op->getSink()->getValue();
               actv.insert(val);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
      }
   }
   
   void update(size_t nIterations, const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& actv)
   {
      std::deque<tree_nodeConstRef> queue(actv.begin(), actv.end());
      actv.clear();
      while(!queue.empty())
      {
         const auto V = queue.front();
         queue.pop_front();
         // The use list.
         const auto& L = compUseMap.at(V);
         for(auto op : L)
         {
            if(nIterations == 0)
            {
               return;
            }
            --nIterations;
            if(Meet::fixed(op))
            {
               const auto next = op->getSink()->getValue();
               if(std::find(queue.begin(), queue.end(), next) == queue.end())
               {
                  queue.push_back(next);
               }
            }
         }
      }
   }

   virtual void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) = 0;
   virtual void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& activeVars, const CustomSet<VarNode*>& component) = 0;

 private:
   #ifndef NDEBUG
   int debug_level;
   int graph_debug;
   #endif

   const application_managerRef AppM;

   // A map from variables to the operations where these
   // variables are present as bounds
   SymbMap symbMap;
   // A map from functions to the operations where they are called
   CallMap callMap;
   // A map from functions to the ssa_name associated with parm_decl (bool value is true when all parameters are associated with a variable)
   ParmMap parmMap;

   // Vector containing the constants from a SCC
   // It is cleared at the beginning of every SCC resolution
   std::vector<APInt> constantvector;

   /**
    * @brief Analyse branch instruction and build conditional value range
    * 
    * @param br Branch instruction
    * @param branchBB Branch basic block
    * @param function_id Function id
    * @return unsigned int Return dead basic block to be removed when necessary and possible (bloc::ENTRY_BLOCK_ID indicates no dead block found, bloc::EXIT_BLOCK_ID indicates constant codition was found but could not be evaluated) 
    */
   unsigned int buildCVR(const gimple_cond* br, const blocRef branchBB, unsigned int function_id)
   {
      if(GetPointer<const cst_node>(GET_CONST_NODE(br->op0)) != nullptr)
      {
         return evaluateBranch(br->op0, branchBB
            #ifndef NDEBUG
            , debug_level
            #endif
            );
      }
      THROW_ASSERT(GET_CONST_NODE(br->op0)->get_kind() == ssa_name_K, "Non SSA variable found in branch (" + GET_CONST_NODE(br->op0)->get_kind_text() + " " + GET_CONST_NODE(br->op0)->ToString() + ")");
      const auto Cond = branchOpRecurse(br->op0);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition is " + Cond->get_kind_text() + " " + Cond->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      if(const auto* bin_op = GetPointer<const binary_expr>(Cond))
      {
         if(!isCompare(bin_op))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a compare codition, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         if(!isValidType(bin_op->op0) || !isValidType(bin_op->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Non-integer operands, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         // Create VarNodes for comparison operands explicitly
         addVarNode(bin_op->op0, function_id);
         addVarNode(bin_op->op1, function_id);

         // Gets the successors of the current basic block.
         const auto TrueBBI = branchBB->true_edge;
         const auto FalseBBI = branchBB->false_edge;

         // We have a Variable-Constant comparison.
         const auto Op0 = GET_CONST_NODE(bin_op->op0);
         const auto Op1 = GET_CONST_NODE(bin_op->op1);
         tree_nodeConstRef constant = nullptr;
         tree_nodeConstRef variable = nullptr;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

         // If both operands are constants, nothing to do here
         if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return evaluateBranch(br->op0, branchBB
               #ifndef NDEBUG
               , debug_level
               #endif
               );
         }

         // Then there are two cases: variable being compared to a constant,
         // or variable being compared to another variable

         // Op0 is constant, Op1 is variable
         if(GetPointer<const cst_node>(Op0) != nullptr)
         {
            constant = Op0;
            variable = bin_op->op1;
            // Op0 is variable, Op1 is constant
         }
         else if(GetPointer<const cst_node>(Op1) != nullptr)
         {
            constant = Op1;
            variable = bin_op->op0;
         }
         // Both are variables
         // which means constant == 0 and variable == 0

         if(constant != nullptr)
         {
            const kind pred = isSignedType(variable) ? bin_op->get_kind() : op_unsigned(bin_op->get_kind());
            const kind swappred = op_swap(pred);
            RangeRef CR = getGIMPLE_range(constant);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable bitwidth is " + STR(getGIMPLE_BW(variable)) + " and constant value is " + constant->ToString());

            const auto TValues = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0)) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + TValues->ToString());

            const auto FValues = TValues->isFullSet() ? getEmptyFor(variable) : TValues->getAnti();

            // Create the interval using the intersection in the branch.
            const auto BT = ValueRangeRef(new ValueRange(TValues));
            const auto BF = ValueRangeRef(new ValueRange(FValues));

            addConditionalValueRange(ConditionalValueRange(variable, TrueBBI, FalseBBI, BT, BF));

            // Do the same for the operand of variable (if variable is a cast
            // instruction)
            if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  #ifndef NDEBUG
                  if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());
                  }
                  #endif

                  const auto _BT = ValueRangeRef(new ValueRange(TValues));
                  const auto _BF = ValueRangeRef(new ValueRange(FValues));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, _BT, _BF));
               }
            }
         }
         else
         {
            const kind pred = isSignedType(bin_op->op0) ? bin_op->get_kind() : op_unsigned(bin_op->get_kind());
            const kind invPred = op_inv(pred);
            const kind swappred = op_swap(pred);
            const kind invSwappred = op_inv(swappred);

            #if !defined(NDEBUG) or HAVE_ASSERTS
            const auto bw0 = getGIMPLE_BW(bin_op->op0);
            #endif
            #if HAVE_ASSERTS
            const auto bw1 = getGIMPLE_BW(bin_op->op1);
            THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) + ", Op1 = " + STR(bw1) + ").");
            #endif

            const auto CR = getUnknownFor(bin_op->op0);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,"Variables bitwidth is " + STR(bw0));

            // Symbolic intervals for op0
            const auto STOp0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, pred));
            const auto SFOp0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, invPred));

            addConditionalValueRange(ConditionalValueRange(bin_op->op0, TrueBBI, FalseBBI, STOp0, SFOp0));

            // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op0))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());

                  const auto STOp0_0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, pred));
                  const auto SFOp0_0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, invPred));
               
                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp0_0, SFOp0_0));
               }
            }

            // Symbolic intervals for op1
            const auto STOp1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, swappred));
            const auto SFOp1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, invSwappred));
            addConditionalValueRange(ConditionalValueRange(bin_op->op1, TrueBBI, FalseBBI, STOp1, SFOp1));

            // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op1))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());

                  const auto STOp1_1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, swappred));
                  const auto SFOp1_1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, invSwappred));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp1_1, SFOp1_1));
               }
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare codition, skipping...");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return bloc::ENTRY_BLOCK_ID;
   }

   bool buildCVR(const gimple_multi_way_if* mwi, const blocRef /*mwifBB*/, unsigned int function_id)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Multi-way if with " + STR(mwi->list_of_cond.size()) + " conditions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      // Find else branch BBI if any
      unsigned int DefaultBBI = 0;
      for(const auto& [cond, BBI] : mwi->list_of_cond)
      {
         if(!cond)
         {
            DefaultBBI = BBI;
            break;
         }
      }

      // Analyse each if branch condition
      CustomMap<tree_nodeConstRef, std::map<unsigned int, ValueRangeRef>> switchSSAMap;
      for(const auto& [cond, BBI] : mwi->list_of_cond)
      {
         if(!cond)
         {
            // Default branch is handled at the end
            continue;
         }

         if(GetPointer<const cst_node>(GET_CONST_NODE(cond)) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a cst_node, dead code elimination necessary!");
            // TODO: abort and call dead code elimination to evaluate constant condition
            //    return true;
            continue;
         }
         THROW_ASSERT(GET_CONST_NODE(cond)->get_kind() == ssa_name_K, "Case conditional variable should be an ssa_name (" + GET_CONST_NODE(cond)->get_kind_text() + " " + GET_CONST_NODE(cond)->ToString() + ")");
         const auto case_compare = branchOpRecurse(cond);
         if(const auto* cmp_op = GetPointer<const binary_expr>(case_compare))
         {
            if(!isCompare(cmp_op))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare codition, skipping...");
               continue;
            }

            if(!isValidType(cmp_op->op0) || !isValidType(cmp_op->op1))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Non-integer operands, skipping...");
               continue;
            }

            // Create VarNodes for comparison operands explicitly
            addVarNode(cmp_op->op0, function_id);
            addVarNode(cmp_op->op1, function_id);

            // We have a Variable-Constant comparison.
            const auto Op0 = GET_CONST_NODE(cmp_op->op0);
            const auto Op1 = GET_CONST_NODE(cmp_op->op1);
            const struct integer_cst* constant = nullptr;
            tree_nodeConstRef variable = nullptr;

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

            // If both operands are constants, nothing to do here
            if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Both operands are constants, dead code elimination necessary!");
               // TODO: abort and call dead code elimination to evaluate constant condition
               //    return true;
               continue;
            }

            // Then there are two cases: variable being compared to a constant,
            // or variable being compared to another variable

            // Op0 is constant, Op1 is variable
            if((constant = GetPointer<const integer_cst>(Op0)) != nullptr)
            {
               variable = cmp_op->op1;
            }  
            else if((constant = GetPointer<const integer_cst>(Op1)) != nullptr)
            {
               // Op0 is variable, Op1 is constant
               variable = cmp_op->op0;
            }
            // Both are variables
            // which means constant == 0 and variable == 0

            if(constant != nullptr)
            {
               const kind pred = isSignedType(variable) ? cmp_op->get_kind() : op_unsigned(cmp_op->get_kind());
               const kind swappred = op_swap(pred);
               const auto bw = getGIMPLE_BW(variable);
               RangeConstRef CR(new Range(Regular, bw, constant->value, constant->value));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable bitwidth is " + STR(bw) + " and constant value is " + STR(constant->value));

               const auto tmpT = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0)) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + tmpT->ToString());

               RangeRef TValues = tmpT->isFullSet() ? RangeRef(new Range(Regular, bw)) : tmpT;

               // Create the interval using the intersection in the branch.
               auto BT = ValueRangeRef(new ValueRange(TValues));
               switchSSAMap[variable].insert(std::make_pair(BBI, BT));

               // Do the same for the operand of variable (if variable is a cast
               // instruction)
               if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     #ifndef NDEBUG
                     if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());
                     }
                     #endif

                     auto _BT = ValueRangeRef(new ValueRange(TValues));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(BBI, _BT));
                  }
               }
            }
            else
            {
               const kind pred = isSignedType(cmp_op->op0) ? cmp_op->get_kind() : op_unsigned(cmp_op->get_kind());
               const kind swappred = op_swap(pred);

               #if !defined(NDEBUG) or HAVE_ASSERTS
               const auto bw0 = getGIMPLE_BW(cmp_op->op0);
               #endif
               #if HAVE_ASSERTS
               const auto bw1 = getGIMPLE_BW(cmp_op->op1);
               THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) + ", Op1 = " + STR(bw1) + ").");
               #endif

               const auto CR = getUnknownFor(cmp_op->op0);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,"Variables bitwidth is " + STR(bw0));

               // Symbolic intervals for op0
               const auto STOp0 =ValueRangeRef(new SymbRange(CR, cmp_op->op1, pred));
               switchSSAMap[cmp_op->op0].insert(std::make_pair(BBI, STOp0));

               // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op0))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp0_0 =ValueRangeRef(new SymbRange(CR, cmp_op->op1, pred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(BBI, STOp0_0));
                  }
               }

               // Symbolic intervals for op1
               const auto STOp1 =ValueRangeRef(new SymbRange(CR, cmp_op->op0, swappred));
               switchSSAMap[cmp_op->op1].insert(std::make_pair(BBI, STOp1));

               // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op1))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp1_1 =ValueRangeRef(new SymbRange(CR, cmp_op->op0, swappred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(BBI, STOp1_1));
                  }
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Multi-way condition different from binary_expr not handled, skipping... (" + case_compare->get_kind_text() + " " + case_compare->ToString() + ")");
         }
      }

      // Handle else branch, if there is any
      // TODO: maybe it should be better to leave fullset as interval for default edge 
      //       because usign getAnti implies internal values to be excluded while they 
      //       could still be valid values
      if(static_cast<bool>(DefaultBBI))
      {
         for(auto& [var, VSM] : switchSSAMap)
         {
            auto elseRange = getEmptyFor(var);
            for(const auto& BBIinterval : VSM)
            {
               elseRange = elseRange->unionWith(BBIinterval.second->getRange());
            }
            elseRange = elseRange->getAnti();
            VSM.insert(std::make_pair(DefaultBBI, ValueRangeRef(new ValueRange(elseRange))));
         }
      }

      for(const auto& [var, VSM] : switchSSAMap)
      {
         addConditionalValueRange(ConditionalValueRange(var, VSM));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return false;
   }

   void addStoreOp(const tree_nodeConstRef /*I*/, unsigned int /*function_id*/, const tree_managerConstRef /*TM*/)
   {
      // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing store instruction " + GetPointer<const gimple_assign>(GET_CONST_NODE(I))->ToString());
      /*
      VarNode* sink = addVarNode(ga->op0);
      auto bw = getGIMPLE_BW(ga->op1);
      Range intersection(Regular, bw, Min, Max);
      const auto Op0 = GET_CONST_NODE(ga->op0);
      CustomOrderedSet<unsigned int> res_set;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      if(tree_helper::is_fully_resolved(TM, Op0->index, res_set))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Pointer is fully resolved");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         //    bool pointToConstants = true;
         for(const auto& index : res_set)
         {
            const auto TN = TM->CGetTreeNode(index);
            if(const auto* vd = GetPointer<const var_decl>(TN))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Points to " + TN->ToString() + 
                  " (readonly = " + STR(vd->readonly_flag) + ", defs = " + STR(vd->defs.size()) + ")");
               if(const auto* constr = GetPointer<const constructor>(GET_CONST_NODE(vd->init)))
               {
                  for(const auto& [idx, valu] : constr->list_of_idx_valu)
                  {
                     StoreOp* storeOp;
                     if(tree_helper::is_constant(TM, GET_INDEX_CONST_NODE(idx)) && isValidType(idx))
                     {
                        storeOp = new StoreOp(sink, I, getGIMPLE_range(idx));
                     }
                     else
                     {
                        storeOp = new StoreOp(sink, I, RangeRef(new Range(Empty, bw)));
                     }
                     this->oprs.insert(storeOp);
                     this->defMap[sink->getValue()] = storeOp;
                     VarNode* source = addVarNode(ga->op1);
                     storeOp->addSource(source);
                     this->useMap.find(source->getValue())->second.insert(storeOp);
                  }
               }
               else
               {
                  THROW_UNREACHABLE("Unhandled initializer " + GET_CONST_NODE(vd->init)->get_kind_text() + " " + GET_CONST_NODE(vd->init)->ToString());
                  continue;
               }
            }
            else
            {
               THROW_UNREACHABLE("Unknown tree node " + TN->get_kind_text() + " " + TN->ToString());
               continue;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      */
      
      /*
      // LLVM implementation
   #if HAVE_LIBBDD
      if(arePointersResolved)
      {
         assert(PtoSets_AA);
         auto PO = SI->getPointerOperand();
         assert(PtoSets_AA->PE(PO) != NOVAR_ID);
         auto bw = SI->getValueOperand()->getType()->getPrimitiveSizeInBits();
         Range intersection(Regular, bw, Min, Max);

         for(auto var : *PtoSets_AA->pointsToSet(PO))
         {
            auto varValue = PtoSets_AA->getValue(var);
            assert(varValue);
            VarNode* sink = addVarNode(SI, varValue, DL);
            StoreOp* storeOp;
            if(dyn_cast<llvm::GlobalVariable>(varValue) && dyn_cast<llvm::GlobalVariable>(varValue)->hasInitializer())
            {
               storeOp = new StoreOp(sink, SI, getLLVM_range(dyn_cast<llvm::GlobalVariable>(varValue)->getInitializer()));
            }
            else
            {
               storeOp = new StoreOp(sink, SI, Range(Empty, bw));
            }
            this->oprs.insert(storeOp);
            this->defMap[sink->getValue()] = storeOp;
            VarNode* source = addVarNode(SI->getValueOperand(), nullptr, DL);
            storeOp->addSource(source);
            this->useMap.find(source->getValue())->second.insert(storeOp);

            if(isAggregateValue(varValue))
            {
               //               llvm::errs() << "SI: ";
               //               SI->print(llvm::errs());
               //               llvm::errs() << "\n";
               //               llvm::errs() << "    isAggregateValue: ";
               //               varValue->print(llvm::errs());
               //               llvm::errs() << "\n";
               for(const Value* operand : ComputeConflictingStores(SI, varValue, SI, PtoSets_AA, Function2Store, modulePass))
               {
                  //                  llvm::errs() << "  source: ";
                  //                  operand->print(llvm::errs());
                  //                  llvm::errs() << "\n";
                  VarNode* source = addVarNode(operand, varValue, DL);
                  storeOp->addSource(source);
                  this->useMap.find(source->getValue())->second.insert(storeOp);
               }
            }
         }
      }
   #endif
      */
   }

   /*
      *	This method builds a map that binds each variable label to the
      * operations
      * where this variable is used.
      */
   UseMap buildUseMap(const CustomSet<VarNode*>& component)
   {
      UseMap compUseMap;
      for(auto vit = component.begin(), vend = component.end(); vit != vend; ++vit)
      {
         const VarNode* var = *vit;
         const auto V = var->getValue();
         // Get the component's use list for V (it does not exist until we try to get it)
         auto& list = compUseMap[V];
         // Get the use list of the variable in component
         auto p = getUses().find(V);
         // For each operation in the list, verify if its sink is in the component
         for(auto* opit : p->second)
         {
            VarNode* sink = opit->getSink();
            // If it is, add op to the component's use map
            if(static_cast<bool>(component.count(sink)))
            {
               list.insert(opit);
            }
         }
      }
      return compUseMap;
   }

   /*
      * Used to insert constant in the right position
      */
   void insertConstantIntoVector(const APInt& constantval)
   {
      constantvector.push_back(constantval);
   }

   /*
      * Create a vector containing all constants related to the component
      * They include:
      *   - Constants inside component
      *   - Constants that are source of an edge to an entry point
      *   - Constants from intersections generated by sigmas
      */
   void buildConstantVector(const CustomSet<VarNode*>& component, const UseMap& compusemap)
   {
      // Remove all elements from the vector
      constantvector.clear();

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
         {
            insertConstantIntoVector(tree_helper::get_integer_cst_value(ic));
         }
      }

      // Get constants that are sources of operations whose sink belong to the
      // component
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         auto dfit = getDefs().find(V);
         if(dfit == getDefs().end())
         {
            continue;
         }

         auto pushConstFor = [this](const APInt& cst, bw_t bw, kind pred) {
            if(isCompare(pred))
            {
               if(pred == eq_expr_K || pred == ne_expr_K)
               {
                  insertConstantIntoVector(cst);
                  insertConstantIntoVector(cst - 1);
                  insertConstantIntoVector(cst + 1);
               }
               else if(pred == uneq_expr_K)
                  {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst);
                  insertConstantIntoVector(ucst - 1);
                  insertConstantIntoVector(ucst + 1);
                  }
                  else if(pred == gt_expr_K || pred == le_expr_K)
                  {
                  insertConstantIntoVector(cst);
                  insertConstantIntoVector(cst + 1);
                  }
                  else if(pred == ge_expr_K || pred == lt_expr_K)
                  {
                  insertConstantIntoVector(cst);
                  insertConstantIntoVector(cst - 1);
                  }
                  else if(pred == ungt_expr_K || pred == unle_expr_K)
                  {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst);
                  insertConstantIntoVector(ucst + 1);
                  }
                  else if(pred == unge_expr_K || pred == unlt_expr_K)
                  {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst);
                  insertConstantIntoVector(ucst - 1);
                  }
                  else
                  {
                  THROW_UNREACHABLE("unexpected condition (" + tree_node::GetString(pred) + ")");
                  }
               }
               else
               {
               insertConstantIntoVector(cst);
                  }
         };

         // Handle BinaryOp case
         if(const auto* bop = dynamic_cast<BinaryOpNode*>(dfit->second))
                  {
            const auto* source1 = bop->getSource1();
            const auto& sourceval1 = source1->getValue();
            const auto* source2 = bop->getSource2();
            const auto& sourceval2 = source2->getValue();

            const auto pred = bop->getOpcode();

            if(const auto *const1 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval1)))
                  {
               const auto bw = source1->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const1);
               pushConstFor(cst_val, bw, pred); // TODO: maybe should swap predicate for lhs constant?
               }
            if(const auto* const2 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval2)))
               {
               const auto bw = source2->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const2);
               pushConstFor(cst_val, bw, pred);
            }
         }
         // Handle PhiOp case
         else if(const auto* pop = dynamic_cast<PhiOpNode*>(dfit->second))
         {
            for(size_t i = 0, e = pop->getNumSources(); i < e; ++i)
            {
               const VarNode* source = pop->getSource(i);
               const auto& sourceval = source->getValue();
               if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval)))
               {
                  insertConstantIntoVector(tree_helper::get_integer_cst_value(ic));
               }
            }
         }
      }

      // Get constants used in intersections
      for(const auto& varOps : compusemap)
      {
         for(auto* op : varOps.second)
         {
            const auto* sigma = dynamic_cast<SigmaOpNode*>(op);
            // Symbolic intervals are discarded, as they don't have fixed values yet
            if(sigma == nullptr || SymbRange::classof(sigma->getIntersect().get()))
            {
               continue;
            }
            const auto rintersect = op->getIntersect()->getRange();
            if(rintersect->isAnti())
            {
               const auto anti = rintersect->getAnti();
               const auto& lb = anti->getLower();
               const auto& ub = anti->getUpper();
               if((lb != Min) && (lb != Max))
               {
                  insertConstantIntoVector(lb - 1);
                  insertConstantIntoVector(lb);
               }
               if((ub != Min) && (ub != Max))
               {
                  insertConstantIntoVector(ub);
                  insertConstantIntoVector(ub + 1);
               }
            }
            else
            {
               const auto& lb = rintersect->getLower();
               const auto& ub = rintersect->getUpper();
               if((lb != Min) && (lb != Max))
               {
                  insertConstantIntoVector(lb - 1);
                  insertConstantIntoVector(lb);
               }
               if((ub != Min) && (ub != Max))
               {
                  insertConstantIntoVector(ub);
                  insertConstantIntoVector(ub + 1);
               }
            }
         }
      }

      // Sort vector in ascending order and remove duplicates
      std::sort(constantvector.begin(), constantvector.end(), [](const APInt& i1, const APInt& i2) { return i1 < i2; });

      // std::unique doesn't remove duplicate elements, only
      // move them to the end
      // This is why erase is necessary. To remove these duplicates
      // that will be now at the end.
      auto last = std::unique(constantvector.begin(), constantvector.end());
      constantvector.erase(last, constantvector.end());
   }

   /*
      * This method builds a map of variables to the lists of operations where
      * these variables are used as futures. Its C++ type should be something like
      * map<VarNode, List<Operation>>.
      */
   void buildSymbolicIntersectMap()
   {
      // Creates the symbolic intervals map
      symbMap = SymbMap();

      // Iterate over the operations set
      for(auto* op : getOpNodes())
      {
         // If the operation is unary and its interval is symbolic
         auto* uop = dynamic_cast<UnaryOpNode*>(op);
         if((uop != nullptr) && SymbRange::classof(uop->getIntersect().get()))
         {
            const auto symbi = std::static_pointer_cast<const SymbRange>(uop->getIntersect());
            const auto V = symbi->getBound();
            auto p = symbMap.find(V);
            if(p != symbMap.end())
            {
               p->second.insert(uop);
            }
            else
            {
               CustomSet<OpNode*> l;
               l.insert(uop);
               symbMap.insert(std::make_pair(V, l));
            }
         }
      }
   }

   /*
      * This method evaluates once each operation that uses a variable in
      * component, so that the next SCCs after component will have entry
      * points to kick start the range analysis algorithm.
      */
   void propagateToNextSCC(const CustomSet<VarNode*>& component)
   {
      for(auto var : component)
      {
         const auto V = var->getValue();
         auto p = getUses().at(V);
         for(auto* op : p)
         {
            /// VarNodes belonging to the current SCC must not be evaluated otherwise we break the fixed point previously computed
            if(component.contains(op->getSink()))
            {
               continue;
            }
            auto* sigmaop = dynamic_cast<SigmaOpNode*>(op);
            op->getSink()->setRange(op->eval());
            if((sigmaop != nullptr) && sigmaop->getIntersect()->getRange()->isUnknown())
            {
               sigmaop->markUnresolved();
            }
         }
      }
   }

   void generateEntryPoints(const CustomSet<VarNode*>& component, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints)
   {
      // Iterate over the varnodes in the component
      for(VarNode* varNode : component)
      {
         const auto V = varNode->getValue();
         if(const auto* ssa = GetPointer<const ssa_name>(GET_CONST_NODE(V)))
         if(const auto* phi_def = GetPointer<const gimple_phi>(GET_CONST_NODE(ssa->CGetDefStmt())))
         if(phi_def->CGetDefEdgesList().size() == 1)
         {
            auto dit = getDefs().find(V);
            if(dit != getDefs().end())
            {
               auto* bop = dit->second;
               auto* defop = dynamic_cast<SigmaOpNode*>(bop);

               if((defop != nullptr) && defop->isUnresolved())
               {
                  defop->getSink()->setRange(bop->eval());
                  defop->markResolved();
               }
            }
         }
         if(!varNode->getRange()->isUnknown())
         {
            entryPoints.insert(V);
         }
      }
   }

   void solveFutures(const CustomSet<VarNode*>& component)
   {
      // Iterate again over the varnodes in the component
      for(auto* varNode : component)
      {
         solveFuturesSC(varNode);
      }
   }

   void solveFuturesSC(VarNode* varNode)
   {
      const auto& V = varNode->getValue();
      auto sit = symbMap.find(V);
      if(sit != symbMap.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Fix intersects: " + varNode->ToString());
         for(auto* op : sit->second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Op intersects: " + op->ToString());
            op->solveFuture(varNode);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Sink: " + op->ToString());
         }
      }
   }

   void generateActivesVars(const CustomSet<VarNode*>& component, std::set<tree_nodeConstRef, tree_reindexCompare>& activeVars)
   {
      for(auto* varNode : component)
      {
         const auto V = varNode->getValue();
         const auto* CI = GetPointer<const integer_cst>(GET_CONST_NODE(V));
         if(CI != nullptr)
         {
            continue;
         }
         activeVars.insert(V);
      }
   }

   void parametersBinding(const tree_nodeRef stmt, const struct function_decl* FD)
   {
      const auto& args = FD->list_of_args;
      auto parmMapIt = parmMap.find(FD->index);
      if(parmMapIt == parmMap.end())
      {
         parmMapIt = parmMap.insert(std::make_pair(FD->index, std::make_pair(false, std::vector<tree_nodeConstRef>(args.size(), nullptr)))).first;
      }
      auto& [foundAll, parmBind] = parmMapIt->second;
      // Skip ssa uses computation when all parameters have already been associated with a variable
      if(foundAll)
      {
         return;
      }

      const auto ssa_uses = tree_helper::ComputeSsaUses(stmt);
      for(const auto& [ssa, use_counter] : ssa_uses)
      {
         const auto* SSA = GetPointer<const ssa_name>(GET_CONST_NODE(ssa));
         // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function parameter inside the function body
         if(SSA->var != nullptr && GET_CONST_NODE(SSA->var)->get_kind() == parm_decl_K && GET_CONST_NODE(SSA->CGetDefStmt())->get_kind() == gimple_nop_K)
         {
            auto argIt = std::find_if(args.begin(), args.end(), [&](const tree_nodeRef& arg){ return GET_INDEX_CONST_NODE(arg) == GET_INDEX_CONST_NODE(SSA->var); });
            THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
            size_t arg_pos = static_cast<size_t>(argIt - args.begin());
            THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos) + " of function " + GetPointer<const identifier_node>(GET_CONST_NODE(FD->name))->strg);
            parmBind[arg_pos] = ssa;
            foundAll = std::find(parmBind.begin(), parmBind.end(), nullptr) == parmBind.end();
         }
      }
   }

   bool storeFunctionCall(const tree_nodeConstRef tn)
   {
      tree_nodeRef fun_node = nullptr;

      if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(tn)))
      if(const auto* ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1)))
      {
         fun_node = ce->fn;
         
      }
      if(const auto* ce = GetPointer<const gimple_call>(GET_CONST_NODE(tn)))
      {
         fun_node = ce->fn;
      }

      if(fun_node)
      {
         if(GET_NODE(fun_node)->get_kind() == addr_expr_K)
         {
            const auto* ue = GetPointer<const unary_expr>(GET_NODE(fun_node));
            fun_node = ue->op;
         }
         else if(GET_NODE(fun_node)->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(fun_node);
         }
         
         const auto* FD = GetPointer<const function_decl>(GET_CONST_NODE(fun_node));
         THROW_ASSERT(FD, "Function call should reference a function_decl node");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function call to " + 
            tree_helper::print_type(AppM->get_tree_manager(), FD->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(FD->index)->CGetBehavioralHelper()))));

         auto it = callMap.find(FD->index);
         if(it == callMap.end())
         {
            it = callMap.insert(std::make_pair(FD->index, std::list<tree_nodeConstRef>())).first;
         }
         it->second.emplace_back(tn);
         return true;
      }
      return false;
   }

 public:
 
   ConstraintGraph(application_managerRef _AppM 
   #ifndef NDEBUG
      , int _debug_level, int _graph_debug
   #else
      ,int,int
   #endif
      ) : 
   #ifndef NDEBUG
      debug_level(_debug_level), graph_debug(_graph_debug), 
   #endif
      AppM(_AppM)
      {
      #ifndef NDEBUG
      NodeContainer::debug_level = debug_level;
      #endif
      }

   virtual ~ConstraintGraph() = default;

   CallMap* getCallMap()
   {
      return &callMap;
   }
   const ParmMap &getParmMap()
   {
      return parmMap;
   }

   /// Iterates through all instructions in the function and builds the graph.
   bool buildGraph(unsigned int function_id)
   {
      const auto TM = AppM->get_tree_manager();
      const auto FB = AppM->CGetFunctionBehavior(function_id);
      const auto* FD = GetPointer<const function_decl>(TM->get_tree_node_const(function_id));
      const auto* SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
      #ifndef NDEBUG
      std::string fn_name = tree_helper::print_type(TM, function_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(FB->CGetBehavioralHelper())));
      #endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function " + fn_name + " with " + STR(SL->list_of_bloc.size()) + " blocks");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis...");
      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& stmt_list = idxBB.second->CGetStmtList();
         if(stmt_list.empty())
         {
            continue;
         }

         const auto terminator = GET_CONST_NODE(stmt_list.back());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(idxBB.first) + " has terminator type " + terminator->get_kind_text() + " " + terminator->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         if(const auto* br = GetPointer<const gimple_cond>(terminator))
         {
            #ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(br, idxBB.second, function_id))
            {
               // Dead code elimination necessary
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return true;
            }
            #else
            buildCVR(br, idxBB.second, function_id);
            #endif
         }
         else if(const auto* mwi = GetPointer<const gimple_multi_way_if>(terminator))
         {
            #ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(mwi, idxBB.second, function_id))
            {
               // Dead code elimination necessary
               return true;
            }
            #else
            buildCVR(mwi, idxBB.second, function_id);
            #endif
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis completed");

      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& phi_list = idxBB.second->CGetPhiList();
         if(phi_list.size())
         {
            for(const auto& stmt : phi_list)
            {
               parametersBinding(stmt, FD);
               if(isValidInstruction(stmt, FB, TM))
               {
                  addOperation(stmt, function_id, FB, TM);
               }
            }
         }

         const auto& stmt_list = idxBB.second->CGetStmtList();
         if(stmt_list.size())
         {
            for(const auto& stmt : stmt_list)
            {
               if(!isValidInstruction(stmt, FB, TM))
               {
                  parametersBinding(stmt, FD);
                  if(!storeFunctionCall(stmt))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Skipping " + GET_NODE(stmt)->get_kind_text() + " " + GET_NODE(stmt)->ToString());
                  }
                  continue;
               }
               addOperation(stmt, function_id, FB, TM);
               parametersBinding(stmt, FD);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Graph built for function " + fn_name);
      return false;
   }

   void buildVarNodes()
   {
      // Initializes the nodes and the use map structure.
      for(auto& pair : getVarNodes())
      {
         pair.second->init(!static_cast<bool>(getDefs().count(pair.first)));
      }
   }

   void findIntervals(
   #ifndef NDEBUG
      const ParameterConstRef parameters, const std::string& step_name
   #endif
      )
   {
      buildSymbolicIntersectMap();
      // List of SCCs
      Nuutila sccList(getVarNodes(), getUses(), symbMap);
      
      for(const auto& n : sccList)
      {
         const auto& component = sccList.getComponent(n);

         #ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Components:");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
            for(const auto* var : component)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, var->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-----------");
         }
         #endif
         if(component.size() == 1)
         {
            VarNode* var = *component.begin();
            solveFuturesSC(var);
            auto varDef = getDefs().find(var->getValue());
            if(varDef != getDefs().end())
            {
               auto* op = varDef->second;
               var->setRange(op->eval());
            }
            if(var->getRange()->isUnknown())
            {
               var->setRange(var->getMaxRange());
            }
         }
         else
         {
            UseMap compUseMap = buildUseMap(component);

            // Get the entry points of the SCC
            std::set<tree_nodeConstRef, tree_reindexCompare> entryPoints;

            #ifdef RA_JUMPSET
            // Create vector of constants inside component
            // Comment this line below to deactivate jump-set
            buildConstantVector(component, compUseMap);
            #endif
            #ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
            {
               std::stringstream ss;
               for(auto cnst : constantvector)
               {
                  ss << " " << cnst;
               }
               if(!constantvector.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, ss.str());
               }
            }
            #endif
            generateEntryPoints(component, entryPoints);
            // iterate a fixed number of time before widening
            update(static_cast<size_t>(component.size() * 16L), compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("cgfixed.dot", parameters));

            generateEntryPoints(component, entryPoints);
            #ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "entryPoints:");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
               for(const auto& el : entryPoints)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, GET_CONST_NODE(el)->ToString());
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
            }
            #endif
            // First iterate till fix point
            preUpdate(compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "fixIntersects");
            solveFutures(component);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, " --");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("cgfixintersect.dot", parameters));

            for(VarNode* varNode : component)
            {
               if(varNode->getRange()->isUnknown())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "initialize unknown: " + varNode->ToString());
                  //    THROW_UNREACHABLE("unexpected condition");
                  varNode->setRange(varNode->getMaxRange());
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("cgint.dot", parameters));

            // Second iterate till fix point
            std::set<tree_nodeConstRef, tree_reindexCompare> activeVars;
            generateActivesVars(component, activeVars);
            posUpdate(compUseMap, activeVars, component);
         }
         propagateToNextSCC(component);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed final constraint graph to " + printToFile("CG" + step_name + ".dot", parameters));
   }

   RangeConstRef getRange(const tree_nodeConstRef v)
   {
      auto vit = getVarNodes().find(v);
      if(vit == getVarNodes().end())
      {
         // If the value doesn't have a range,
         // it wasn't considered by the range analysis
         // for some reason.
         // It gets an unknown range if it's a variable,
         // or the tight range if it's a constant
         //
         // I decided NOT to insert these uncovered
         // values to the node set after their range
         // is created here.
         THROW_ASSERT(static_cast<bool>(getGIMPLE_BW(v)), "Invalid bitwidth");
         if(GetPointer<const cst_node>(GET_CONST_NODE(v)))
         {
            return getGIMPLE_range(v);
         }
         return getUnknownFor(v);
      }
      return vit->second->getRange();
   }

   std::string printToFile(const std::string& file_name, const ParameterConstRef parameters) const
   {
      std::string output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "RangeAnalysis/";
      if(!boost::filesystem::exists(output_directory))
      {
         boost::filesystem::create_directories(output_directory);
      }
      const std::string full_name = output_directory + file_name;
      std::ofstream file(full_name);
      print(file);
      return full_name;
   }

   /// Prints the content of the graph in dot format. For more information
   /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
   void print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      // Print the header of the .dot file.
      OS << "digraph dotgraph {\n";
      OS << R"(label="Constraint Graph for ')";
      OS << "all\' functions\";\n";
      OS << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

      // Print the body of the .dot file.
      for(const auto& [var, node] : getVarNodes())
      {
         if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(var)))
         {
            OS << " " << C->value;
         }
         else
         {
            OS << quot;
            printVarName(var, OS);
            OS << quot;
         }
         OS << R"( [label=")" << node << "\"]\n";
      }

      for(auto* op : getOpNodes())
      {
         op->print(OS);
         OS << '\n';
      }
      OS << pseudoEdgesString.str();
      // Print the footer of the .dot file.
      OS << "}\n";
   }
};

// ========================================================================== //
// Cousot
// ========================================================================== //
class Cousot : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) override
   {
      update(compUseMap, entryPoints, Meet::widen);
   }

   void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints, const CustomSet<VarNode*>& /*component*/) override
   {
      update(compUseMap, entryPoints, Meet::narrow);
   }

 public:
   Cousot(application_managerRef _AppM, int _debug_level, int _graph_debug) : ConstraintGraph(_AppM, _debug_level, _graph_debug) {}
};

// ========================================================================== //
// CropDFS
// ========================================================================== //
class CropDFS : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) override
   {
      update(compUseMap, entryPoints, [](OpNode* b, const std::vector<APInt>&) { return Meet::growth(b); });
   }

   void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& /*activeVars*/, const CustomSet<VarNode*>& component) override
   {
      storeAbstractStates(component);
      for(const auto& op : getOpNodes())
      {
         if(static_cast<bool>(component.count(op->getSink())))
         {
            crop(compUseMap, op);
         }
      }
   }

   void storeAbstractStates(const CustomSet<VarNode*>& component)
   {
      for(const auto& varNode : component)
      {
         varNode->storeAbstractState();
      }
   }

   void crop(const UseMap& compUseMap, OpNode* op)
   {
      CustomSet<OpNode*> activeOps;
      CustomSet<const VarNode*> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         auto* V = *activeOps.begin();
         activeOps.erase(V);
         const VarNode* sink = V->getSink();

         // if the sink has been visited go to the next activeOps
         if(static_cast<bool>(visitedOps.count(sink)))
         {
            continue;
         }

         Meet::crop(V);
         visitedOps.insert(sink);

         // The use list.of sink
         const auto& L = compUseMap.at(sink->getValue());
         for(auto* opr : L)
         {
            activeOps.insert(opr);
         }
      }
   }

 public:
   CropDFS(application_managerRef _AppM, int _debug_level, int _graph_debug) : ConstraintGraph(_AppM, _debug_level, _graph_debug) {}
};

static void MatchParametersAndReturnValues(unsigned int function_id, const application_managerRef AppM, const ConstraintGraphRef CG, int 
   #ifndef NDEBUG
   debug_level
   #endif
   )
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = TM->get_tree_node_const(function_id);
   const auto* FD = GetPointer<const function_decl>(fd);
   #if !defined(NDEBUG) or HAVE_ASSERTS
   std::string fn_name = tree_helper::print_type(TM, function_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper())));
   #endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "MatchParms&RetVal on function " + fn_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   if(!static_cast<bool>(CG->getCallMap()->count(function_id)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No call statements for this function, skipping...");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return;
   }
   const auto& functionCalls = CG->getCallMap()->at(function_id);

   // Data structure which contains the matches between formal and real
   // parameters
   // First: formal parameter
   // Second: real parameter
   std::vector<std::pair<tree_nodeConstRef, tree_nodeConstRef>> parameters(FD->list_of_args.size());

   // Fetch the function arguments (formal parameters) into the data structure
   const auto& parmMap = CG->getParmMap();
   const auto funParm = parmMap.find(function_id);
   THROW_ASSERT(funParm != parmMap.end(), "Function parameters binding unavailable");
   const auto& parmBind = funParm->second.second;
   THROW_ASSERT(parmBind.size() == parameters.size(), "Parameters count mismatch");
   for(size_t i = 0; i < parameters.size(); ++i)
   {
      if(const auto& p = parmBind.at(i))
      {
         const auto pType = getGIMPLE_Type(p);
         if(!isValidType(pType))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " is of non-valid type (" + pType->get_kind_text() + ")");
            continue;
         }
         parameters[i].first = p;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " defined as " + GET_CONST_NODE(p)->ToString());
      }
      else
      {
         parameters[i].first = nullptr;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " missing from function body");
      }
   }

   // Check if the function returns a supported value type. If not, no return
   // value matching is done
   const auto ret_type = tree_helper::GetFunctionReturnType(fd);
   bool noReturn = ret_type == nullptr || ret_type->get_kind() == void_type_K;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function has " + (noReturn ? "no return type" : ("return type " + ret_type->get_kind_text())));

   // Creates the data structure which receives the return values of the
   // function, if there is any
   std::set<tree_nodeConstRef, tree_reindexCompare> returnValues;
   if(!noReturn)
   {
      const auto* SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& stmt_list = idxBB.second->CGetStmtList();
            
         if(stmt_list.size())
         if(const auto* gr = GetPointer<const gimple_return>(GET_CONST_NODE(stmt_list.back())))
         if(gr->op != nullptr)   // Compiler defined return statements may be without argument
         {
            returnValues.insert(gr->op);
         }
      }
   }
   if(returnValues.empty() && !noReturn)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function should return, but no return statement was found");
      noReturn = true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, std::string("Function ") + (noReturn ? "has no" : "has explicit") + " return statement" + (returnValues.size() > 1 ? "s" : ""));

   /// Generate PhiOp nodes for parameters call values
   std::vector<PhiOpNode*> matchers(parameters.size(), nullptr);
   for(size_t i = 0, e = parameters.size(); i < e; ++i)
   {
      if(parameters[i].first == nullptr)
      {
         continue;
      }
      VarNode* sink = CG->addVarNode(parameters[i].first, function_id);
      sink->setRange(sink->getMaxRange());
      matchers[i] = new PhiOpNode(ValueRangeRef(new ValueRange(sink->getRange())), sink, nullptr);
   }

   std::vector<VarNode*> returnVars;
   for(auto returnValue : returnValues)
   {
      VarNode* from = CG->addVarNode(returnValue, function_id);
      returnVars.push_back(from);
   }

   for(const auto& call : functionCalls)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing call " + GET_CONST_NODE(call)->ToString());
      const std::vector<tree_nodeRef>* args = nullptr;
      tree_nodeConstRef ret_var = nullptr;
      if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(call)))
      {
         THROW_ASSERT(!noReturn, "Function called from gimple_assign should have a return statement");
         const auto* ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
         args = &ce->args;
         ret_var = ga->op0;
      }
      else if(const auto* gc = GetPointer<const gimple_call>(GET_CONST_NODE(call)))
      {
         args = &gc->args;
      }
      else
      {
         THROW_UNREACHABLE("Call statement should be a gimple_assign or a gimple_call");
      }
         
      THROW_ASSERT(args->size() == parameters.size(), "Function parameters and call arguments size mismatch");
      for(size_t i = 0; i < parameters.size(); ++i)
      {
         parameters[i].second = args->at(i);
      }

      // Do the inter-procedural construction of CG
      VarNode* to = nullptr;
      VarNode* from = nullptr;

      // Match formal and real parameters
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(size_t i = 0; i < parameters.size(); ++i)
      {
         if(parameters[i].first == nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " was constant, matching not necessary");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_CONST_NODE(parameters[i].second)->ToString() + " bound to argument " + GET_CONST_NODE(parameters[i].first)->ToString());
         // Add real parameter to the CG
         from = CG->addVarNode(parameters[i].second, function_id);

         // Connect nodes
         matchers[i]->addSource(from);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      // Match return values when return type is stored from caller
      if(!noReturn && GET_CONST_NODE(call)->get_kind() != gimple_call_K)
      {
         // Add caller instruction to the CG (it receives the return value)
         to = CG->addVarNode(ret_var, function_id);
         to->setRange(to->getMaxRange());

         auto* phiOp = new PhiOpNode(ValueRangeRef(new ValueRange(to->getRange())), to, nullptr);
         for(VarNode* var : returnVars)
         {
            phiOp->addSource(var);
         }
         CG->pushOperation(phiOp);
      
      #ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
         {
            std::string phiString = "Return variable " + GET_CONST_NODE(phiOp->getSink()->getValue())->ToString() + " = PHI<";
            for(size_t i = 0; i < phiOp->getNumSources(); ++i)
            {
               phiString += GET_CONST_NODE(phiOp->getSource(i)->getValue())->ToString() + ", ";
            }
            phiString[phiString.size() - 2] = '>';
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
         }
      #endif
      }

      // Real parameters are cleaned before moving to the next use (for safety's
      // sake)
      for(auto& pair : parameters)
      {
         pair.second = nullptr;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   for(auto* m : matchers)
   {
      if(m == nullptr)
      {
         continue;
      }
      CG->pushOperation(m);
      #ifndef NDEBUG
      if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
      {
         std::string phiString = GET_CONST_NODE(m->getSink()->getValue())->ToString() + " = PHI<";
         for(size_t i = 0; i < m->getNumSources(); ++i)
         {
            phiString += GET_CONST_NODE(m->getSource(i)->getValue())->ToString() + ", ";
         }
         phiString[phiString.size() - 2] = '>';
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
      }
      #endif
   }
}

// ========================================================================== //
// RangeAnalysis
// ========================================================================== //
RangeAnalysis::RangeAnalysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par)
   : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, dfm, par), solverType(st_Cousot), dead_code_restart(false)
   #ifndef NDEBUG
   , graph_debug(DEBUG_LEVEL_NONE), iteration(0), debug_mode(RA_DEBUG_NONE)
   #endif
   , requireESSA(false) // ESSA disabled because of renaming issues in some cases
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   const auto opts = SplitString(parameters->getOption<std::string>(OPT_range_analysis_mode), ",");
   CustomSet<std::string> ra_mode;
   for(const auto& opt : opts)
   {
      ra_mode.insert(opt);
   }
   #ifndef NDEBUG
   if(ra_mode.contains("ro"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: read-only mode enabled");
      debug_mode = RA_DEBUG_READONLY;
   }
   if(ra_mode.contains("skip"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: skip mode enabled");
      debug_mode = RA_DEBUG_NOEXEC;
   }
   if(ra_mode.contains("debug_op"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: range operations debug");
      OpNode::debug_level = debug_level;
   }
   if(ra_mode.contains("debug_graph"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: graph debug");
      graph_debug = debug_level;
      Meet::debug_level = debug_level;
   }
   #endif
   if(ra_mode.contains("crop"))
   {
      solverType = st_Crop;
   }
   if(ra_mode.contains("noESSA"))
   {
      requireESSA = false;
   }
   OPERATION_OPTION(ra_mode, add);
   OPERATION_OPTION(ra_mode, sub);
   OPERATION_OPTION(ra_mode, mul);
   OPERATION_OPTION(ra_mode, sdiv);
   OPERATION_OPTION(ra_mode, udiv);
   OPERATION_OPTION(ra_mode, srem);
   OPERATION_OPTION(ra_mode, urem);
   OPERATION_OPTION(ra_mode, shl);
   OPERATION_OPTION(ra_mode, shr);
   OPERATION_OPTION(ra_mode, abs);
   OPERATION_OPTION(ra_mode, negate);
   OPERATION_OPTION(ra_mode, not);
   OPERATION_OPTION(ra_mode, and);
   OPERATION_OPTION(ra_mode, or);
   OPERATION_OPTION(ra_mode, xor);
   OPERATION_OPTION(ra_mode, sext);
   OPERATION_OPTION(ra_mode, zext);
   OPERATION_OPTION(ra_mode, trunc);
   OPERATION_OPTION(ra_mode, min);
   OPERATION_OPTION(ra_mode, max);
}

RangeAnalysis::~RangeAnalysis() = default;

void RangeAnalysis::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range Analysis step");
   dead_code_restart = false;
   switch(solverType)
   {
      case st_Cousot:
         CG.reset(new Cousot(AppM,
            #ifndef NDEBUG
               debug_level, graph_debug));
            #else
               DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
            #endif
         break;
      case st_Crop:
         CG.reset(new CropDFS(AppM,
            #ifndef NDEBUG
               debug_level, graph_debug));
            #else
               DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
            #endif
         break;
      default:
         THROW_UNREACHABLE("Unknown solver type " + STR(solverType));
         break;
   }
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> 
RangeAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(BIT_VALUE, ALL_FUNCTIONS));
         if(requireESSA)
         {
            relationships.insert(std::make_pair(ESSA, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         //    if(dead_code_restart)
         //    {
         //       relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, ALL_FUNCTIONS));   // TODO: could it be more specific?
         //    }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool RangeAnalysis::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status RangeAnalysis::Exec()
{
   #ifndef NDEBUG
   if(debug_mode == RA_DEBUG_NOEXEC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Range analysis no execution mode enabled");
      return DesignFlowStep_Status::UNCHANGED;
   }
   #endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Analyse only reached functions
   const auto TM = AppM->get_tree_manager();
   auto functions = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();

   #ifdef EARLY_DEAD_CODE_RESTART
   std::vector<unsigned int> dead_code_reboot;
   for(const auto f : functions)
   {
      bool dead_code_necessary = CG->buildGraph(f);
      if(dead_code_necessary)
      {
         dead_code_reboot.push_back(f);
      }
   }
   if(!dead_code_reboot.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Following functions have unpropagated constants:");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto f_id : dead_code_reboot)
      {
         const auto FB = AppM->GetFunctionBehavior(f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, tree_helper::print_type(TM, f_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(FB->CGetBehavioralHelper()))));
         FB->UpdateBBVersion();
         FB->UpdateBitValueVersion();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      dead_code_restart = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unpropagated constants detected, aborting...");
      return DesignFlowStep_Status::ABORTED;
   }
   #else
   for(const auto f : functions)
   {
      CG->buildGraph(f);
   }
   #endif

   // Top functions are not called by any other functions, so they do not have any call statement to analyse
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "MatchParms&RetVal analysis...");
   for(const auto top_fn : AppM->CGetCallGraphManager()->GetRootFunctions())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         tree_helper::print_type(TM, top_fn, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(top_fn)->CGetBehavioralHelper()))) + " is top function");
      functions.erase(top_fn);
   }
   // The two operations are split because the CallMap is built for all functions in buildGraph
   // then it is used from MatchParametersAndReturnValues
   for(const auto f : functions)
   {
      MatchParametersAndReturnValues(f, AppM, CG, debug_level);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "MatchParms&RetVal analysis completed");
   CG->buildVarNodes();

   #ifndef NDEBUG
   CG->findIntervals(parameters, GetName() + "(" + STR(iteration++) + ")");
   #else
   CG->findIntervals();
   #endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(finalize())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges updated");
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges reached fixed point");
      return DesignFlowStep_Status::UNCHANGED;
   }
}

bool RangeAnalysis::finalize()
{
   const auto& vars = std::static_pointer_cast<const ConstraintGraph>(CG)->getVarNodes();
   #ifndef NDEBUG
   if(debug_mode >= RA_DEBUG_READONLY)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& [var, node] : vars)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range " + node->getRange()->ToString() + " for " + GET_CONST_NODE(var)->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "IR update not applied in read-only mode");
      CG.reset();
      return false;
   }
   #endif

   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   CustomSet<unsigned int> updatedFunctions;

   #ifndef NDEBUG
   unsigned long long updated = 0;
               #endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto& varNode : vars)
   {
      if(varNode.second->updateIR(TM, tree_man
            #ifndef NDEBUG
         , debug_level
            #endif
         ))
            {
         updatedFunctions.insert(varNode.second->getFunctionId());
         #ifndef NDEBUG
         ++updated;
         AppM->RegisterTransformation(GetName(), nullptr);
         #endif
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds updated for " + STR(updated) + "/" + STR(vars.size()) + " variables");
   CG.reset();

   for(const auto fun_id : updatedFunctions)
   {
      const auto FB = AppM->GetFunctionBehavior(fun_id);
      FB->UpdateBBVersion();
      FB->UpdateBitValueVersion();
   }
   return !updatedFunctions.empty();
}
