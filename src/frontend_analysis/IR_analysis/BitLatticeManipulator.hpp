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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file BitLatticeManipulator.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef _BIT_LATTICE_MANIPULATOR_HPP_
#define _BIT_LATTICE_MANIPULATOR_HPP_

#include "bit_lattice.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "panda_types.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

class BitLatticeManipulator
{
 protected:
   const ir_managerRef TM;

   /**
    * @brief Map of the current bit-values of each variable.
    * Map storing the current bit-values of the variables at the end of each iteration of forward_transfer or
    * backward_transfer.
    */
   CustomMap<unsigned int, std::deque<bit_lattice>> current;

   /**
    * @brief Map of the best bit-values of each variable.
    * Map storing the best bit-values of the variables at the end of all the iterations of forward_transfer or
    * backward_transfer.
    */
   CustomMap<unsigned int, std::deque<bit_lattice>> best;

   /**
    * @brief Set storing the signed ssa
    */
   CustomSet<unsigned int> signed_var;

   /// The debug level of methods of this class - it cannot be named debug_level because of ambiguity of
   /// FrontendFlowStep::debug_level derived classes
   const int bl_debug_level;

   bool IsSignedIntegerType(const ir_nodeConstRef& tn) const;

   /**
    * Computes the sup between two bitstrings
    * @param a first bitstring variable
    * @param b second bitstring variable
    * @param out_node is the IR node for which the bitvalue is computed
    * @return the sup of the two bitstrings.
    */
   std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const ir_nodeConstRef& out_node) const;

   /**
    * Computes the inf between two bitstrings
    * @param a first bitstring
    * @param b second bitstring
    * @param out_node is the IR node for which the bitvalue is computed
    * @return inf between the two bitstrings
    */
   std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const ir_nodeConstRef& out_node) const;

   /**
    * auxiliary function used to build the bitstring lattice for read-only arrays
    * @param ctor_tn is the IR reindex or an IR node of the contructor
    * @param ssa_node is the ssa node of the lattice destination
    * @param element_size is the size in bits of each array element
    */
   std::deque<bit_lattice> constructor_bitstring(const ir_nodeRef& ctor_tn, const ir_nodeRef& ssa_node,
                                                 unsigned long long element_size) const;

   /**
    * Mixes the content of current and best using the sup operation, storing
    * the result in the best map.
    * Returns true if the best map was updated, false otherwise.
    */
   bool mix();

   /**
    * Given a bitstring res, and the id of an IR node ouput_uid, this
    * functions checks if it is necessary to update the bistring stored in
    * the current map used by the bitvalue analysis algorithm.
    */
   bool update_current(std::deque<bit_lattice>& res, const ir_nodeConstRef& tn);

   /**
    * Clean up the internal data structures
    */
   void clear();

 public:
   explicit BitLatticeManipulator(const ir_managerRef& TM, const int debug_level);

   virtual ~BitLatticeManipulator() = default;

   /**
    * Check if given ir_node type is supported by the BitValue inference
    * @param tn ir_node to check
    * @return true Given ir_node type is supported
    * @return false Given ir_node type is not supported
    */
   static bool IsHandledByBitvalue(const ir_nodeConstRef& tn) __attribute__((pure));
};
#endif
