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
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(tree_node);

class BitLatticeManipulator
{
 protected:
   const tree_managerConstRef TM;

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

   bool IsSignedIntegerType(const tree_nodeConstRef& tn) const;

   /**
    * Computes the sup between two bitstrings
    * @param a first bitstring variable
    * @param b second bitstring variable
    * @param output_uid is the id of the tree node for which the bitvalue is * computed
    * @return the sup of the two bitstrings.
    */
   std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const unsigned int output_uid) const;

   std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const tree_nodeConstRef& out_node) const;

   /**
    * Computes the inf between two bitstrings
    * @param a first bitstring
    * @param b second bitstring
    * @param output_uid is the id of the tree node for which the bitvalue is * computed
    * @return inf between the two bitstrings
    */
   std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const unsigned int output_uid) const;

   std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                               const tree_nodeConstRef& out_node) const;

   /**
    * auxiliary function used to build the bitstring lattice for read-only arrays
    * @param ctor_tn is the tree reindex or a tree node of the contructor
    * @param ssa_node_id is the ssa node id of the lattice destination
    */
   std::deque<bit_lattice> constructor_bitstring(const tree_nodeRef& ctor_tn, unsigned int ssa_node_id) const;

   /**
    * auxiliary function used to build the bitstring lattice for read-only string_cst
    * @param strcst_tn is a tree reindex or a tree node of the string_cst
    * @param ssa_node_id is the ssa node id of the lattice destination
    */
   std::deque<bit_lattice> string_cst_bitstring(const tree_nodeRef& strcst_tn, unsigned int ssa_node_id) const;

   /**
    * Returns true if the type identified by type_id is handled by bitvalue
    * analysis
    */
   bool IsHandledByBitvalue(const tree_nodeConstRef& tn) const;

   /**
    * Mixes the content of current and best using the sup operation, storing
    * the result in the best map.
    * Returns true if the best map was updated, false otherwise.
    */
   bool mix();

   /**
    * Given a bitstring res, and the id of a tree node ouput_uid, this
    * functions checks if it is necessary to update the bistring stored in
    * the current map used by the bitvalue analysis algorithm.
    */
   bool update_current(std::deque<bit_lattice>& res, const tree_nodeConstRef& tn);

   /**
    * Clean up the internal data structures
    */
   void clear();

 public:
   /**
    * Constructor
    */
   explicit BitLatticeManipulator(const tree_managerConstRef TM, const int debug_level);

   /**
    * Destructor
    */
   virtual ~BitLatticeManipulator();
};
#endif
