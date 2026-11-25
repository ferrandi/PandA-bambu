/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file bounded_decimal.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef PANDA_BOUNDED_DECIMAL_HPP
#define PANDA_BOUNDED_DECIMAL_HPP

#include <cstdint>
#include <string>

namespace panda
{
   namespace utility
   {
      enum class bounded_decimal_result
      {
         valid,
         invalid_format,
         out_of_range
      };

      inline bounded_decimal_result parse_bounded_ascii_decimal(const std::string& text, const std::uint32_t minimum,
                                                                const std::uint32_t maximum, std::uint32_t& result)
      {
         if(text.empty() || minimum > maximum)
         {
            return text.empty() ? bounded_decimal_result::invalid_format : bounded_decimal_result::out_of_range;
         }

         std::uint32_t value = 0;
         bool exceeds_maximum = false;
         for(const char raw_ch : text)
         {
            const auto ch = static_cast<unsigned char>(raw_ch);
            if(ch < '0' || ch > '9')
            {
               return bounded_decimal_result::invalid_format;
            }
            if(!exceeds_maximum)
            {
               const auto digit = static_cast<std::uint32_t>(ch - '0');
               if(value > maximum / 10U || (value == maximum / 10U && digit > maximum % 10U))
               {
                  exceeds_maximum = true;
               }
               else
               {
                  value = value * 10U + digit;
               }
            }
         }
         if(exceeds_maximum || value < minimum)
         {
            return bounded_decimal_result::out_of_range;
         }
         result = value;
         return bounded_decimal_result::valid;
      }
   } // namespace utility
} // namespace panda

#endif // PANDA_BOUNDED_DECIMAL_HPP
