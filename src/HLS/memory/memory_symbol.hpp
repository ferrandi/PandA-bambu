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
 * @file memory_symbol.hpp
 * @brief Datastructure to represent a memory symbol in HLS
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _MEMORY_SYMBOL_HPP_
#define _MEMORY_SYMBOL_HPP_

#include "refcount.hpp"
#include <string>

#include "string_manipulation.hpp" //STR

#define MEM_PREFIX "MEM_"

class memory_symbol
{
   /// identifier of the variable
   unsigned int variable;

   /// name of the symbol
   std::string symbol_name;

   /// name of the variable
   std::string name;

   /// current variable address
   unsigned long long int address;

 public:
   /**
    * Constructor
    */
   memory_symbol(unsigned int var, const std::string& _name, unsigned long long _address, unsigned int funId) : variable(var), symbol_name(STR(MEM_PREFIX) + "var_" + STR(var) + "_" + STR(funId)), name(_name), address(_address)
   {
   }

   /**
    * Destructor
    */
   ~memory_symbol() = default;

   /**
    * Sets the actual name for the variable symbol
    */
   void set_symbol_name(const std::string& _symbol_name)
   {
      symbol_name = _symbol_name;
   }

   /**
    * Returns the current name for the variable symbol
    */
   std::string get_symbol_name() const
   {
      return symbol_name;
   }

   /**
    * Sets the actual name for the variable
    */
   void set_name(const std::string& _name)
   {
      name = _name;
   }

   /**
    * Returns the current name for the variable
    */
   std::string get_name() const
   {
      return name;
   }

   /**
    * Sets the actual address for the variable
    */
   void set_address(unsigned long long _address)
   {
      address = _address;
   }

   /**
    * Gets the current address for the variable
    */
   unsigned long long int get_address() const
   {
      return address;
   }

   /**
    * Gets the current the variable
    */
   unsigned int get_variable() const
   {
      return variable;
   }

   bool notEQ(const memory_symbol& ref) const
   {
      return variable != ref.variable || name != ref.name || address != ref.address || symbol_name != ref.symbol_name;
   }
};
/// refcount definition of the class
typedef refcount<memory_symbol> memory_symbolRef;

#endif
