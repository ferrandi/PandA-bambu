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
 * @file APInt.cpp
 * @brief
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "APInt.hpp"

#include "exceptions.hpp"

using namespace boost::multiprecision;

using bw_t = APInt::bw_t;

APInt::APInt() : _data(57)
{
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
   const APInt_internal mask((APInt_internal(1, nullptr) << bw) - APInt_internal(1, nullptr), nullptr);
   _data &= mask;
   if(sign && bit_test(_data, static_cast<unsigned>(bw - 1)))
   {
      _data += (APInt_internal(-1, nullptr) << bw);
   }
   return *this;
}

APInt APInt::extOrTrunc(bw_t bw, bool sign) const
{
   THROW_ASSERT(bw, "Minimum bitwidth of 1 is required");
   const APInt_internal mask((APInt_internal(1, nullptr) << bw) - APInt_internal(1, nullptr), nullptr);
   APInt_internal val = _data & mask;
   if(sign && bit_test(val, static_cast<unsigned>(bw - 1)))
   {
      val += (APInt_internal(-1, nullptr) << bw);
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