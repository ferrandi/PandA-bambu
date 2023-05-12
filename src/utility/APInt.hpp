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
 * @file APInt.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef APINT_HPP
#define APINT_HPP

#include <iostream>
#include <string>
#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/literals.hpp>

class APInt
{
 public:
   using backend = boost::multiprecision::backends::cpp_int_backend<4096, 4096, boost::multiprecision::signed_magnitude,
                                                                    boost::multiprecision::unchecked, void>;
   using number = boost::multiprecision::number<backend>;
   using bw_t = uint16_t;

 private:
   number _data;

 public:
   APInt();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
   template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   APInt(T val) : _data(val)
   {
   }

   APInt(const number& v) : _data(v)
   {
   }

   APInt(const std::string& str) : _data(boost::lexical_cast<number>(str))
   {
   }
#pragma GCC diagnostic pop

   friend bool operator<(const APInt& lhs, const APInt& rhs);
   friend bool operator>(const APInt& lhs, const APInt& rhs);
   friend bool operator<=(const APInt& lhs, const APInt& rhs);
   friend bool operator>=(const APInt& lhs, const APInt& rhs);
   friend bool operator==(const APInt& lhs, const APInt& rhs);
   friend bool operator!=(const APInt& lhs, const APInt& rhs);
   explicit operator bool() const;

   /*
    * Binary operators
    */
   friend APInt operator+(const APInt& lhs, const APInt& rhs);
   friend APInt operator-(const APInt& lhs, const APInt& rhs);
   friend APInt operator*(const APInt& lhs, const APInt& rhs);
   friend APInt operator/(const APInt& lhs, const APInt& rhs);
   friend APInt operator%(const APInt& lhs, const APInt& rhs);
   friend APInt operator&(const APInt& lhs, const APInt& rhs);
   friend APInt operator|(const APInt& lhs, const APInt& rhs);
   friend APInt operator^(const APInt& lhs, const APInt& rhs);
   friend APInt operator<<(const APInt& lhs, const APInt& rhs);
   friend APInt operator>>(const APInt& lhs, const APInt& rhs);
   APInt& operator+=(const APInt& rhs);
   APInt& operator-=(const APInt& rhs);
   APInt& operator*=(const APInt& rhs);
   APInt& operator/=(const APInt& rhs);
   APInt& operator%=(const APInt& rhs);
   APInt& operator&=(const APInt& rhs);
   APInt& operator|=(const APInt& rhs);
   APInt& operator^=(const APInt& rhs);
   APInt& operator<<=(const APInt& rhs);
   APInt& operator>>=(const APInt& rhs);

   /*
    * Unary operators
    */
   APInt abs() const;
   APInt operator-() const;
   APInt operator~() const;
   APInt operator++(int);
   APInt operator--(int);
   APInt& operator++();
   APInt& operator--();

   /*
    * Bitwise helpers
    */
   void bit_set(bw_t i);
   void bit_clr(bw_t i);
   bool bit_tst(bw_t i) const;
   bool sign() const;
   APInt& extOrTrunc(bw_t bw, bool sign);
   APInt extOrTrunc(bw_t bw, bool sign) const;
   bw_t trailingZeros(bw_t bw) const;
   bw_t trailingOnes(bw_t bw) const;
   bw_t leadingZeros(bw_t bw) const;
   bw_t leadingOnes(bw_t bw) const;
   bw_t minBitwidth(bool sign) const;

   template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
   explicit operator T() const
   {
      using U = typename std::make_unsigned<T>::type;
      return static_cast<T>(static_cast<U>(_data & std::numeric_limits<U>::max()));
   }

   static APInt getMaxValue(bw_t bw);
   static APInt getMinValue(bw_t bw);
   static APInt getSignedMaxValue(bw_t bw);
   static APInt getSignedMinValue(bw_t bw);

   friend std::ostream& operator<<(std::ostream& str, const APInt& v);
   friend std::istream& operator>>(std::istream& str, APInt& v);
};

std::ostream& operator<<(std::ostream& str, const APInt& v);
std::istream& operator>>(std::istream& str, APInt& v);

template <char... STR>
constexpr APInt::number operator"" _apint()
{
   typedef typename boost::multiprecision::literals::detail::make_packed_value_from_str<STR...>::type pt;
   return boost::multiprecision::literals::detail::make_backend_from_pack<pt, APInt::backend>::value;
}

#endif
