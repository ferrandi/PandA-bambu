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
 * @file memory_symbol.hpp
 * @brief Datastructure to represent a memory symbol in HLS
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef _MEMORY_SYMBOL_HPP_
#define _MEMORY_SYMBOL_HPP_

#include "refcount.hpp"
#include "string_manipulation.hpp"

#include <string>

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
   memory_symbol(unsigned int var, const std::string& _name, unsigned long long _address, unsigned int funId)
       : variable(var),
         symbol_name(STR(MEM_PREFIX) + "var_" + STR(var) + "_" + STR(funId)),
         name(_name),
         address(_address)
   {
   }

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
using memory_symbolRef = refcount<memory_symbol>;

#endif
