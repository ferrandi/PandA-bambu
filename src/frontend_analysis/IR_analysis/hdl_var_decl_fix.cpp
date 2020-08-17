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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "string_manipulation.hpp" // for GET_CLASS

HDLVarDeclFix::HDLVarDeclFix(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : VarDeclFix(_AppM, _function_id, _design_flow_manager, _parameters, HDL_VAR_DECL_FIX), hdl_writer_type(static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language)))
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
   const auto hdl_writer = language_writer::create_writer(hdl_writer_type, GetPointer<HLS_manager>(AppM)->get_HLS_target()->get_technology_manager(), parameters);
   const auto hdl_reserved_names = hdl_writer->GetHDLReservedNames();
   already_examinated_names.insert(hdl_reserved_names.begin(), hdl_reserved_names.end());

   /// a parameter cannot be named "this"
   already_examinated_names.insert(std::string("this"));

   /// Add function name
   already_examinated_names.insert(Normalize(function_behavior->CGetBehavioralHelper()->get_function_name()));

   /// Fixing names of parameters
   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);

   if(HLSMgr && !HLSMgr->design_interface.empty() && HLSMgr->design_interface.find(fname) != HLSMgr->design_interface.end())
   {
      for(auto arg : fd->list_of_args)
      {
         auto a = GetPointer<parm_decl>(GET_NODE(arg));
         auto argName = GET_NODE(a->name);
         THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
         const std::string argName_string = GetPointer<identifier_node>(argName)->strg;
         recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         argName = GET_NODE(a->name);
         THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
         const std::string argName_string_new = GetPointer<identifier_node>(argName)->strg;
         if(argName_string != argName_string_new)
         {
            auto di_it = HLSMgr->design_interface.find(fname)->second.find(argName_string);
            auto di_value = di_it->second;
            HLSMgr->design_interface.find(fname)->second.erase(di_it);
            HLSMgr->design_interface.find(fname)->second[argName_string_new] = di_value;
            if(HLSMgr->design_interface_arraysize.find(fname) != HLSMgr->design_interface_arraysize.end() && HLSMgr->design_interface_arraysize.find(fname)->second.find(argName_string) != HLSMgr->design_interface_arraysize.find(fname)->second.end())
            {
               auto dia_it = HLSMgr->design_interface_arraysize.find(fname)->second.find(argName_string);
               auto dia_value = dia_it->second;
               HLSMgr->design_interface_arraysize.find(fname)->second.erase(dia_it);
               HLSMgr->design_interface_arraysize.find(fname)->second[argName_string_new] = dia_value;
            }
            if(HLSMgr->design_interface_attribute2.find(fname) != HLSMgr->design_interface_attribute2.end() && HLSMgr->design_interface_attribute2.find(fname)->second.find(argName_string) != HLSMgr->design_interface_attribute2.find(fname)->second.end())
            {
               auto dia_it = HLSMgr->design_interface_attribute2.find(fname)->second.find(argName_string);
               auto dia_value = dia_it->second;
               HLSMgr->design_interface_attribute2.find(fname)->second.erase(dia_it);
               HLSMgr->design_interface_attribute2.find(fname)->second[argName_string_new] = dia_value;
            }
            if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() && HLSMgr->design_interface_attribute3.find(fname)->second.find(argName_string) != HLSMgr->design_interface_attribute3.find(fname)->second.end())
            {
               auto dia_it = HLSMgr->design_interface_attribute3.find(fname)->second.find(argName_string);
               auto dia_value = dia_it->second;
               HLSMgr->design_interface_attribute3.find(fname)->second.erase(dia_it);
               HLSMgr->design_interface_attribute3.find(fname)->second[argName_string_new] = dia_value;
            }
            auto dit_it = HLSMgr->design_interface_typename.find(fname)->second.find(argName_string);
            auto dit_value = dit_it->second;
            HLSMgr->design_interface_typename.find(fname)->second.erase(dit_it);
            HLSMgr->design_interface_typename.find(fname)->second[argName_string_new] = dit_value;

            auto diti_it = HLSMgr->design_interface_typenameinclude.find(fname)->second.find(argName_string);
            auto diti_value = diti_it->second;
            HLSMgr->design_interface_typenameinclude.find(fname)->second.erase(diti_it);
            HLSMgr->design_interface_typenameinclude.find(fname)->second[argName_string_new] = diti_value;

            if(HLSMgr->design_interface_loads.find(fname) != HLSMgr->design_interface_loads.end())
            {
               for(auto& bb2parLoads : HLSMgr->design_interface_loads.find(fname)->second)
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
            if(HLSMgr->design_interface_stores.find(fname) != HLSMgr->design_interface_stores.end())
            {
               for(auto& bb2parLoads : HLSMgr->design_interface_stores.find(fname)->second)
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
      for(auto arg : fd->list_of_args)
         recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
   }

   VarDeclFix::InternalExec();

   return DesignFlowStep_Status::SUCCESS;
}

const std::string HDLVarDeclFix::Normalize(const std::string& identifier) const
{
   return hdl_writer_type == HDLWriter_Language::VHDL ? boost::to_upper_copy<std::string>(identifier) : identifier;
}
