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
 *              Copyright (C) 2020-2023 Politecnico di Milano
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
 * @file APInt.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "APInt.hpp"

#include "exceptions.hpp"

using namespace boost::multiprecision;

using bw_t = APInt::bw_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
APInt::APInt() : _data(57)
{
}
#pragma GCC diagnostic pop

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
   _data <<= static_cast<bw_t>(rhs._data);
   return *this;
}

APInt& APInt::operator>>=(const APInt& rhs)
{
   _data >>= static_cast<bw_t>(rhs._data);
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
   _data |= 0x1_apint << i;
}

void APInt::bit_clr(bw_t i)
{
   _data &= ~(0x1_apint << i);
}

bool APInt::bit_tst(bw_t i) const
{
   return ((_data >> i) & 1) != 0;
}

bool APInt::sign() const
{
   return _data < 0;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

APInt& APInt::extOrTrunc(bw_t bw, bool sign)
{
   THROW_ASSERT(bw, "Minimum bitwidth of 1 is required");
   const number mask = (0x1_apint << bw) - 1;
   _data &= mask;
   if(sign && bit_tst(bw - 1U))
   {
      _data += (-0x1_apint << bw);
   }
   return *this;
}

APInt APInt::extOrTrunc(bw_t bw, bool sign) const
{
   THROW_ASSERT(bw, "Minimum bitwidth of 1 is required");
   const number mask = (0x1_apint << bw) - 1;
   APInt val(_data & mask);
   if(sign && val.bit_tst(bw - 1U))
   {
      val += (-0x1_apint << bw);
   }
   return APInt(val);
}

bw_t APInt::trailingZeros(bw_t bw) const
{
   bw_t i = 0;
   for(; i < bw; ++i)
   {
      if(bit_tst(i))
      {
         break;
      }
   }
   return i;
}

bw_t APInt::trailingOnes(bw_t bw) const
{
   bw_t i = 0;
   for(; i < bw; ++i)
   {
      if(!bit_tst(i))
      {
         break;
      }
   }
   return i;
}

bw_t APInt::leadingZeros(bw_t bw) const
{
   if(_data < 0)
   {
      return 0;
   }
   if(_data == 0)
   {
      return bw;
   }
   THROW_ASSERT(_data.backend().size() > 0, "unexpected condition");
   const auto limbs = _data.backend().limbs();
   auto nchunks = bw / backend::limb_bits + ((bw % backend::limb_bits) ? 1 : 0);
   THROW_ASSERT(_data.backend().size() <= nchunks, "unexpected condition");
   bw_t lzc = 0;
   bw_t offset = 0;
   if(_data.backend().size() < nchunks)
   {
      lzc += bw - _data.backend().size() * backend::limb_bits;
   }
   else
   {
      offset += (bw % backend::limb_bits) ? backend::limb_bits - (bw % backend::limb_bits) : 0;
   }
   for(int i = _data.backend().size() - 1; i >= 0; --i)
   {
      const auto& val = limbs[i];
      if(val != 0)
      {
         if(backend::limb_bits == 64)
         {
            return lzc + __builtin_clzll(val) - offset;
         }
         else if(backend::limb_bits == 32)
         {
            return lzc + __builtin_clz(val) - offset;
         }
         else
         {
            THROW_ERROR("backend::limb_bits size not supported");
         }
      }
      lzc += backend::limb_bits;
   }
   return lzc;
}

bw_t APInt::leadingOnes(bw_t bw) const
{
   bw_t i = bw;
   for(; i > 0; --i)
   {
      if(!bit_tst(i - 1U))
      {
         break;
      }
   }
   return bw - i;
}

APInt::bw_t APInt::minBitwidth(bool sign) const
{
   if(_data.backend().isneg())
   {
      if(!sign)
      {
         return std::numeric_limits<number>::digits;
      }
      return std::numeric_limits<number>::digits + 1 -
             APInt(-_data - 1).leadingZeros(std::numeric_limits<number>::digits);
   }
   else if(_data.is_zero())
   {
      return 1U;
   }
   return std::numeric_limits<number>::digits - leadingZeros(std::numeric_limits<number>::digits) + sign;
}

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

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
   str << v._data;
   return str;
}

std::istream& operator>>(std::istream& str, APInt& v)
{
   str >> v._data;
   return str;
}
