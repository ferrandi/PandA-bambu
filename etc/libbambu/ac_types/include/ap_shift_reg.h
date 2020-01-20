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
 *              Copyright (C) 2018-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
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
 * @file ap_shift_reg.h
 * @brief Very simple shift register model
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef __AP_SHIFT_REG_H
#define __AP_SHIFT_REG_H

#define __FORCE_INLINE __attribute__((always_inline)) inline

template <typename DATATYPE, unsigned N = 32>
struct ap_shift_reg
{
   ap_shift_reg() = default;
   ap_shift_reg(const char*)
   {
   }
   ap_shift_reg(const ap_shift_reg<DATATYPE, N>& sr) = delete;
   ap_shift_reg& operator=(const ap_shift_reg<DATATYPE, N>& sr) = delete;
   ~ap_shift_reg() = default;
   __FORCE_INLINE DATATYPE shift(DATATYPE val, unsigned index = N - 1, bool en = true)
   {
      DATATYPE res = data[index];
      if(en)
      {
         for(auto i = N - 1; i > 0; --i)
            data[i] = data[i - 1];
         data[0] = val;
      }
      return res;
   }
   __FORCE_INLINE DATATYPE read(unsigned index = N - 1) const
   {
      return data[index];
   }

 private:
   DATATYPE data[N];
};
#endif
