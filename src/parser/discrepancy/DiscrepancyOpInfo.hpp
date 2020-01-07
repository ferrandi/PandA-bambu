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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Castellana <michele.castellana@mail.polimi.it>
 */

#ifndef DISCREPANCY_INFO_HPP
#define DISCREPANCY_INFO_HPP

#include "custom_set.hpp"

// include from utility/
#include "refcount.hpp"
#include <string>

enum discrepancy_type_mask
{
   DISCR_NONE = 0,
   DISCR_ADDR = 1 << 0,
   DISCR_REAL = 1 << 1,
   DISCR_COMPLEX = 1 << 2,
   DISCR_VECTOR = 1 << 3,
};

inline static discrepancy_type_mask operator&(const discrepancy_type_mask& a, const discrepancy_type_mask& b)
{
   return static_cast<discrepancy_type_mask>(static_cast<int>(a) & static_cast<int>(b));
}

inline static discrepancy_type_mask& operator&=(discrepancy_type_mask& a, const discrepancy_type_mask& b)
{
   a = a & b;
   return a;
}

inline static discrepancy_type_mask operator|(const discrepancy_type_mask& a, const discrepancy_type_mask& b)
{
   return static_cast<discrepancy_type_mask>(static_cast<int>(a) | static_cast<int>(b));
}

inline static discrepancy_type_mask& operator|=(discrepancy_type_mask& a, const discrepancy_type_mask& b)
{
   a = a | b;
   return a;
}

/*
 * Data structure used to store parsing data
 */
class DiscrepancyOpInfo
{
 public:
   unsigned long n_cycles;
   unsigned int stg_fun_id;
   unsigned int op_id;
   unsigned int ssa_name_node_id;
   unsigned int bitsize;
   unsigned int vec_base_bitsize;
   enum discrepancy_type_mask type;
   bool is_bounded_op;
   CustomOrderedSet<unsigned int> start_states;
   CustomOrderedSet<unsigned int> exec_states;
   CustomOrderedSet<unsigned int> end_states;
   std::string ssa_name;
};

bool operator!=(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
bool operator==(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
bool operator<(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
bool operator<=(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
bool operator>(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
bool operator>=(const DiscrepancyOpInfo& a, const DiscrepancyOpInfo& b);
#endif
