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
 * @file memory.cpp
 * @brief Class for representing memory information
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "memory.hpp"
#include "memory_cs.hpp"

#include "memory_symbol.hpp"

#include "application_manager.hpp"
#include "call_graph_manager.hpp"

#include "Parameter.hpp"
#include "funit_obj.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "exceptions.hpp"
#include "utility.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

#include "math_function.hpp"

/// STL includes
#include <algorithm>
#include <vector>

/// tree include
#include "tree_node.hpp"

/// we start to allocate from internal_base_address_alignment byte to align address to internal_base_address_alignment bits
/// we can use address 0 in some cases but it is not safe in general.

memory::memory(const tree_managerRef _TreeM, unsigned long long int _off_base_address, unsigned int max_bram, bool _null_pointer_check, bool initial_internal_address_p, unsigned long long int initial_internal_address, const unsigned& _bus_addr_bitsize)
    : TreeM(_TreeM),
      maximum_private_memory_size(0),
      total_amount_of_private_memory(0),
      total_amount_of_parameter_memory(0),
      off_base_address(_off_base_address),
      next_off_base_address(_off_base_address),
      bus_data_bitsize(0),
      bus_size_bitsize(0),
      bram_bitsize(0),
      maxbram_bitsize(0),
      intern_shared_data(false),
      use_unknown_addresses(false),
      pointer_conversion(false),
      unaligned_accesses(false),
      all_pointers_resolved(false),
      implicit_memcpy(false),
      parameter_alignment(16),
      null_pointer_check(_null_pointer_check),
      packed_vars(false),
      bus_addr_bitsize(_bus_addr_bitsize)
{
   unsigned int max_bus_size = 2 * max_bram;
   external_base_address_alignment = internal_base_address_alignment = max_bus_size / 8;
   if(null_pointer_check && !initial_internal_address_p)
      next_base_address = internal_base_address_start = max_bus_size / 8;
   else if(initial_internal_address_p && initial_internal_address != 0)
   {
      align(initial_internal_address, internal_base_address_alignment);
      next_base_address = internal_base_address_start = initial_internal_address;
   }
   else
      next_base_address = internal_base_address_start = 0;
}

memory::~memory() = default;

memoryRef memory::create_memory(const ParameterConstRef _parameters, const tree_managerRef _TreeM, unsigned long long _off_base_address, unsigned int max_bram, bool _null_pointer_check, bool initial_internal_address_p,
                                unsigned int initial_internal_address, const unsigned int& _address_bitsize)
{
   if(_parameters->isOption(OPT_context_switch))
      return memoryRef(new memory_cs(_TreeM, _off_base_address, max_bram, _null_pointer_check, initial_internal_address_p, initial_internal_address, _address_bitsize));
   else
      return memoryRef(new memory(_TreeM, _off_base_address, max_bram, _null_pointer_check, initial_internal_address_p, initial_internal_address, _address_bitsize));
}

std::map<unsigned int, memory_symbolRef> memory::get_ext_memory_variables() const
{
   return external;
}

void memory::compute_next_base_address(unsigned long long int& address, unsigned int var, unsigned int alignment)
{
   const tree_nodeRef node = TreeM->get_tree_node_const(var);
   unsigned int size = 0;

   // The __builtin_wait_call associate an address to the call site to
   // identify it.  For this case we are allocating a word.
   if(GetPointer<gimple_call>(node))
   {
      size = (bus_addr_bitsize % 8 == 0) ? bus_addr_bitsize / 8 : (bus_addr_bitsize / 8) + 1;
   }
   else
   {
      /// compute the next base address
      size = compute_n_bytes(tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, var)));
   }
   address += size;
   /// align the memory address
   align(address, alignment);
}

