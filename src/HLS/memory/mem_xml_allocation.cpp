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
 * @file mem_xml_allocation.cpp
 * @brief Parsing of memory allocation described in XML
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mem_xml_allocation.hpp"

#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"
#include "target_device.hpp"
#include "technology_manager.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"

mem_xml_allocation::mem_xml_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : memory_allocation(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::XML_MEMORY_ALLOCATOR)
{
}

mem_xml_allocation::~mem_xml_allocation() = default;

DesignFlowStep_Status mem_xml_allocation::InternalExec()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reading memory allocation from XML file...");
   memoryRef prevRmem = HLSMgr->Rmem;
   std::string xml_file;
   if(parameters->isOption(OPT_xml_memory_allocation))
      xml_file = parameters->getOption<std::string>(OPT_xml_memory_allocation);
   else if(parameters->isOption(OPT_xml_input_configuration))
      xml_file = parameters->getOption<std::string>(OPT_xml_input_configuration);
   else
      THROW_ERROR("Memory configuration file not specified");
   if(!boost::filesystem::exists(xml_file))
      THROW_ERROR("Memory configuration file \"" + xml_file + "\" does not exist");
   if(!parse_xml_allocation(xml_file))
      THROW_ERROR("Memory configuration not found in file \"" + xml_file + "\"");
   bool changed = HLSMgr->Rmem->notEQ(prevRmem);
   if(changed)
   {
      HLSMgr->UpdateMemVersion();
      /// clean proxy library
      auto HLS_T = HLSMgr->get_HLS_target();
      auto TM = HLS_T->get_technology_manager();
      TM->erase_library(PROXY_LIBRARY);
      TM->erase_library(WORK_LIBRARY);
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool mem_xml_allocation::parse_xml_allocation(const std::string& xml_file)
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   auto max_bram = HLS_T->get_target_device()->get_parameter<unsigned int>("BRAM_bitsize_max");
   bool initial_internal_address_p = parameters->isOption(OPT_initial_internal_address);
   unsigned int initial_internal_address = initial_internal_address_p ? parameters->getOption<unsigned int>(OPT_initial_internal_address) : std::numeric_limits<unsigned int>::max();
   bool null_pointer_check = true;
   if(parameters->isOption(OPT_gcc_optimizations))
   {
      const auto gcc_parameters = parameters->getOption<const CustomSet<std::string>>(OPT_gcc_optimizations);
      if(gcc_parameters.find("no-delete-null-pointer-checks") != gcc_parameters.end())
         null_pointer_check = false;
   }
   XMLDomParser parser(xml_file);
   parser.Exec();
   if(parser)
   {
      const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
      const xml_node::node_list list = node->get_children();
      for(const auto& l : list)
      {
         const xml_element* child = GetPointer<xml_element>(l);
         if(!child)
            continue;
         if(child->get_name() == "HLS_memory")
         {
            unsigned int base_address = 0;
            LOAD_XVM(base_address, child);
            HLSMgr->base_address = base_address;
            // HLSMgr->Rmem = memoryRef(new memory(TreeM, base_address, max_bram, null_pointer_check, initial_internal_address_p, initial_internal_address, HLSMgr->get_address_bitsize()));
            HLSMgr->Rmem = memoryRef(memory::create_memory(parameters, TreeM, base_address, max_bram, null_pointer_check, initial_internal_address_p, initial_internal_address, HLSMgr->get_address_bitsize()));
            setup_memory_allocation();

            const xml_node::node_list mem_list = child->get_children();
            for(const auto& it : mem_list)
            {
               const xml_element* mem_node = GetPointer<xml_element>(it);
               if(!mem_node)
                  continue;
               if(mem_node->get_name() == "external_memory")
               {
                  parse_external_allocation(mem_node);
               }
               else if(mem_node->get_name() == "internal_memory")
               {
                  parse_internal_allocation(mem_node);
               }
               else
               {
                  THROW_ERROR("Memory node \"" + mem_node->get_name() + "\"");
               }
            }
            finalize_memory_allocation();
            return true;
         }
      }
   }
   return false;
}

