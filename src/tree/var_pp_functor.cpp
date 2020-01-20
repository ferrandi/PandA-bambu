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
 * @file var_pp_functor.cpp
 * @brief Helper for reading data about internal representation after behavioral_manager analysis about specification produced from tree
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
/// Header include
#include "var_pp_functor.hpp"
#include "behavioral_helper.hpp"
#include <cstddef> // for size_t

pointer_var_pp_functor::pointer_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars, bool _add_restrict) : pointer_based_variables(vars), BH(_BH), std_functor(std_var_pp_functor(_BH)), add_restrict(_add_restrict)
{
}

std::string std_var_pp_functor::operator()(unsigned int var) const
{
   if(BH->is_an_indirect_ref(var))
   {
      return BH->PrintVariable(var);
   }
   if(BH->is_an_array_ref(var))
   {
      unsigned int array = BH->get_array_ref_array(var);
      unsigned int index = BH->get_array_ref_index(var);
      if(BH->is_a_mem_ref(array))
      {
         unsigned int base = BH->get_mem_ref_base(array);
         unsigned int offset = BH->get_mem_ref_offset(array);
         unsigned int type = BH->get_type(array);
         std::string offset_str = this->operator()(offset);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type))
         {
            size_t found_square_bracket = type_string.find("[");
            if(found_square_bracket != std::string::npos)
               type_string.insert(found_square_bracket, "(*)");
            else
               type_string = type_string + "*";
         }
         else
            type_string = type_string + "*";
         if(offset_str == "0")
            return "(*((" + type_string + ")(" + this->operator()(base) + ")))" + "[" + this->operator()(index) + "]";
         else
            return "(*((" + type_string + ")((unsigned char*)" + this->operator()(base) + " + " + offset_str + ")))" + "[" + this->operator()(index) + "]";
      }
      else
         return this->operator()(array) + "[" + this->operator()(index) + "]";
   }
   if(BH->is_a_component_ref(var))
   {
      unsigned int record = BH->get_component_ref_record(var);
      unsigned int field = BH->get_component_ref_field(var);
      return "(" + this->operator()(record) + ")." + this->operator()(field);
   }
   if(BH->is_an_addr_expr(var))
   {
      unsigned int pointed = BH->get_operand_from_unary_expr(var);
      if(BH->is_a_mem_ref(pointed))
      {
         unsigned int base = BH->get_mem_ref_base(pointed);
         unsigned int offset = BH->get_mem_ref_offset(pointed);
         unsigned int type = BH->get_type(pointed);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type))
         {
            size_t found_square_bracket = type_string.find("[");
            if(found_square_bracket != std::string::npos)
               type_string.insert(found_square_bracket, "(*)");
            else
               type_string = type_string + "*";
         }
         else
            type_string = type_string + "*";
         return "((" + type_string + ")((unsigned char*)" + this->operator()(base) + " + " + this->operator()(offset) + "))";
      }
      else
         return "(&(" + this->operator()(pointed) + "))";
   }
   if(BH->is_a_realpart_expr(var))
   {
      unsigned int complex = BH->get_operand_from_unary_expr(var);
      return "__real__ " + this->operator()(complex);
   }
   if(BH->is_a_imagpart_expr(var))
   {
      unsigned int complex = BH->get_operand_from_unary_expr(var);
      return "__imag__ " + this->operator()(complex);
   }
   return BH->PrintVariable(var);
}

std::string pointer_var_pp_functor::operator()(unsigned int var) const
{
   if(pointer_based_variables.find(var) == pointer_based_variables.end() and (not BH->IsDefaultSsaName(var) or pointer_based_variables.find(BH->GetVarFromSsa(var)) == pointer_based_variables.end()))
   {
      if(BH->is_an_indirect_ref(var))
      {
         unsigned int pointer = BH->get_indirect_ref_var(var);
         if(pointer_based_variables.find(pointer) != pointer_based_variables.end())
         {
            if(add_restrict)
               return "*__restrict__ " + BH->PrintVariable(var);
            else
               return "*" + BH->PrintVariable(var);
         }
         else
         {
            return BH->PrintVariable(var);
         }
      }
      if(BH->is_an_array_ref(var))
      {
         unsigned int array = BH->get_array_ref_array(var);
         unsigned int index = BH->get_array_ref_index(var);
         return std_functor(array) + "[" + this->operator()(index) + "]";
      }
      if(BH->is_a_component_ref(var))
      {
         unsigned int record = BH->get_component_ref_record(var);
         unsigned int field = BH->get_component_ref_field(var);
         return "(" + this->operator()(record) + ")." + this->operator()(field);
      }
      if(BH->is_an_addr_expr(var))
      {
         unsigned int pointed = BH->get_operand_from_unary_expr(var);
         if(BH->is_an_array(pointed))
            return this->operator()(pointed);
         else
            return "&(" + this->operator()(pointed) + ")";
      }
      if(BH->is_a_realpart_expr(var))
      {
         unsigned int complex = BH->get_operand_from_unary_expr(var);
         return "__real__ " + this->operator()(complex);
      }
      if(BH->is_a_imagpart_expr(var))
      {
         unsigned int complex = BH->get_operand_from_unary_expr(var);
         return "__imag__ " + this->operator()(complex);
      }
      return BH->PrintVariable(var);
   }
   else
   {
      if(BH->is_an_array(var))
         return BH->PrintVariable(var);
      else if(add_restrict)
         return "*__restrict__ " + BH->PrintVariable(var);
      else
         return "*" + BH->PrintVariable(var);
   }
}