void memory::add_internal_variable(unsigned int funID_scope, unsigned int var, const std::string& var_name)
{
   memory_symbolRef m_sym;
   if(in_vars.find(var) != in_vars.end())
   {
      m_sym = in_vars[var];
   }
   else if(is_private_memory(var))
   {
      if(null_pointer_check)
      {
         align(internal_base_address_start, tree_helper::get_var_alignment(TreeM, var));
         m_sym = memory_symbolRef(new memory_symbol(var, var_name, internal_base_address_start, funID_scope));
      }
      else
      {
         m_sym = memory_symbolRef(new memory_symbol(var, var_name, 0, funID_scope));
      }
   }
   else
   {
      align(next_base_address, tree_helper::get_var_alignment(TreeM, var));
      m_sym = memory_symbolRef(new memory_symbol(var, var_name, next_base_address, funID_scope));
   }
   add_internal_symbol(funID_scope, var, m_sym);
}

void memory::add_internal_variable_proxy(unsigned int funID_scope, unsigned int var)
{
   internal_variable_proxy[funID_scope].insert(var);
   proxied_variables.insert(var);
}

const CustomOrderedSet<unsigned int>& memory::get_proxied_internal_variables(unsigned int funID_scope) const
{
   THROW_ASSERT(has_proxied_internal_variables(funID_scope), "No proxy variables for " + STR(funID_scope));
   return internal_variable_proxy.find(funID_scope)->second;
}

bool memory::has_proxied_internal_variables(unsigned int funID_scope) const
{
   return internal_variable_proxy.find(funID_scope) != internal_variable_proxy.end();
}

bool memory::is_a_proxied_variable(unsigned int var) const
{
   return proxied_variables.find(var) != proxied_variables.end();
}

void memory::add_read_only_variable(unsigned var)
{
   read_only_vars.insert(var);
}

bool memory::is_read_only_variable(unsigned var) const
{
   return read_only_vars.find(var) != read_only_vars.end();
}

void memory::add_internal_symbol(unsigned int funID_scope, unsigned int var, const memory_symbolRef m_sym)
{
   if(external.find(var) != external.end())
      THROW_WARNING("The variable " + STR(var) + " has been already allocated out of the module");
   if(parameter.find(var) != parameter.end())
      THROW_WARNING("The variable " + STR(var) + " has been already set as a parameter");
   THROW_ASSERT(in_vars.find(var) == in_vars.end() || is_private_memory(var), "variable already allocated inside this module");

   internal[funID_scope][var] = m_sym;
   if(GetPointer<gimple_call>(TreeM->get_tree_node_const(var)))
      callSites[var] = m_sym;
   else
      in_vars[var] = m_sym;

   if(is_private_memory(var))
   {
      unsigned long long int allocated_memory = compute_n_bytes(tree_helper::size(TreeM, var));
      rangesize[var] = allocated_memory;
      align(rangesize[var], internal_base_address_alignment);
      total_amount_of_private_memory += allocated_memory;
      maximum_private_memory_size = std::max(maximum_private_memory_size, allocated_memory);
   }
   else
   {
      unsigned long long int address = m_sym->get_address();
      unsigned long long int address_orig = address;
      compute_next_base_address(address, var, internal_base_address_alignment);
      next_base_address = std::max(next_base_address, address);
      rangesize[var] = next_base_address - address_orig;
   }
}

unsigned int memory::count_non_private_internal_symbols() const
{
   unsigned int n_non_private = 0;
   for(auto iv_pair : in_vars)
      if(!is_private_memory(iv_pair.first))
         ++n_non_private;
   return n_non_private;
}

void memory::add_external_variable(unsigned int var, const std::string& var_name)
{
   align(next_off_base_address, tree_helper::get_var_alignment(TreeM, var));
   memory_symbolRef m_sym = memory_symbolRef(new memory_symbol(var, var_name, next_off_base_address, 0));
   add_external_symbol(var, m_sym);
}

void memory::add_external_symbol(unsigned int var, const memory_symbolRef m_sym)
{
   if(in_vars.find(var) != in_vars.end())
      THROW_WARNING("The variable " + STR(var) + " has been already internally allocated");
   if(parameter.find(var) != parameter.end())
      THROW_WARNING("The variable " + STR(var) + " has been already set as a parameter");
   external[var] = m_sym;
   unsigned long long int address = m_sym->get_address();
   compute_next_base_address(address, var, external_base_address_alignment);
   next_off_base_address = std::max(next_off_base_address, address);
}

