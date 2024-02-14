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
 * @file bit_lattice.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "bit_lattice.hpp"

#include "exceptions.hpp"

std::deque<bit_lattice> create_u_bitstring(size_t lenght)
{
   return std::deque<bit_lattice>(lenght, bit_lattice::U);
}

std::deque<bit_lattice> create_x_bitstring(size_t lenght)
{
   return std::deque<bit_lattice>(lenght, bit_lattice::X);
}

std::deque<bit_lattice> create_bitstring_from_constant(integer_cst_t value, unsigned long long len, bool signed_value)
{
   std::deque<bit_lattice> res;
   if(value == 0)
   {
      res.push_front(bit_lattice::ZERO);
      return res;
   }
   unsigned bit;
   for(bit = 0; bit < len && value != 0; bit++, value >>= 1)
   {
      res.push_front((value & 1) ? bit_lattice::ONE : bit_lattice::ZERO);
   }
   if(bit < len && signed_value)
   {
      res.push_front(bit_lattice::ZERO);
   }
   return res;
}

std::string bitstring_to_string(const std::deque<bit_lattice>& bitstring)
{
   std::string res;
   for(const auto bit : bitstring)
   {
      switch(bit)
      {
         case(bit_lattice::U):
            res.push_back('U');
            break;
         case(bit_lattice::X):
            res.push_back('X');
            break;
         case(bit_lattice::ZERO):
            res.push_back('0');
            break;
         case(bit_lattice::ONE):
            res.push_back('1');
            break;
         default:
            THROW_ERROR("unexpected value in bit_lattice");
            break;
      }
   }
   return res;
}

std::deque<bit_lattice> string_to_bitstring(const std::string& s)
{
   std::deque<bit_lattice> res;
   for(const auto bit : s)
   {
      switch(bit)
      {
         case('U'):
            res.push_back(bit_lattice::U);
            break;
         case('X'):
            res.push_back(bit_lattice::X);
            break;
         case('0'):
            res.push_back(bit_lattice::ZERO);
            break;
         case('1'):
            res.push_back(bit_lattice::ONE);
            break;
         default:
            THROW_ERROR(std::string("unexpected char in bitvalue string: ") + bit);
            break;
      }
   }
   return res;
}

bool bitstring_constant(const std::deque<bit_lattice>& a)
{
   bool res = true;
   for(const auto i : a)
   {
      if(i == bit_lattice::U || i == bit_lattice::X)
      {
         res = false;
         break;
      }
   }
   return res;
}

bit_lattice bit_sup(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::X || b == bit_lattice::X)
   {
      return bit_lattice::X;
   }
   else if(a == bit_lattice::U)
   {
      return b;
   }
   else if(b == bit_lattice::U)
   {
      return a;
   }
   else
   {
      return bit_lattice::X;
   }
}

