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
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _MEMORY_SYMBOL_HPP_
#define _MEMORY_SYMBOL_HPP_

#include "refcount.hpp"
#include <string>

class memory_symbol
{
   /// identifier of the variable
   unsigned int variable;

   /// name of the symbol
   std::string name;

   /// current variable address
   unsigned int address;

   /// flag to check if the memory address is defined or not (i.e., it has been actually resolved)
   bool resolved;

 public:
   /**
    * Constructor
    */
   memory_symbol(unsigned int var, unsigned int address, unsigned int funId);

   /**
    * Destructor
    */
   ~memory_symbol();

   /**
    * Sets the actual name for the variable symbol
    */
   void set_symbol_name(const std::string& name);

   /**
    * Returns the current name for the variable symbol
    */
   std::string get_symbol_name() const;

   /**
    * Sets the actual address for the variable
    */
   void set_address(unsigned int address);

   /**
    * Gets the current address for the variable
    */
   unsigned int get_address() const;

   /**
    * Gets the current the variable
    */
   unsigned int get_variable() const;

   /**
    * Sets if the variable has been actually resolved or not
    */
   void set_resolved(bool flag) const;

   /**
    * Returns if the variable address has been already resolved or not
    */
   bool is_resolved() const;

   /**
    * Returns the binary address in form of string
    * @param precision is the number of bits which the address should be represented
    */
   std::string get_address_string(unsigned int precision) const;

   bool notEQ(const memory_symbol& ref) const
   {
      return variable != ref.variable || name != ref.name || address != ref.address || resolved != ref.resolved;
   }
};
/// refcount definition of the class
typedef refcount<memory_symbol> memory_symbolRef;

#endif