void memory::add_private_memory(unsigned int var)
{
   private_memories.insert(var);
}

void memory::set_sds_var(unsigned int var, bool value)
{
   same_data_size_accesses[var] = value;
}

void memory::add_source_value(unsigned int var, unsigned int value)
{
   source_values[var].insert(value);
}

void memory::add_parameter(unsigned int funID_scope, unsigned int var, const std::string& var_name, bool is_last)
{
   memory_symbolRef m_sym = memory_symbolRef(new memory_symbol(var, var_name, next_base_address, funID_scope));
   add_parameter_symbol(funID_scope, var, m_sym);
   if(is_last)
   {
      unsigned long long int address = next_base_address;
      /// align in case is not aligned
      align(next_base_address, internal_base_address_alignment);
      total_amount_of_parameter_memory += next_base_address - address;
   }
}

void memory::add_parameter_symbol(unsigned int funID_scope, unsigned int var, const memory_symbolRef m_sym)
{
   if(external.find(var) != external.end())
      THROW_WARNING("The variable " + STR(var) + " has been already allocated out of the module");
   if(in_vars.find(var) != in_vars.end())
      THROW_WARNING("The variable " + STR(var) + " has been already internally allocated");
   /// allocation of the parameters
   params[var] = parameter[funID_scope][var] = m_sym;
   unsigned long long int address = m_sym->get_address();
   compute_next_base_address(next_base_address, var, parameter_alignment);
   next_base_address = std::max(next_base_address, address);
   total_amount_of_parameter_memory += next_base_address - address;
}

unsigned long long int memory::get_memory_address() const
{
   return next_off_base_address;
}

bool memory::is_internal_variable(unsigned int funID_scope, unsigned int var) const
{
   return internal.find(funID_scope) != internal.end() && internal.find(funID_scope)->second.find(var) != internal.find(funID_scope)->second.end();
}

bool memory::is_external_variable(unsigned int var) const
{
   return external.find(var) != external.end();
}

bool memory::is_private_memory(unsigned int var) const
{
   return private_memories.find(var) != private_memories.end();
}

bool memory::is_sds_var(unsigned int var) const
{
   THROW_ASSERT(has_sds_var(var), "variable not classified" + STR(var));
   return same_data_size_accesses.find(var)->second;
}

bool memory::has_sds_var(unsigned int var) const
{
   return same_data_size_accesses.find(var) != same_data_size_accesses.end();
}

bool memory::is_parameter(unsigned int funID_scope, unsigned int var) const
{
   return parameter.find(funID_scope) != parameter.end() && parameter.find(funID_scope)->second.find(var) != parameter.find(funID_scope)->second.end();
}

unsigned long long int memory::get_callSite_base_address(unsigned int var) const
{
   THROW_ASSERT(callSites.find(var) != callSites.end(), "Variable not yet allocated");
   return callSites.find(var)->second->get_address();
}

unsigned long long int memory::get_internal_base_address(unsigned int var) const
{
   THROW_ASSERT(in_vars.find(var) != in_vars.end(), "Variable not yet allocated");
   return in_vars.find(var)->second->get_address();
}

unsigned long long int memory::get_external_base_address(unsigned int var) const
{
   THROW_ASSERT(external.find(var) != external.end(), "Variable not yet allocated");
   return external.find(var)->second->get_address();
}

unsigned long long int memory::get_parameter_base_address(unsigned int funId, unsigned int var) const
{
   THROW_ASSERT(parameter.find(funId) != parameter.end(), "Function not yet allocated");
   THROW_ASSERT(parameter.find(funId)->second.find(var) != parameter.find(funId)->second.end(), "Function not yet allocated");
   return parameter.find(funId)->second.find(var)->second->get_address();
}