std::deque<bit_lattice> sup(const std::deque<bit_lattice>& _a, const std::deque<bit_lattice>& _b,
                            const size_t out_type_size, const bool out_is_signed, const bool out_is_bool)
{
   THROW_ASSERT(!_a.empty() && !_b.empty(), "a = " + std::string(_a.empty() ? "empty" : bitstring_to_string(_a)) +
                                                ", b = " + (_b.empty() ? "empty" : bitstring_to_string(_b)));
   THROW_ASSERT(out_type_size, "Size can not be zero");
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   std::deque<bit_lattice> res;
   if(out_is_bool)
   {
      res.push_back(bit_sup(_a.back(), _b.back()));
      return res;
   }

   std::deque<bit_lattice> longer = (_a.size() >= _b.size()) ? _a : _b;
   std::deque<bit_lattice> shorter = (_a.size() >= _b.size()) ? _b : _a;
   while(longer.size() > out_type_size)
   {
      longer.pop_front();
   }
   while(shorter.size() > out_type_size)
   {
      shorter.pop_front();
   }
   if(longer.size() < out_type_size)
   {
      longer = sign_extend_bitstring(longer, out_is_signed, out_type_size);
   }
   if(shorter.size() < out_type_size)
   {
      shorter = sign_extend_bitstring(shorter, out_is_signed, out_type_size);
   }
   //    // BEWARE: zero is stored as single bit, but it has to be considered as a full string
   //    //         of zeros to propagate values correctly
   //    if(shorter.size() == 1 && shorter.front() == bit_lattice::ZERO)
   //    {
   //       auto l_bw = longer.size();
   //       for(;l_bw > 1; --l_bw)
   //             shorter.push_front(bit_lattice::ZERO);
   //    }
   //    if(!out_is_signed && longer.size() > shorter.size() && longer.front() == bit_lattice::ZERO)
   //    {
   //       shorter = sign_extend_bitstring(shorter, out_is_signed, longer.size());
   //    }
   //    else
   //    {
   //       while(longer.size() > shorter.size())
   //       {
   //          if(out_is_signed)
   //          {
   //             shorter = sign_extend_bitstring(shorter, out_is_signed, longer.size());
   //          }
   //          else
   //          {
   //             longer.pop_front();
   //          }
   //       }
   //    }

   auto a_it = longer.crbegin();
   auto b_it = shorter.crbegin();
   const auto a_end = longer.crend();
   const auto b_end = shorter.crend();
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
   {
      res.push_front(bit_sup(*a_it, *b_it));
   }

   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   sign_reduce_bitstring(res, out_is_signed);
   size_t final_size;
   const bool a_sign_is_x = _a.front() == bit_lattice::X;
   const bool b_sign_is_x = _b.front() == bit_lattice::X;
   auto sign_bit = res.front();
   if(out_is_signed)
   {
      final_size =
          std::min(out_type_size,
                   ((a_sign_is_x == b_sign_is_x ||
                     ((_a.front() == bit_lattice::ZERO || _a.front() == bit_lattice::ONE) && _a.size() < _b.size()) ||
                     ((_b.front() == bit_lattice::ZERO || _b.front() == bit_lattice::ONE) && _a.size() > _b.size())) ?
                        (std::min(_a.size(), _b.size())) :
                        (a_sign_is_x ? _a.size() : _b.size())));
      THROW_ASSERT(final_size, "final size of sup cannot be 0");
   }
   else
   {
      final_size = std::min(out_type_size, (a_sign_is_x == b_sign_is_x) ?
                                               std::min(_a.size(), _b.size()) :
                                               (a_sign_is_x ? (_a.size() < _b.size() ? _a.size() : 1 + _b.size()) :
                                                              (_a.size() < _b.size() ? 1 + _a.size() : _b.size())));
      sign_bit = (a_sign_is_x == b_sign_is_x) ? sign_bit :
                                                (a_sign_is_x ? (_a.size() < _b.size() ? sign_bit : bit_lattice::ZERO) :
                                                               (_a.size() < _b.size() ? bit_lattice::ZERO : sign_bit));
      THROW_ASSERT(final_size, "final size of sup cannot be 0");
   }
   while(res.size() > final_size)
   {
      res.pop_front();
   }
   if(sign_bit != bit_lattice::X && res.front() == bit_lattice::X)
   {
      res.pop_front();
      res.push_front(sign_bit);
   }
   if(!out_is_signed)
   {
      sign_reduce_bitstring(res, out_is_signed);
   }

   return res;
}

bit_lattice bit_inf(const bit_lattice a, const bit_lattice b)
{
   if(a == b)
   {
      return a;
   }
   else if(a == bit_lattice::U || b == bit_lattice::U)
   {
      return bit_lattice::U;
   }
   else if(a == bit_lattice::X)
   {
      return b;
   }
   else if(b == bit_lattice::X)
   {
      return a;
   }
   else
   {
      return bit_lattice::U;
   }
}