address_var_pp_functor::address_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars, const CustomSet<unsigned int> pointer_vars) : addr_based_variables(vars), pointer_based_variables(pointer_vars), BH(_BH)
{
}

std::string address_var_pp_functor::operator()(unsigned int var) const
{
   /// pointer_based_variables are the I/O parameters of the function
   /// addr_based_variables are the I/O parameters of the called task
   if(pointer_based_variables.find(var) != pointer_based_variables.end())
   {
      if(addr_based_variables.find(var) != addr_based_variables.end())
      {
         return BH->PrintVariable(var);
      }
      else
      {
         return "*" + BH->PrintVariable(var);
      }
   }
   else
   {
      if(addr_based_variables.find(var) != addr_based_variables.end())
      {
         if(BH->is_an_array(var))
            return BH->PrintVariable(var);
         else
            return "&" + BH->PrintVariable(var);
      }
      else
      {
         return BH->PrintVariable(var);
      }
   }
   // not reachable point
   return "";
}

std::string isolated_var_pp_functor::operator()(unsigned int var) const
{
   if(BH->is_an_indirect_ref(var))
   {
      if(repl_var == var)
         return var_string;
      else
         return BH->PrintVariable(var);
   }
   if(BH->is_an_array_ref(var))
   {
      unsigned int array = BH->get_array_ref_array(var);
      unsigned int index = BH->get_array_ref_index(var);
      if(BH->is_a_mem_ref(array))
      {
         unsigned int base = BH->get_mem_ref_base(array);
         unsigned int offset = BH->get_mem_ref_offset(array);
         unsigned int type = BH->get_type(array);
         std::string offset_str = this->operator()(offset);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type))
         {
            size_t found_square_bracket = type_string.find("[");
            if(found_square_bracket != std::string::npos)
               type_string.insert(found_square_bracket, "(*)");
            else
               type_string = type_string + "*";
         }
         else
            type_string = type_string + "*";
         if(offset_str == "0")
            return "(*((" + type_string + ")(" + this->operator()(base) + ")))" + "[" + this->operator()(index) + "]";
         else
            return "(*((" + type_string + ")((unsigned char*)" + this->operator()(base) + " + " + offset_str + ")))" + "[" + this->operator()(index) + "]";
      }
      else
         return this->operator()(array) + "[" + this->operator()(index) + "]";
   }
   if(BH->is_a_component_ref(var))
   {
      unsigned int record = BH->get_component_ref_record(var);
      unsigned int field = BH->get_component_ref_field(var);
      return "(" + this->operator()(record) + ")." + this->operator()(field);
   }
   if(BH->is_an_addr_expr(var))
   {
      unsigned int pointed = BH->get_operand_from_unary_expr(var);
      if(BH->is_a_mem_ref(pointed))
      {
         unsigned int base = BH->get_mem_ref_base(pointed);
         unsigned int offset = BH->get_mem_ref_offset(pointed);
         unsigned int type = BH->get_type(pointed);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type))
         {
            size_t found_square_bracket = type_string.find("[");
            if(found_square_bracket != std::string::npos)
               type_string.insert(found_square_bracket, "(*)");
            else
               type_string = type_string + "*";
         }
         else
            type_string = type_string + "*";
         return "((" + type_string + ")((unsigned char*)" + this->operator()(base) + " + " + this->operator()(offset) + "))";
      }
      else
         return "(&(" + this->operator()(pointed) + "))";
   }
   if(BH->is_a_realpart_expr(var))
   {
      unsigned int complex = BH->get_operand_from_unary_expr(var);
      return "__real__ " + this->operator()(complex);
   }
   if(BH->is_a_imagpart_expr(var))
   {
      unsigned int complex = BH->get_operand_from_unary_expr(var);
      return "__imag__ " + this->operator()(complex);
   }
   if(repl_var == var)
      return var_string;
   else
      return BH->PrintVariable(var);
}