std::map<unsigned int, memory_symbolRef> memory::get_function_vars(unsigned int funID_scope) const
{
   if(internal.find(funID_scope) == internal.end())
      return std::map<unsigned int, memory_symbolRef>();
   return internal.find(funID_scope)->second;
}

std::map<unsigned int, memory_symbolRef> memory::get_function_parameters(unsigned int funID_scope) const
{
   if(parameter.find(funID_scope) == parameter.end())
      return std::map<unsigned int, memory_symbolRef>();
   return parameter.find(funID_scope)->second;
}

bool memory::has_callSite_base_address(unsigned int var) const
{
   return callSites.find(var) != callSites.end();
}

bool memory::has_internal_base_address(unsigned int var) const
{
   return in_vars.find(var) != in_vars.end();
}

bool memory::has_external_base_address(unsigned int var) const
{
   return external.find(var) != external.end();
}

bool memory::has_parameter_base_address(unsigned int var, unsigned int funId) const
{
   auto itr = parameter.find(funId);
   if(itr == parameter.end())
      return false;
   return itr->second.find(var) != itr->second.end();
}

bool memory::has_base_address(unsigned int var) const
{
   return external.find(var) != external.end() || in_vars.find(var) != in_vars.end() || params.find(var) != params.end() || callSites.find(var) != callSites.end();
}

unsigned long long memory::get_base_address(unsigned int var, unsigned int funId) const
{
   THROW_ASSERT(has_base_address(var), "Variable not yet allocated: @" + STR(var));
   if(has_callSite_base_address(var))
   {
      return get_callSite_base_address(var);
   }
   if(has_internal_base_address(var))
   {
      return get_internal_base_address(var);
   }
   if(funId && has_parameter_base_address(var, funId))
   {
      return get_parameter_base_address(var, funId);
   }
   return get_external_base_address(var);
}

unsigned long long int memory::get_first_address(unsigned int funId) const
{
   const std::map<unsigned int, memory_symbolRef>& internalVars = (internal.find(funId)->second);
   unsigned long long int minAddress = UINT_MAX;
   for(const auto& internalVar : internalVars)
   {
      unsigned int var = internalVar.first;
      if(internalVar.second && !is_private_memory(var) && !has_parameter_base_address(var, funId))
         minAddress = std::min(minAddress, internalVar.second->get_address());
   }
   if(minAddress == UINT_MAX)
   {
      const std::map<unsigned int, memory_symbolRef>& paramsVar = (parameter.find(funId)->second);
      for(const auto& itr : paramsVar)
      {
         unsigned int var = itr.first;
         if(!is_private_memory(var))
            minAddress = std::min(minAddress, itr.second->get_address());
      }
   }
   return minAddress;
}

unsigned long long int memory::get_last_address(unsigned int funId, const application_managerRef AppMgr) const
{
   const CustomOrderedSet<unsigned int> calledSet = AppMgr->CGetCallGraphManager()->get_called_by(funId);
   const std::map<unsigned int, memory_symbolRef>& internalVars = (internal.find(funId)->second);
   unsigned long long int maxAddress = 0;
   for(const auto& internalVar : internalVars)
   {
      unsigned int var = internalVar.first;
      if(!is_private_memory(var) && !has_parameter_base_address(var, funId) && has_base_address(var))
      {
         maxAddress = std::max(maxAddress, internalVar.second->get_address() + tree_helper::size(TreeM, var) / 8);
      }
   }
   if(AppMgr->hasToBeInterfaced(funId))
   {
      const std::map<unsigned int, memory_symbolRef>& paramsVar = get_function_parameters(funId);
      for(const auto& itr : paramsVar)
      {
         unsigned int var = itr.first;
         maxAddress = std::max(maxAddress, itr.second->get_address() + tree_helper::size(TreeM, var) / 8);
      }
   }
   for(unsigned int Itr : calledSet)
   {
      if(not AppMgr->hasToBeInterfaced(Itr))
      {
         maxAddress = std::max(get_last_address(Itr, AppMgr), maxAddress);
      }
   }

   return maxAddress;
}

