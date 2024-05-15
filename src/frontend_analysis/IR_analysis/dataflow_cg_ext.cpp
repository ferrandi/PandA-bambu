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
 *              Copyright (C) 2024 Politecnico di Milano
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
 * @file dataflow_cg_ext.cpp
 * @brief Dataflow call graph extension
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "dataflow_cg_ext.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "dead_code_elimination.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"

dataflow_cg_ext::dataflow_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DATAFLOW_CG_EXT, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
dataflow_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DATAFLOW_CG_EXT, CALLING_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool dataflow_cg_ext::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

static void CleanVirtuals(const tree_managerRef& TM, const tree_nodeRef& call_stmt)
{
   const auto gn = GetPointerS<gimple_node>(call_stmt);
   if(gn->vdef)
   {
      dead_code_elimination::kill_vdef(TM, gn->vdef);
      gn->vdef = nullptr;
   }
   std::for_each(gn->vuses.begin(), gn->vuses.end(), [&](auto& it) { GetPointer<ssa_name>(it)->RemoveUse(call_stmt); });
   gn->vuses.clear();
   std::for_each(gn->vovers.begin(), gn->vovers.end(),
                 [&](auto& it) { GetPointer<ssa_name>(it)->RemoveUse(call_stmt); });
   gn->vovers.clear();
   THROW_ASSERT(!gn->memdef && !gn->memuse, "Unexpected condition");
}

DesignFlowStep_Status dataflow_cg_ext::InternalExec()
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto fsymbol = function_behavior->CGetBehavioralHelper()->GetMangledFunctionName();
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
   const auto is_dataflow_top =
       func_arch && func_arch->attrs.find(FunctionArchitecture::func_dataflow_top) != func_arch->attrs.end() &&
       func_arch->attrs.find(FunctionArchitecture::func_dataflow_top)->second == "1";
   if(!is_dataflow_top)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   const auto TM = AppM->get_tree_manager();
   const auto CGM = AppM->GetCallGraphManager();
   const auto CG = CGM->CGetCallGraph();
   const auto f_v = CGM->GetVertex(function_id);

   tree_manipulation tree_man(TM, parameters, AppM);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Expand Dataflow modules");
   std::vector<unsigned int> new_modules;
   BOOST_FOREACH(EdgeDescriptor ie, boost::out_edges(f_v, *CG))
   {
      auto tgt = boost::target(ie, *CG);
      const auto target_id = CGM->get_function(tgt);
      const auto tsymbol = AppM->CGetFunctionBehavior(target_id)->CGetBehavioralHelper()->GetMangledFunctionName();
      const auto tarch = HLSMgr->module_arch->GetArchitecture(tsymbol);
      const auto is_dataflow_module =
          tarch && tarch->attrs.find(FunctionArchitecture::func_dataflow_module) != tarch->attrs.end() &&
          tarch->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1";
      if(!is_dataflow_module)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function " + tsymbol + " is not a dataflow module");
         continue;
      }
      const auto call_info = CG->CGetFunctionEdgeInfo(ie);
      if(call_info->function_addresses.size() || call_info->indirect_call_points.size())
      {
         THROW_ERROR("Address/indirect function calls not supported in dataflow.");
      }

      const auto fnode = TM->GetTreeNode(function_id);
      auto is_single_call = boost::in_degree(tgt, *CG) == 1;
      std::vector<unsigned int> call_points(is_single_call ? ++(call_info->direct_call_points.begin()) :
                                                             call_info->direct_call_points.begin(),
                                            call_info->direct_call_points.end());

      for(auto call_id : call_points)
      {
         const auto call_node = TM->GetTreeNode(call_id);
         const auto module_suffix = "_" + std::to_string(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Clone module " + tsymbol + " -> " + tsymbol + module_suffix);
         tree_man.VersionFunctionCall(call_node, fnode, module_suffix);
         const auto version_symbol = tsymbol + module_suffix;
         const auto version_fnode = TM->GetFunction(version_symbol);
         THROW_ASSERT(version_fnode, "Expected version function node for " + version_symbol);
         new_modules.push_back(version_fnode->index);
         const auto march = FunctionArchitectureRef(new FunctionArchitecture(*tarch));
         march->attrs.at(FunctionArchitecture::func_symbol) += module_suffix;
         march->attrs.at(FunctionArchitecture::func_symbol) += module_suffix;
         HLSMgr->module_arch->AddArchitecture(version_symbol, march);
      }
   }
   BOOST_FOREACH(EdgeDescriptor ie, boost::out_edges(f_v, *CG))
   {
      const auto call_info = CG->CGetFunctionEdgeInfo(ie);
      std::vector<unsigned int> call_points(call_info->direct_call_points.begin(), call_info->direct_call_points.end());
      for(auto call_id : call_points)
      {
         const auto call_node = TM->GetTreeNode(call_id);
         CleanVirtuals(TM, call_node);
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(debug_level >= DEBUG_LEVEL_PEDANTIC || parameters->getOption<bool>(OPT_print_dot))
   {
      CGM->CGetCallGraph()->WriteDot("DFcall_graph.dot");
   }

   if(new_modules.size())
   {
      auto root_functions = CGM->GetRootFunctions();
      root_functions.insert(new_modules.begin(), new_modules.end());
      CGM->SetRootFunctions(root_functions);
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }

   return DesignFlowStep_Status::UNCHANGED;
}
