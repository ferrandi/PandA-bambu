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
 *              Copyright (C) 2024-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file dataflow_cg_ext.cpp
 * @brief Dataflow call graph extension
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "dataflow_cg_ext.hpp"

#include "DCE.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"

dataflow_cg_ext::dataflow_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
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

static void CleanVirtuals(const ir_managerRef& TM, const ir_nodeRef& call_stmt)
{
   const auto gn = GetPointerS<node_stmt>(call_stmt);
   if(gn->vdef)
   {
      DCE::kill_vdef(TM, gn->vdef);
      gn->vdef = nullptr;
   }
   std::for_each(gn->vuses.begin(), gn->vuses.end(), [&](auto& it) { GetPointer<ssa_node>(it)->RemoveUse(call_stmt); });
   gn->vuses.clear();
   std::for_each(gn->vovers.begin(), gn->vovers.end(),
                 [&](auto& it) { GetPointer<ssa_node>(it)->RemoveUse(call_stmt); });
   gn->vovers.clear();
   THROW_ASSERT(!gn->memdef && !gn->memuse, "Unexpected condition");
}

DesignFlowStep_Status dataflow_cg_ext::InternalExec()
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto fsymbol = function_behavior->CGetBehavioralHelper()->GetFunctionName();
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
   const auto is_dataflow_top =
       func_arch && func_arch->attrs.find(FunctionArchitecture::func_dataflow_top) != func_arch->attrs.end() &&
       func_arch->attrs.find(FunctionArchitecture::func_dataflow_top)->second == "1";
   if(!is_dataflow_top)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   const auto TM = AppM->get_ir_manager();
   auto& CGM = AppM->GetCallGraphManager();
   const auto& CG = CGM.GetCallGraph();
   const auto f_v = CGM.GetVertex(function_id);
   /// Top has to be function pipelined
   const auto fTopnode = TM->GetIRNode(function_id);
   THROW_ASSERT(fTopnode->get_kind() == function_val_node_K, "unexpected condition");
   auto fdT = GetPointerS<function_val_node>(fTopnode);
   fdT->set_pipelining(true);
   fdT->set_pipeline_style(function_val_node::FRP_STYLE);
   function_behavior->enable_function_pipelining();
   function_behavior->disable_stp();

   ir_manipulation ir_man(TM, parameters, AppM);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Expand Dataflow modules");
   std::vector<std::tuple<CallGraph::vertex_descriptor, std::string, FunctionArchitectureRef,
                          const CustomOrderedSet<unsigned int>, const std::vector<unsigned int>>>
       tgt_vertices;
   for(const auto& ie : CG.out_edges(f_v))
   {
      auto tgt = CG.target(ie);
      const auto target_id = CGM.get_function(tgt);
      const auto tsymbol = AppM->CGetFunctionBehavior(target_id)->CGetBehavioralHelper()->GetFunctionName();
      const auto tarch = HLSMgr->module_arch->GetArchitecture(tsymbol);
      const auto is_dataflow_module =
          tarch && tarch->attrs.find(FunctionArchitecture::func_dataflow_module) != tarch->attrs.end() &&
          tarch->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1";
      if(!is_dataflow_module)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function " + tsymbol + " is not a dataflow module");
         continue;
      }
      const auto& call_info = CG.CGetEdgeInfo(ie);
      const auto call_direct_point = call_info.direct_call_points;
      if(call_info.function_addresses.size() || call_info.indirect_call_points.size())
      {
         THROW_ERROR("Address/indirect function calls not supported in dataflow.");
      }
      auto is_single_call = CG.in_degree(tgt) == 1;
      std::vector<unsigned int> call_points(
          (is_single_call ? ++(call_direct_point.begin()) : call_direct_point.begin()), call_direct_point.end());
      tgt_vertices.emplace_back(tgt, tsymbol, tarch, call_direct_point, call_points);
   }
   std::set<unsigned int> new_modules;
   for(const auto& [tgt, tsymbol, tarch, call_direct_point, call_points] : tgt_vertices)
   {
      for(auto call_id : call_direct_point)
      {
         const auto call_node = TM->GetIRNode(call_id);
         CleanVirtuals(TM, call_node);
      }
      for(auto call_id : call_points)
      {
         const auto call_node = TM->GetIRNode(call_id);
         const auto module_suffix = "_" + std::to_string(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Clone module " + tsymbol + " -> " + tsymbol + module_suffix);
         ir_man.VersionFunctionCall(call_node, fTopnode, module_suffix);
         const auto version_symbol = tsymbol + module_suffix;
         const auto version_fnode = TM->GetFunction(version_symbol);
         THROW_ASSERT(version_fnode, "Expected version function node for " + version_symbol);
         new_modules.insert(version_fnode->index);
         const auto march = FunctionArchitectureRef(new FunctionArchitecture(*tarch));
         march->attrs.at(FunctionArchitecture::func_symbol) += module_suffix;
         march->attrs.at(FunctionArchitecture::func_symbol) += module_suffix;
         HLSMgr->module_arch->AddArchitecture(version_symbol, march);
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   if(debug_level >= DEBUG_LEVEL_PEDANTIC || parameters->getOption<bool>(OPT_print_dot))
   {
      CG.writeDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "DFcall_graph.dot");
   }

   if(new_modules.size())
   {
      auto root_functions = CGM.GetRootFunctions();
      root_functions.insert(new_modules.begin(), new_modules.end());
      CGM.SetRootFunctions(root_functions);
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }

   return DesignFlowStep_Status::UNCHANGED;
}