memory_symbolRef memory::get_symbol(unsigned int var, unsigned int funId) const
{
   THROW_ASSERT(has_base_address(var), "Variable not yet allocated: @" + STR(var));
   if(has_callSite_base_address(var))
      return callSites.find(var)->second;
   if(has_internal_base_address(var))
      return in_vars.find(var)->second;
   if(funId && has_parameter_base_address(var, funId))
   {
      auto itr = parameter.find(funId);
      return itr->second.find(var)->second;
   }
   return external.find(var)->second;
}

unsigned long long int memory::get_rangesize(unsigned int var) const
{
   THROW_ASSERT(has_base_address(var), "Variable not yet allocated: @" + STR(var));
   return rangesize.find(var)->second;
}

void memory::reserve_space(unsigned int space)
{
   next_off_base_address += space;
   align(next_off_base_address, internal_base_address_alignment);
}

unsigned long long int memory::get_allocated_space() const
{
   return total_amount_of_private_memory + next_base_address - internal_base_address_start;
}

unsigned long long int memory::get_allocated_parameters_memory() const
{
   return total_amount_of_parameter_memory;
}

unsigned long long memory::get_max_address() const
{
   return std::max(next_base_address, maximum_private_memory_size + internal_base_address_start);
}

bool memory::is_parm_decl_copied(unsigned int var) const
{
   return parm_decl_copied.find(var) != parm_decl_copied.end();
}

void memory::add_parm_decl_copied(unsigned int var)
{
   parm_decl_copied.insert(var);
}

bool memory::is_parm_decl_stored(unsigned int var) const
{
   return parm_decl_stored.find(var) != parm_decl_stored.end();
}

void memory::add_parm_decl_stored(unsigned int var)
{
   parm_decl_stored.insert(var);
}

bool memory::is_actual_parm_loaded(unsigned int var) const
{
   return actual_parm_loaded.find(var) != actual_parm_loaded.end();
}

void memory::add_actual_parm_loaded(unsigned int var)
{
   actual_parm_loaded.insert(var);
}

void memory::set_internal_base_address_alignment(unsigned int _internal_base_address_alignment)
{
   THROW_ASSERT(_internal_base_address_alignment && !(_internal_base_address_alignment & (_internal_base_address_alignment - 1)), "alignment must be a power of two");
   internal_base_address_alignment = _internal_base_address_alignment;
   align(internal_base_address_start, internal_base_address_alignment);
   next_base_address = internal_base_address_start;
}

void memory::propagate_memory_parameters(const structural_objectRef src, const structural_managerRef tgt)
{
   std::map<std::string, std::string> res_parameters;

   if(src->ExistsParameter(MEMORY_PARAMETER))
   {
      std::vector<std::string> current_src_parameters = convert_string_to_vector<std::string>(src->GetParameter(MEMORY_PARAMETER), ";");
      for(const auto current_src_parameter : current_src_parameters)
      {
         std::vector<std::string> current_parameter = convert_string_to_vector<std::string>(current_src_parameter, "=");
         res_parameters[current_parameter[0]] = current_parameter[1];
      }
   }

   auto* srcModule = GetPointer<module>(src);
   // std::cout << srcModule->get_id() << std::endl;
   if(srcModule)
   {
      for(unsigned int i = 0; i < srcModule->get_internal_objects_size(); ++i)
      {
         structural_objectRef subModule = srcModule->get_internal_object(i);
         if(subModule->ExistsParameter(MEMORY_PARAMETER))
         {
            std::vector<std::string> current_src_parameters = convert_string_to_vector<std::string>(subModule->GetParameter(MEMORY_PARAMETER), ";");
            for(const auto current_src_parameter : current_src_parameters)
            {
               std::vector<std::string> current_parameter = convert_string_to_vector<std::string>(current_src_parameter, "=");
               res_parameters[current_parameter[0]] = current_parameter[1];
            }
         }
      }
   }

   if(not tgt->get_circ()->ExistsParameter(MEMORY_PARAMETER))
   {
      tgt->get_circ()->AddParameter(MEMORY_PARAMETER, "");
   }
   std::vector<std::string> current_tgt_parameters = convert_string_to_vector<std::string>(tgt->get_circ()->GetParameter(MEMORY_PARAMETER), ";");
   for(unsigned int l = 0; l < current_tgt_parameters.size(); l++)
   {
      std::vector<std::string> current_parameter = convert_string_to_vector<std::string>(current_tgt_parameters[l], "=");
      if(res_parameters.find(current_parameter[0]) != res_parameters.end() && res_parameters[current_parameter[0]] != current_parameter[1])
         THROW_ERROR("The parameter \"" + current_parameter[0] + "\" has been set with (at least) two different values");
      res_parameters[current_parameter[0]] = current_parameter[1];
   }

   if(res_parameters.size() == 0)
      return;

   std::string memory_parameters;
   for(auto& res_parameter : res_parameters)
   {
      if(memory_parameters.size())
         memory_parameters += ";";
      memory_parameters += res_parameter.first + "=" + res_parameter.second;
   }
   tgt->get_circ()->SetParameter(MEMORY_PARAMETER, memory_parameters);
}