std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                            const size_t out_type_size, const bool out_is_signed, const bool out_is_bool)
{
   THROW_ASSERT(!(a.empty() && b.empty()),
                "a.size() = " + std::to_string(a.size()) + " b.size() = " + std::to_string(b.size()));
   THROW_ASSERT(out_type_size, "");
   THROW_ASSERT(!out_is_bool || (out_type_size == 1), "boolean with type size != 1");
   std::deque<bit_lattice> res;
   if(out_is_bool)
   {
      res.push_back(bit_inf(a.back(), b.back()));
      return res;
   }

   std::deque<bit_lattice> a_copy = a;
   std::deque<bit_lattice> b_copy = b;
   sign_reduce_bitstring(a_copy, out_is_signed);
   sign_reduce_bitstring(b_copy, out_is_signed);

   // a_tmp is the longer bistring
   std::deque<bit_lattice> a_tmp = (a_copy.size() >= b_copy.size()) ? a_copy : b_copy;
   std::deque<bit_lattice> b_tmp = (a_copy.size() >= b_copy.size()) ? b_copy : a_copy;

   if(a_tmp.size() > b_tmp.size())
   {
      b_tmp = sign_extend_bitstring(b_tmp, out_is_signed, a_tmp.size());
   }

   auto a_it = a_tmp.crbegin();
   auto b_it = b_tmp.crbegin();
   const auto a_end = a_tmp.crend();
   const auto b_end = b_tmp.crend();
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
   {
      res.push_front(bit_inf(*a_it, *b_it));
   }

   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }

   sign_reduce_bitstring(res, out_is_signed);

   while(res.size() > out_type_size)
   {
      res.pop_front();
   }
   return res;
}

/// function slightly different than tree_helper.cpp: sign_reduce_bitstring
void sign_reduce_bitstring(std::deque<bit_lattice>& bitstring, bool bitstring_is_signed)
{
   THROW_ASSERT(!bitstring.empty(), "");
   while(bitstring.size() > 1)
   {
      if(bitstring_is_signed)
      {
         if(bitstring.at(0) != bit_lattice::U && bitstring.at(0) == bitstring.at(1))
         {
            bitstring.pop_front();
         }
         else
         {
            break;
         }
      }
      else
      {
         if((bitstring.at(0) == bit_lattice::X && bitstring.at(1) == bit_lattice::X) ||
            (bitstring.at(0) == bit_lattice::ZERO && bitstring.at(1) != bit_lattice::X))
         {
            bitstring.pop_front();
         }
         else if(bitstring.at(0) == bit_lattice::ZERO && bitstring.at(1) == bit_lattice::X)
         {
            bitstring.pop_front();
            bitstring.pop_front();
            bitstring.push_front(bit_lattice::ZERO);
         }
         else
         {
            break;
         }
      }
   }
}

std::deque<bit_lattice> sign_extend_bitstring(const std::deque<bit_lattice>& bitstring, bool bitstring_is_signed,
                                              size_t final_size)
{
   THROW_ASSERT(final_size, "cannot sign extend a bitstring to final_size 0");
   THROW_ASSERT(final_size > bitstring.size(), "useless sign extension");
   std::deque<bit_lattice> res = bitstring;
   if(res.empty())
   {
      res.push_front(bit_lattice::X);
   }
   const auto sign_bit = (bitstring_is_signed || res.front() == bit_lattice::X) ? res.front() : bit_lattice::ZERO;
   while(res.size() < final_size)
   {
      res.push_front(sign_bit);
   }
   return res;
}

bool isBetter(const std::string& a_string, const std::string& b_string)
{
   auto av = string_to_bitstring(a_string);
   auto bv = string_to_bitstring(b_string);
   auto a_it = av.crbegin();
   auto b_it = bv.crbegin();
   const auto a_end = av.crend();
   const auto b_end = bv.crend();
   for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
   {
      if((*b_it == bit_lattice::U && *a_it != bit_lattice::U) || (*b_it != bit_lattice::X && *a_it == bit_lattice::X))
      {
         return true;
      }
   }
   return a_string.size() < b_string.size();
}
