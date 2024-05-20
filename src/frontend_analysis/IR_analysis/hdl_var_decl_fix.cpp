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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
#include "hdl_var_decl_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/algorithm/string.hpp>

HDLVarDeclFix::HDLVarDeclFix(const application_managerRef _AppM, unsigned int _function_id,
                             const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : VarDeclFix(_AppM, _function_id, _design_flow_manager, _parameters, HDL_VAR_DECL_FIX),
      hdl_writer_type(parameters->getOption<HDLWriter_Language>(OPT_writer_language))
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
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);

   for(const auto& arg : fd->list_of_args)
   {
      auto a = GetPointer<parm_decl>(arg);
      THROW_ASSERT(GetPointer<identifier_node>(a->name), "unexpected condition");
      const std::string parm_name = GetPointer<identifier_node>(a->name)->strg;
      recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names,
                          already_visited_ae);
      if(HLSMgr)
      {
         THROW_ASSERT(GetPointer<identifier_node>(a->name), "unexpected condition");
         const std::string parm_name_new = GetPointer<identifier_node>(a->name)->strg;
         const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
         if(func_arch && parm_name != parm_name_new)
         {
            const auto parm_it = func_arch->parms.find(parm_name);
            if(parm_it != func_arch->parms.end())
            {
               func_arch->parms[parm_name_new] = parm_it->second;
               func_arch->parms.erase(parm_it);

               if(HLSMgr->design_interface_io.find(fname) != HLSMgr->design_interface_io.end())
               {
                  for(auto& [bbi, ioOps] : HLSMgr->design_interface_io.find(fname)->second)
                  {
                     const auto it = ioOps.find(parm_name);
                     if(it != ioOps.end())
                     {
                        ioOps[parm_name_new] = it->second;
                        ioOps.erase(it);
                     }
                  }
               }
            }
         }
      }
   }

   return VarDeclFix::InternalExec();
}

const std::string HDLVarDeclFix::Normalize(const std::string& identifier) const
{
   return hdl_writer_type == HDLWriter_Language::VHDL ? boost::to_upper_copy<std::string>(identifier) : identifier;
}
