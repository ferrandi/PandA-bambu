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
 *              Copyright (C) 2015-2023 Politecnico di Milano
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
 * @file hdl_var_decl_fix.cpp
 * @brief Pre-analysis step fixing var_decl duplication and HDL name conflicts.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "hdl_var_decl_fix.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// boost include
#include <boost/algorithm/string.hpp>

/// design_flows/backend/ToHDL include
#include "language_writer.hpp"

/// HLS includes
#include "hls_device.hpp"
#include "hls_manager.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "string_manipulation.hpp" // for GET_CLASS

HDLVarDeclFix::HDLVarDeclFix(const application_managerRef _AppM, unsigned int _function_id,
                             const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : VarDeclFix(_AppM, _function_id, _design_flow_manager, _parameters, HDL_VAR_DECL_FIX),
      hdl_writer_type(static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

HDLVarDeclFix::~HDLVarDeclFix() = default;

DesignFlowStep_Status HDLVarDeclFix::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   /// Already considered decl_node
   CustomUnorderedSet<unsigned int> already_examinated_decls;

   /// Already found variable and parameter names
   CustomUnorderedSet<std::string> already_examinated_names;

   /// Already found type names
   CustomUnorderedSet<std::string> already_examinated_type_names;

   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   /// Preload backend names
   const auto hdl_writer = language_writer::create_writer(
       hdl_writer_type, GetPointer<HLS_manager>(AppM)->get_HLS_device()->get_technology_manager(), parameters);
   const auto hdl_reserved_names = hdl_writer->GetHDLReservedNames();
   already_examinated_names.insert(hdl_reserved_names.begin(), hdl_reserved_names.end());

   /// a parameter cannot be named "this"
   already_examinated_names.insert(std::string("this"));

   /// Add function name
   already_examinated_names.insert(Normalize(function_behavior->CGetBehavioralHelper()->get_function_name()));

   /// Fixing names of parameters
   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);

   /* Check if there is at least one interface type */
   bool type_found = false;
   if(HLSMgr)
   {
      for(auto& fun : HLSMgr->design_attributes)
      {
         for(auto& par : fun.second)
         {
            if(par.second.find(attr_interface_type) != par.second.end())
            {
               type_found = true;
            }
         }
      }
   }

   if(HLSMgr && type_found)
   {
      for(const auto& arg : fd->list_of_args)
      {
         auto a = GetPointer<parm_decl>(GET_NODE(arg));
         auto argName = GET_NODE(a->name);
         THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
         const std::string argName_string = GetPointer<identifier_node>(argName)->strg;
         recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
         argName = GET_NODE(a->name);
         THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
         const std::string argName_string_new = GetPointer<identifier_node>(argName)->strg;
         if(argName_string != argName_string_new)
         {
            auto di_it =
                HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.find(attr_interface_type);
            auto di_value = di_it->second;
            HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.erase(di_it);
            HLSMgr->design_attributes.find(fname)->second[argName_string_new][attr_interface_type] = di_value;
            if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
               HLSMgr->design_attributes.at(fname).find(argName_string) != HLSMgr->design_attributes.at(fname).end() &&
               HLSMgr->design_attributes.at(fname).at(argName_string).find(attr_interface_type) !=
                   HLSMgr->design_attributes.at(fname).at(argName_string).end())
            {
               auto dia_it = HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.find(attr_size);
               auto dia_value = dia_it->second;
               HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.erase(dia_it);
               HLSMgr->design_attributes.find(fname)->second[argName_string_new][attr_size] = dia_value;
            }
            if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
               HLSMgr->design_attributes.at(fname).find(argName_string) != HLSMgr->design_attributes.at(fname).end() &&
               HLSMgr->design_attributes.at(fname).at(argName_string).find(attr_offset) !=
                   HLSMgr->design_attributes.at(fname).at(argName_string).end())
            {
               auto dia_it =
                   HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.find(attr_offset);
               auto dia_value = dia_it->second;
               HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.erase(dia_it);
               HLSMgr->design_attributes.find(fname)->second[argName_string_new][attr_offset] = dia_value;
            }
            if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
               HLSMgr->design_attributes.at(fname).find(argName_string) != HLSMgr->design_attributes.at(fname).end() &&
               HLSMgr->design_attributes.at(fname).at(argName_string).find(attr_bundle_name) !=
                   HLSMgr->design_attributes.at(fname).at(argName_string).end())
            {
               auto dia_it =
                   HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.find(attr_bundle_name);
               auto dia_value = dia_it->second;
               HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.erase(dia_it);
               HLSMgr->design_attributes.find(fname)->second[argName_string_new][attr_bundle_name] = dia_value;
            }
            auto dit_it =
                HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.find(attr_typename);
            auto dit_value = dit_it->second;
            HLSMgr->design_attributes.find(fname)->second.find(argName_string)->second.erase(dit_it);
            HLSMgr->design_attributes.find(fname)->second[argName_string_new][attr_typename] = dit_value;

            auto diti_it = HLSMgr->design_interface_typenameinclude.find(fname)->second.find(argName_string);
            auto diti_value = diti_it->second;
            HLSMgr->design_interface_typenameinclude.find(fname)->second.erase(diti_it);
            HLSMgr->design_interface_typenameinclude.find(fname)->second[argName_string_new] = diti_value;

            if(HLSMgr->design_interface_io.find(fname) != HLSMgr->design_interface_io.end())
            {
               for(auto& bb2parLoads : HLSMgr->design_interface_io.find(fname)->second)
               {
                  if(bb2parLoads.second.find(argName_string) != bb2parLoads.second.end())
                  {
                     auto l_it = bb2parLoads.second.find(argName_string);
                     auto l_value = l_it->second;
                     bb2parLoads.second.erase(l_it);
                     bb2parLoads.second[argName_string_new] = l_value;
                  }
               }
            }
         }
      }
   }
   else
   {
      for(const auto& arg : fd->list_of_args)
      {
         recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                             already_visited_ae);
      }
   }

   return VarDeclFix::InternalExec();
}

const std::string HDLVarDeclFix::Normalize(const std::string& identifier) const
{
   return hdl_writer_type == HDLWriter_Language::VHDL ? boost::to_upper_copy<std::string>(identifier) : identifier;
}
