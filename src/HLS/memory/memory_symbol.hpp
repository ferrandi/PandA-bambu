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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