unsigned int get_id(const std::string& var_string)
{
   std::string var_id = var_string.substr(var_string.find("@") + 1, var_string.size());
   return boost::lexical_cast<unsigned int>(var_id);
}

void mem_xml_allocation::parse_external_allocation(const xml_element* node)
{
   const xml_node::node_list mem_list = node->get_children();
   for(const auto& it : mem_list)
   {
      const xml_element* mem_node = GetPointer<xml_element>(it);
      if(!mem_node)
         continue;
      if(mem_node->get_name() == "variable")
      {
         std::string var_id;
         LOAD_XVFM(var_id, mem_node, id);
         unsigned int tree_id = get_id(var_id);
         unsigned int address = 0;
         LOAD_XVM(address, mem_node);
         ext_variables[tree_id] = memory_symbolRef(new memory_symbol(tree_id, address, 0));
         std::string symbol;
         if(CE_XVM(symbol, mem_node))
         {
            LOAD_XVM(symbol, mem_node);
            ext_variables[tree_id]->set_symbol_name(symbol);
         }
      }
   }
}

void mem_xml_allocation::parse_internal_allocation(const xml_element* node)
{
   const xml_node::node_list mem_list = node->get_children();
   for(const auto& it : mem_list)
   {
      const xml_element* mem_node = GetPointer<xml_element>(it);
      if(!mem_node)
         continue;
      if(mem_node->get_name() == "scope")
      {
         std::string scope_id;
         LOAD_XVFM(scope_id, mem_node, id);
         unsigned int scp_id = get_id(scope_id);
         const xml_node::node_list var_list = mem_node->get_children();
         for(const auto& v : var_list)
         {
            const xml_element* var_node = GetPointer<xml_element>(v);
            if(!var_node)
               continue;
            if(var_node->get_name() == "variable")
            {
               std::string var_id;
               LOAD_XVFM(var_id, var_node, id);
               unsigned int tree_id = get_id(var_id);
               unsigned int address;
               LOAD_XVM(address, var_node);
               int_variables[scp_id][tree_id] = memory_symbolRef(new memory_symbol(tree_id, address, scp_id));
               std::string symbol;
               if(CE_XVM(symbol, mem_node))
               {
                  LOAD_XVM(symbol, mem_node);
                  int_variables[scp_id][tree_id]->set_symbol_name(symbol);
               }
            }
            else if(var_node->get_name() == "parameter")
            {
               std::string var_id;
               LOAD_XVFM(var_id, var_node, id);
               unsigned int tree_id = get_id(var_id);
               unsigned int address;
               LOAD_XVM(address, var_node);
               param_variables[scp_id][tree_id] = memory_symbolRef(new memory_symbol(tree_id, address, scp_id));
               std::string symbol;
               if(CE_XVM(symbol, mem_node))
               {
                  LOAD_XVM(symbol, mem_node);
                  param_variables[scp_id][tree_id]->set_symbol_name(symbol);
               }
            }
            else
            {
               THROW_ERROR("Node \"" + var_node->get_name() + "\" not supported");
            }
         }
      }
   }
}

void mem_xml_allocation::finalize_memory_allocation()
{
   for(auto& ext_variable : ext_variables)
   {
      HLSMgr->Rmem->add_external_symbol(ext_variable.first, ext_variable.second);
   }
   for(auto& int_variable : int_variables)
   {
      for(auto vIt = int_variable.second.begin(); vIt != int_variable.second.end(); ++vIt)
      {
         HLSMgr->Rmem->add_internal_symbol(int_variable.first, vIt->first, vIt->second);
      }
   }
   for(auto& param_variable : param_variables)
   {
      for(auto vIt = param_variable.second.begin(); vIt != param_variable.second.end(); ++vIt)
      {
         HLSMgr->Rmem->add_parameter_symbol(param_variable.first, vIt->first, vIt->second);
      }
   }
   memory_allocation::finalize_memory_allocation();
}