void memory::add_memory_parameter(const structural_managerRef SM, const std::string& name, const std::string& value)
{
   if(not SM->get_circ()->ExistsParameter(MEMORY_PARAMETER))
   {
      SM->get_circ()->AddParameter(MEMORY_PARAMETER, "");
   }
   auto memory_parameters = SM->get_circ()->GetParameter(MEMORY_PARAMETER) + ";";
   std::vector<std::string> current_parameters = convert_string_to_vector<std::string>(memory_parameters, ";");
   for(const auto& l : current_parameters)
   {
      std::vector<std::string> current_parameter = convert_string_to_vector<std::string>(l, "=");
      THROW_ASSERT(current_parameter.size() == 2, "expected two elements");
      if(current_parameter[0] == name)
      {
         if(value == current_parameter[1])
            return;
         THROW_ERROR("The parameter \"" + name + "\" has been set with (at least) two different values: " + value + " != " + current_parameter[1]);
      }
   }
   memory_parameters += name + "=" + value;
   SM->get_circ()->SetParameter(MEMORY_PARAMETER, memory_parameters);
}

void memory::xwrite(xml_element* node)
{
   xml_element* Enode = node->add_child_element("HLS_memory");
   unsigned long long int base_address = off_base_address;
   WRITE_XVM(base_address, Enode);
   if(internal.size() or parameter.size())
   {
      xml_element* IntNode = Enode->add_child_element("internal_memory");
      for(auto iIt = internal.begin(); iIt != internal.end(); ++iIt)
      {
         xml_element* ScopeNode = IntNode->add_child_element("scope");
         std::string id = "@" + STR(iIt->first);
         WRITE_XVM(id, ScopeNode);
         std::string name = tree_helper::name_function(TreeM, iIt->first);
         WRITE_XVM(name, ScopeNode);
         for(auto vIt = iIt->second.begin(); vIt != iIt->second.end(); ++vIt)
         {
            xml_element* VarNode = ScopeNode->add_child_element("variable");
            std::string variable = "@" + STR(vIt->second->get_variable());
            WRITE_XNVM(id, variable, VarNode);
            unsigned long long int address = vIt->second->get_address();
            WRITE_XVM(address, VarNode);
            std::string var_symbol_name = vIt->second->get_symbol_name();
            WRITE_XNVM(name, var_symbol_name, VarNode);
            std::string var_name = vIt->second->get_name();
            WRITE_XNVM(name, var_name, VarNode);
         }
         if(parameter.find(iIt->first) != parameter.end())
         {
            auto params0 = parameter.find(iIt->first)->second;
            for(auto vIt = params0.begin(); vIt != params0.end(); ++vIt)
            {
               xml_element* VarNode = ScopeNode->add_child_element("parameter");
               std::string variable = "@" + STR(vIt->second->get_variable());
               WRITE_XNVM(id, variable, VarNode);
               unsigned long long int address = vIt->second->get_address();
               WRITE_XVM(address, VarNode);
               std::string var_symbol_name = vIt->second->get_symbol_name();
               WRITE_XNVM(symbol, var_symbol_name, VarNode);
               std::string var_name = vIt->second->get_symbol_name();
               WRITE_XNVM(symbol, var_name, VarNode);
            }
         }
      }
      if(parameter.size())
      {
         for(auto iIt = internal.begin(); iIt != internal.end(); ++iIt)
         {
            if(internal.find(iIt->first) != internal.end())
               continue;
            xml_element* ScopeNode = IntNode->add_child_element("scope");
            std::string id = "@" + STR(iIt->first);
            WRITE_XVM(id, ScopeNode);
            std::string name = tree_helper::name_function(TreeM, iIt->first);
            WRITE_XVM(name, ScopeNode);
            auto params0 = parameter.find(iIt->first)->second;
            for(auto vIt = params0.begin(); vIt != params0.end(); ++vIt)
            {
               xml_element* VarNode = ScopeNode->add_child_element("parameter");
               std::string variable = "@" + STR(vIt->second->get_variable());
               WRITE_XNVM(id, variable, VarNode);
               unsigned long long int address = vIt->second->get_address();
               WRITE_XVM(address, VarNode);
               std::string var_symbol_name = vIt->second->get_symbol_name();
               WRITE_XNVM(symbol, var_symbol_name, VarNode);
               std::string var_name = vIt->second->get_symbol_name();
               WRITE_XNVM(symbol, var_name, VarNode);
            }
         }
      }
   }
   if(external.size())
   {
      xml_element* ExtNode = Enode->add_child_element("external_memory");
      for(auto eIt = external.begin(); eIt != external.end(); ++eIt)
      {
         xml_element* VarNode = ExtNode->add_child_element("variable");
         std::string variable = "@" + STR(eIt->second->get_variable());
         WRITE_XNVM(id, variable, VarNode);
         unsigned long long int address = eIt->second->get_address();
         WRITE_XVM(address, VarNode);
         std::string var_name = eIt->second->get_symbol_name();
         WRITE_XNVM(symbol, var_name, VarNode);
      }
   }
}

