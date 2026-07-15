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
 *   Copyright (C) 2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file kcm_constmul.hpp
 * @brief Helpers for KCM (LUT-based) constant multiplication.
 */
/**
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef KCM_CONSTMUL_HPP
#define KCM_CONSTMUL_HPP

#include <cstdint>

namespace kcm_constmul
{
   inline uint64_t maskForWidth(unsigned width)
   {
      if(width >= 64)
      {
         return ~0ULL;
      }
      if(width == 0)
      {
         return 0ULL;
      }
      return (1ULL << width) - 1ULL;
   }

   inline uint64_t truncUnsigned(uint64_t value, unsigned width)
   {
      return value & maskForWidth(width);
   }

   inline int64_t truncSigned(int64_t value, unsigned width)
   {
      if(width >= 64)
      {
         return value;
      }
      if(width == 0)
      {
         return 0;
      }
      const uint64_t mask = maskForWidth(width);
      const uint64_t sign = 1ULL << (width - 1);
      const uint64_t v = static_cast<uint64_t>(value) & mask;
      if((v & sign) != 0)
      {
         return static_cast<int64_t>(v | (~mask));
      }
      return static_cast<int64_t>(v);
   }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
   inline uint64_t buildKcmLutConstant(uint64_t coeff, unsigned alpha, unsigned bit_index, unsigned width)
   {
      if(alpha == 0 || width == 0)
      {
         return 0;
      }
      uint64_t lut_const = 0;
      const unsigned entries = 1U << alpha;
      const unsigned __int128 mask =
          width >= 64 ? static_cast<unsigned __int128>(~0ULL) : ((static_cast<unsigned __int128>(1) << width) - 1U);

      for(unsigned v = 0; v < entries; ++v)
      {
         const unsigned __int128 prod = static_cast<unsigned __int128>(coeff) * static_cast<unsigned __int128>(v);
         const unsigned __int128 masked = prod & mask;
         if(((masked >> bit_index) & 1U) != 0)
         {
            lut_const |= (1ULL << v);
         }
      }
      return lut_const;
   }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

   inline uint64_t kcmMulUnsigned(uint64_t x, uint64_t coeff, unsigned width, unsigned alpha)
   {
      if(alpha == 0 || width == 0)
      {
         return 0;
      }
      const uint64_t mask = maskForWidth(width);
      const uint64_t ux = truncUnsigned(x, width);
      const uint64_t uc = truncUnsigned(coeff, width);
      const unsigned q = (width + alpha - 1) / alpha;
      uint64_t result = 0;
      const uint64_t chunk_mask = (alpha >= 64) ? ~0ULL : ((1ULL << alpha) - 1ULL);

      for(unsigned chunk = 0; chunk < q; ++chunk)
      {
         const unsigned base_bit = chunk * alpha;
         const uint64_t xi = (ux >> base_bit) & chunk_mask;
         uint64_t partial = 0;
         for(unsigned b = 0; b < width; ++b)
         {
            const uint64_t lut_const = buildKcmLutConstant(uc, alpha, b, width);
            const uint64_t bit = (lut_const >> xi) & 1ULL;
            partial |= (bit << b);
         }
         if(base_bit < 64)
         {
            const uint64_t shifted = (partial << base_bit) & mask;
            result = (result + shifted) & mask;
         }
      }

      return result & mask;
   }

   inline int64_t kcmMulSigned(int64_t x, int64_t coeff, unsigned width, unsigned alpha)
   {
      if(alpha == 0 || width == 0)
      {
         return 0;
      }
      const bool neg = coeff < 0;
      const uint64_t abs_coeff = truncUnsigned(static_cast<uint64_t>(neg ? -coeff : coeff), width);
      const uint64_t ux = truncUnsigned(static_cast<uint64_t>(truncSigned(x, width)), width);
      uint64_t res = kcmMulUnsigned(ux, abs_coeff, width, alpha);
      if(neg)
      {
         res = truncUnsigned(static_cast<uint64_t>(~res) + 1ULL, width);
      }
      return truncSigned(static_cast<int64_t>(res), width);
   }
} // namespace kcm_constmul

#endif
