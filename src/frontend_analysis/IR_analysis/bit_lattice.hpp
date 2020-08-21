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
 * @file bit_lattice.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef _BIT_LATTICE_HPP_
#define _BIT_LATTICE_HPP_

/// Header include
#include "frontend_flow_step.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <deque>

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(tree_node);

enum class bit_lattice
{
   U,
   ZERO,
   ONE,
   X
};

class BitLatticeManipulator
{
 protected:
   const tree_managerConstRef TM;

   /**
    * @brief Map of the current bit-values of each variable.
    * Map storing the current bit-values of the variables at the end of each iteration of forward_transfer or backward_transfer.
    */
   CustomUnorderedMap<unsigned int, std::deque<bit_lattice>> current;

   /**
    * @brief Map of the best bit-values of each variable.
    * Map storing the best bit-values of the variables at the end of all the iterations of forward_transfer or backward_transfer.
    */
   CustomUnorderedMap<unsigned int, std::deque<bit_lattice>> best;

   /**
    * @brief Set storing the signed ssa
    */
   CustomUnorderedSet<unsigned int> signed_var;

   /// The debug level of methods of this class - it cannot be named debug_level because of ambiguity of FrontendFlowStep::debug_level derived classes
   const int bl_debug_level;

   /**
    * Computes the sup between two bitstrings
    * @param a first bitstring variable
    * @param b second bitstring variable
    * @param output_uid is the id of the tree node for which the bitvalue is * computed
    * @return the sup of the two bitstrings.
    */

   std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const unsigned int output_uid) const;

   /**
    * Computes the inf between two bitstrings
    * @param a first bitstring
    * @param b second bitstring
    * @param output_uid is the id of the tree node for which the bitvalue is * computed
    * @return inf between the two bitstrings
    */
   std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const unsigned int output_uid) const;

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
   bool is_handled_by_bitvalue(unsigned int type_id) const;

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
   bool update_current(std::deque<bit_lattice>& res, unsigned int output_uid);

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
   ~BitLatticeManipulator();

   /**
    * Extends a bitstring
    * @param bitstring to extend
    * @param bitstring_is_signed must be true if bitstring is signed
    * @param final_size desired length of the bitstrign
    * @return the extended bitstring
    */
   static std::deque<bit_lattice> sign_extend_bitstring(const std::deque<bit_lattice>& bitstring, bool bitstring_is_signed, size_t final_size);

   /**
    * @brief Reduce the size of a bitstring
    * 	erasing all but one most significant zeros in unsigned bitstring and all
    * 	but one most significant values in signed bitstrings.
    * 	@param bitstring bitstring to reduce.
    * 	@param bitstring_is_signed must be true if bitstring is signed
    */
   static void sign_reduce_bitstring(std::deque<bit_lattice>& bitstring, bool bitstring_is_signed);

   static bit_lattice bit_sup(const bit_lattice a, const bit_lattice b);

   static bit_lattice bit_inf(const bit_lattice a, const bit_lattice b);

   static std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const size_t out_type_size, const bool out_is_signed, const bool out_is_bool);

   static std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b, const size_t out_type_size, const bool out_is_signed, const bool out_is_bool);

   static unsigned int Size(const tree_nodeConstRef t);

   static unsigned int size(const tree_managerConstRef tm, unsigned int index);
};

/**
 * Creates a bitstring containing bits initialized at <U>
 * @param lenght the lenght of the bitstring
 * @return a bitstring of the specified length containing <U> values.
 */
std::deque<bit_lattice> create_u_bitstring(size_t lenght);

/**
 * Create a bitstring containing bits initialized at <X>
 * @param lenght the lenght of the bitstring
 * @return a bitstring of the specified length containing <X> values.
 */
std::deque<bit_lattice> create_x_bitstring(unsigned int lenght);

/**
 * Creates a bitstring from a constant input
 * @param value_int integer constant
 * @param length the length of the bitstring to be generated
 * @param signed_value specified if this bitstring can have negative values
 * @return bitstring generated from the integer constant
 */
std::deque<bit_lattice> create_bitstring_from_constant(long long int value_int, unsigned int length, bool signed_value);

/**
 * Translates a bitstring ( expressed as an std::deque of bit_lattice ) into a string of characters.
 */
std::string bitstring_to_string(const std::deque<bit_lattice>& bitstring);

/**
 * inverse of bitstring_to_string
 */
std::deque<bit_lattice> string_to_bitstring(const std::string& s);

/**
 * Checks if a bitstring is constant
 * @param a the bitstring to be checked
 * @return TRUE if the bitstring contains only 1, 0 or X but not U values
 */
bool bitstring_constant(const std::deque<bit_lattice>& a);
#endif