bool memory::notEQ(refcount<memory> ref) const
{
   if(!ref)
      return true;
   auto neEQMapSymbolRef = [](const std::map<unsigned int, memory_symbolRef>& ref1, const std::map<unsigned int, memory_symbolRef>& ref2) -> bool {
      if(ref1.size() != ref2.size())
         return true;
      else
      {
         std::map<unsigned int, memory_symbolRef>::const_iterator i_it, j_it;
         for(i_it = ref1.begin(), j_it = ref2.begin(); i_it != ref1.end(); ++i_it, ++j_it)
         {
            if(i_it->first != j_it->first)
               return true;
            if((i_it->second)->notEQ(*(j_it->second)))
               return true;
         }
      }
      return false;
   };
   auto neEQ2MapSymbolRef = [&neEQMapSymbolRef](const std::map<unsigned int, std::map<unsigned int, memory_symbolRef>>& ref1, const std::map<unsigned int, std::map<unsigned int, memory_symbolRef>>& ref2) -> bool {
      if(ref1.size() != ref2.size())
         return true;
      else
      {
         std::map<unsigned int, std::map<unsigned int, memory_symbolRef>>::const_iterator i_it, j_it;
         for(i_it = ref1.begin(), j_it = ref2.begin(); i_it != ref1.end(); ++i_it, ++j_it)
         {
            if(i_it->first != j_it->first)
               return true;
            if(neEQMapSymbolRef(i_it->second, j_it->second))
               return true;
         }
      }
      return false;
   };
   if(neEQMapSymbolRef(external, ref->external))
      return true;
   if(neEQ2MapSymbolRef(internal, ref->internal))
      return true;
   if(internal_variable_proxy != ref->internal_variable_proxy)
      return true;
   if(proxied_variables != ref->proxied_variables)
      return true;
   if(read_only_vars != ref->read_only_vars)
      return true;
   if(neEQMapSymbolRef(in_vars, ref->in_vars))
      return true;
   if(rangesize != ref->rangesize)
      return true;
   if(neEQ2MapSymbolRef(parameter, ref->parameter))
      return true;
   if(neEQMapSymbolRef(params, ref->params))
      return true;
   if(neEQMapSymbolRef(callSites, ref->callSites))
      return true;
   if(private_memories != ref->private_memories)
      return true;
   if(same_data_size_accesses != ref->same_data_size_accesses)
      return true;
   // may oscillate
   //   if(source_values != ref->source_values)
   //      return true;
   if(parm_decl_copied != ref->parm_decl_copied)
      return true;
   if(parm_decl_stored != ref->parm_decl_stored)
      return true;
   if(actual_parm_loaded != ref->actual_parm_loaded)
      return true;
   if(next_base_address != ref->next_base_address)
      return true;
   if(internal_base_address_start != ref->internal_base_address_start)
      return true;
   if(maximum_private_memory_size != ref->maximum_private_memory_size)
      return true;
   if(total_amount_of_private_memory != ref->total_amount_of_private_memory)
      return true;
   if(total_amount_of_parameter_memory != ref->total_amount_of_parameter_memory)
      return true;
   if(off_base_address != ref->off_base_address)
      return true;
   if(next_off_base_address != ref->next_off_base_address)
      return true;
   if(bus_data_bitsize != ref->bus_data_bitsize)
      return true;
   if(bus_size_bitsize != ref->bus_size_bitsize)
      return true;
   if(bram_bitsize != ref->bram_bitsize)
      return true;
   if(maxbram_bitsize != ref->maxbram_bitsize)
      return true;
   if(intern_shared_data != ref->intern_shared_data)
      return true;
   if(use_unknown_addresses != ref->use_unknown_addresses)
      return true;
   if(pointer_conversion != ref->pointer_conversion)
      return true;
   if(unaligned_accesses != ref->unaligned_accesses)
      return true;
   if(all_pointers_resolved != ref->all_pointers_resolved)
      return true;
   if(implicit_memcpy != ref->implicit_memcpy)
      return true;
   if(internal_base_address_alignment != ref->internal_base_address_alignment)
      return true;
   if(external_base_address_alignment != ref->external_base_address_alignment)
      return true;
   if(parameter_alignment != ref->parameter_alignment)
      return true;
   // if(n_mem_operations_per_var != ref->n_mem_operations_per_var)
   //   return true;
   if(null_pointer_check != ref->null_pointer_check)
      return true;
   if(maximum_references != ref->maximum_references)
      return true;
   if(maximum_loads != ref->maximum_loads)
      return true;
   if(need_bus != ref->need_bus)
      return true;
   if(packed_vars != ref->packed_vars)
      return true;
   if(bus_addr_bitsize != ref->bus_addr_bitsize)
      return true;

   return false;
}

void memory::xwrite2(xml_element* node)
{
   xml_element* Enode = node->add_child_element("memory_allocation");
   unsigned long long int base_address = off_base_address;
   WRITE_XVM(base_address, Enode);
   for(auto int_obj : internal)
   {
      for(auto var_obj : int_obj.second)
      {
         xml_element* ObjNode = Enode->add_child_element("object");
         std::string scope = tree_helper::name_function(TreeM, int_obj.first);
         WRITE_XVM(scope, ObjNode);
         std::string name = var_obj.second->get_name();
         WRITE_XVM(name, ObjNode);
         WRITE_XNVM(is_internal, "T", ObjNode);
      }
   }
   for(auto ext_obj : external)
   {
      xml_element* ObjNode = Enode->add_child_element("object");
      std::string name = ext_obj.second->get_name();
      WRITE_XVM(name, ObjNode);
      WRITE_XNVM(is_internal, "F", ObjNode);
   }
}

void memory::xwrite(const std::string& filename)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("memory");
      xwrite2(nodeRoot);
      document.write_to_file_formatted(filename);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}
