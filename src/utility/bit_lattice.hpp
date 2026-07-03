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
 * @file bit_lattice.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef _BIT_LATTICE_HPP
#define _BIT_LATTICE_HPP
#include "panda_types.hpp"

#include <deque>
#include <string>

enum class bit_lattice
{
   U,
   ZERO,
   ONE,
   X
};

/**
 * Creates a bitstring containing bits initialized at \c U
 * @param lenght the lenght of the bitstring
 * @return a bitstring of the specified length containing \c U values.
 */
std::deque<bit_lattice> create_u_bitstring(size_t lenght);

/**
 * Create a bitstring containing bits initialized at \c X
 * @param lenght the lenght of the bitstring
 * @return a bitstring of the specified length containing \c X values.
 */
std::deque<bit_lattice> create_x_bitstring(size_t lenght);

/**
 * Creates a bitstring from a constant input
 * @param value_int integer constant
 * @param length the length of the bitstring to be generated
 * @param signed_value specified if this bitstring can have negative values
 * @return bitstring generated from the integer constant
 */
std::deque<bit_lattice> create_bitstring_from_constant(integer_cst_t value_int, unsigned long long length,
                                                       bool signed_value);

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

/**
 * Extends a bitstring
 * @param bitstring to extend
 * @param bitstring_is_signed must be true if bitstring is signed
 * @param final_size desired length of the bitstrign
 * @return the extended bitstring
 */
std::deque<bit_lattice> sign_extend_bitstring(const std::deque<bit_lattice>& bitstring, bool bitstring_is_signed,
                                              size_t final_size);

/**
 * @brief Reduce the size of a bitstring
 * 	erasing all but one most significant zeros in unsigned bitstring and all
 * 	but one most significant values in signed bitstrings.
 * 	@param bitstring bitstring to reduce.
 * 	@param bitstring_is_signed must be true if bitstring is signed
 */
void sign_reduce_bitstring(std::deque<bit_lattice>& bitstring, bool bitstring_is_signed);

bit_lattice bit_sup(const bit_lattice a, const bit_lattice b);

bit_lattice bit_inf(const bit_lattice a, const bit_lattice b);

std::deque<bit_lattice> sup(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                            const size_t out_type_size, const bool out_is_signed, const bool out_is_bool);

std::deque<bit_lattice> inf(const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b,
                            const size_t out_type_size, const bool out_is_signed, const bool out_is_bool);

bool isBetter(const std::string& a_string, const std::string& b_string);

#endif // _BIT_LATTICE_HPP
