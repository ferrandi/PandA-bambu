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
 * @file var_pp_functor.cpp
 * @brief Helper for reading data about internal representation after behavioral_manager analysis about specification
 * produced from IR
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "var_pp_functor.hpp"

#include "behavioral_helper.hpp"

#include <cstddef>

pointer_var_pp_functor::pointer_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars,
                                               bool _add_restrict)
    : pointer_based_variables(vars), BH(_BH), std_functor(std_var_pp_functor(_BH)), add_restrict(_add_restrict)
{
}

std::string std_var_pp_functor::operator()(unsigned int var) const
{
   if(BH->is_an_indirect_ref(var))
   {
      return BH->PrintVariable(var);
   }

   if(BH->is_an_addr_node(var))
   {
      unsigned int pointed = BH->get_operand_from_unary_node(var);
      if(BH->is_a_mem_access(pointed))
      {
         unsigned int base = BH->get_mem_access_base(pointed);
         unsigned int type = BH->get_type(pointed);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type) && !BH->is_a_struct(type))
         {
            size_t found_square_bracket = type_string.find('[');
            if(found_square_bracket != std::string::npos)
            {
               type_string.insert(found_square_bracket, "(*)");
            }
            else
            {
               type_string = type_string + "*";
            }
         }
         else
         {
            type_string = type_string + "*";
         }
         return "((" + type_string + ")((unsigned char*)" + this->operator()(base) + "))";
      }
      return "(&(" + this->operator()(pointed) + "))";
   }
   return BH->PrintVariable(var);
}

std::string pointer_var_pp_functor::operator()(unsigned int var) const
{
   if(pointer_based_variables.find(var) == pointer_based_variables.end() and
      (not BH->IsDefaultSsaName(var) or
       pointer_based_variables.find(BH->GetVarFromSsa(var)) == pointer_based_variables.end()))
   {
      if(BH->is_an_indirect_ref(var))
      {
         unsigned int pointer = BH->get_indirect_ref_var(var);
         if(pointer_based_variables.find(pointer) != pointer_based_variables.end())
         {
            if(add_restrict)
            {
               return "*__restrict__ " + BH->PrintVariable(var);
            }
            else
            {
               return "*" + BH->PrintVariable(var);
            }
         }
         else
         {
            return BH->PrintVariable(var);
         }
      }
      if(BH->is_an_addr_node(var))
      {
         unsigned int pointed = BH->get_operand_from_unary_node(var);
         if(BH->is_an_array(pointed) && !BH->is_a_struct(pointed))
         {
            return this->operator()(pointed);
         }
         else
         {
            return "&(" + this->operator()(pointed) + ")";
         }
      }
      return BH->PrintVariable(var);
   }
   else
   {
      if(BH->is_an_array(var) && !BH->is_a_struct(var))
      {
         return BH->PrintVariable(var);
      }
      else if(add_restrict)
      {
         return "*__restrict__ " + BH->PrintVariable(var);
      }
      else
      {
         return "*" + BH->PrintVariable(var);
      }
   }
}

address_var_pp_functor::address_var_pp_functor(const BehavioralHelperConstRef _BH, const CustomSet<unsigned int> vars,
                                               const CustomSet<unsigned int> pointer_vars)
    : addr_based_variables(vars), pointer_based_variables(pointer_vars), BH(_BH)
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
         if(BH->is_an_array(var) && !BH->is_a_struct(var))
         {
            return BH->PrintVariable(var);
         }
         else
         {
            return "&" + BH->PrintVariable(var);
         }
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
      {
         return var_string;
      }
      else
      {
         return BH->PrintVariable(var);
      }
   }
   if(BH->is_an_addr_node(var))
   {
      unsigned int pointed = BH->get_operand_from_unary_node(var);
      if(BH->is_a_mem_access(pointed))
      {
         unsigned int base = BH->get_mem_access_base(pointed);
         unsigned int type = BH->get_type(pointed);
         std::string type_string = BH->print_type(type);
         if(BH->is_an_array(type) && !BH->is_a_struct(type))
         {
            size_t found_square_bracket = type_string.find('[');
            if(found_square_bracket != std::string::npos)
            {
               type_string.insert(found_square_bracket, "(*)");
            }
            else
            {
               type_string = type_string + "*";
            }
         }
         else
         {
            type_string = type_string + "*";
         }
         return "((" + type_string + ")((unsigned char*)" + this->operator()(base) + "))";
      }
      else
      {
         return "(&(" + this->operator()(pointed) + "))";
      }
   }
   if(repl_var == var)
   {
      return var_string;
   }
   else
   {
      return BH->PrintVariable(var);
   }
}
