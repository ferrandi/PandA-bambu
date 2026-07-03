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
 *              Copyright (C) 2016-2026 Politecnico di Milano
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
 * @file dead_code_eliminationIPA.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "dead_code_eliminationIPA.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <string>
#include <utility>

dead_code_eliminationIPA::dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManager& dfm,
                                                   const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, DEAD_CODE_ELIMINATION_IPA, dfm, par)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
dead_code_eliminationIPA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DCE_PASS, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BIT_VALUE_OPT, ALL_FUNCTIONS));
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

void dead_code_eliminationIPA::ComputeRelationships(DesignFlowStepSet& relationships,
                                                    const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP && GetStatus() == DesignFlowStep_Status::SUCCESS)
   {
      const auto DFG = design_flow_manager.CGetDesignFlowGraph();
      std::vector<FrontendFlowStepType> step_types = {FrontendFlowStepType::DCE_PASS};
      step_types.push_back(FrontendFlowStepType::BIT_VALUE);
      for(const auto i : fun_id_to_restart)
      {
         for(auto step_type : step_types)
         {
            const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(step_type, i);
            const auto frontend_step = design_flow_manager.GetDesignFlowStep(step_signature);
            THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
            const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
            relationships.insert(design_flow_step);
         }
      }
      for(const auto i : fun_id_to_restartParm)
      {
         const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::PARM2SSA, i);
         const auto frontend_step = design_flow_manager.GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
         const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool dead_code_eliminationIPA::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status dead_code_eliminationIPA::Exec()
{
   if(!AppM->ApplyNewTransformation())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   fun_id_to_restart.clear();
   fun_id_to_restartParm.clear();
   const auto TM = AppM->get_ir_manager();
   const auto& CGM = AppM->CGetCallGraphManager();
   CustomSet<unsigned int> interface_functions;
   {
      const auto top_functions = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
      std::transform(top_functions.begin(), top_functions.end(),
                     std::inserter(interface_functions, interface_functions.end()),
                     [&](const auto& fname) { return TM->GetFunction(fname)->index; });
      const auto addr_funcs = CGM.GetAddressedFunctions();
      interface_functions.insert(addr_funcs.begin(), addr_funcs.end());
   }
   const auto reached_body_fun_ids = CGM.GetReachedBodyFunctions();
   for(const auto f_id : reached_body_fun_ids)
   {
      const auto is_root = interface_functions.find(f_id) != interface_functions.end();
      if(!is_root)
      {
         const auto fu_name = AppM->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->GetFunctionName();
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "-->Analyzing function \"" + fu_name + "\": id = " + STR(f_id));
         const auto fu_node = TM->GetIRNode(f_id);
         auto fd = GetPointerS<function_val_node>(fu_node);
         THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
         if(!fd->list_of_args.empty())
         {
            signature_opt(TM, fd, f_id, reached_body_fun_ids);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function ");
      }
   }
   for(auto& f_id : fun_id_to_restart)
   {
      const auto FB = AppM->GetFunctionBehavior(f_id);
      FB->UpdateBBVersion();
   }
   for(auto& f_id : fun_id_to_restartParm)
   {
      const auto FB = AppM->GetFunctionBehavior(f_id);
      FB->UpdateBBVersion();
   }
   return fun_id_to_restart.empty() && fun_id_to_restartParm.empty() ? DesignFlowStep_Status::UNCHANGED :
                                                                       DesignFlowStep_Status::SUCCESS;
}

bool dead_code_eliminationIPA::signature_opt(const ir_managerRef& TM, function_val_node* fd, unsigned int function_id,
                                             const CustomOrderedSet<unsigned int>& rFunctions)
{
   const auto& parms = fd->list_of_args;
   std::vector<unsigned int> unused_parm_indices;
   {
      auto idx = static_cast<unsigned int>(parms.size() - 1);
      for(auto it = parms.rbegin(); it != parms.rend(); ++it, --idx)
      {
         const auto ssa = AppM->getSSAFromParm(function_id, (*it)->index);
         if(GetPointer<const ssa_node>(TM->GetIRNode(ssa))->CGetUseStmts().empty())
         {
            unused_parm_indices.push_back(idx);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   if(unused_parm_indices.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No unused parameter found");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return false;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Unused parameter indices: " +
                      container_to_string(unused_parm_indices.rbegin(), unused_parm_indices.rend(), ", ", false));
   const auto arg_eraser = [&](std::vector<ir_nodeRef>& arg_list, const ir_nodeRef& call_stmt) {
      for(const auto& idx : unused_parm_indices)
      {
         const auto arg_it = std::next(arg_list.begin(), idx);
         auto ssa = GetPointer<ssa_node>(*arg_it);
         if(ssa)
         {
            THROW_ASSERT(ssa->CGetUseStmts().count(call_stmt),
                         "ssa " + ssa->ToString() + " not used in " + call_stmt->ToString());

            if(ssa->virtual_flag)
            {
               const auto gn = GetPointerS<node_stmt>(call_stmt);
               if(gn->vuses.erase(*arg_it))
               {
                  ssa->RemoveUse(call_stmt);
               }
               if(gn->vovers.erase(*arg_it))
               {
                  ssa->RemoveUse(call_stmt);
               }
            }
            else
            {
               ssa->RemoveUse(call_stmt);
            }
         }
         arg_list.erase(arg_it);
      }
   };
   const auto& CGM = AppM->CGetCallGraphManager();
   const auto& CG = CGM.GetCallGraph();
   const auto function_v = CGM.GetVertex(function_id);

   ir_manipulationRef ir_man(new ir_manipulation(TM, parameters, AppM));
   std::vector<ir_nodeRef> loa = fd->list_of_args;
   std::vector<ir_nodeConstRef> argsT;
   arg_eraser(loa, nullptr);
   std::transform(loa.cbegin(), loa.cend(), std::back_inserter(argsT),
                  [&](const ir_nodeRef& arg) { return ir_helper::CGetType(arg); });
   const auto ftype = ir_man->GetFunctionType(ir_helper::GetFunctionReturnType(fd->type, false), argsT);
   const auto ftype_ptr = ir_man->GetPointerType(ftype);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Erasing unused arguments from call points");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto& ie : CG.in_edges(function_v))
   {
      const auto caller_id = CGM.get_function(ie.m_source);
      if(rFunctions.find(caller_id) != rFunctions.end())
      {
         const auto& fei = CG.CGetEdgeInfo(ie);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Analysing call points from " + ir_helper::GetFunctionName(TM->GetIRNode(caller_id)));
         for(const auto& call_id : fei.direct_call_points)
         {
            const auto call_tn = TM->GetIRNode(call_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before: " + call_tn->ToString());
            ir_nodeRef fn;
            if(call_tn->get_kind() == call_stmt_K)
            {
               auto gc = GetPointerS<call_stmt>(call_tn);
               THROW_ASSERT(gc->args.size() == parms.size(), "");
               fn = gc->fn;
               arg_eraser(gc->args, call_tn);
            }
            else if(call_tn->get_kind() == assign_stmt_K)
            {
               const auto ga = GetPointerS<const assign_stmt>(call_tn);
               auto ce = GetPointer<call_node>(ga->op1);
               fn = ce->fn;
               THROW_ASSERT(ce, "Unexpected call expression: " + ga->op1->get_kind_text());
               THROW_ASSERT(ce->args.size() == parms.size(), "");
               arg_eraser(ce->args, call_tn);
            }
            else
            {
               THROW_UNREACHABLE("Call point statement not handled: " + call_tn->get_kind_text());
            }
            auto ae = GetPointer<addr_node>(fn);
            if(ae)
            {
               ae->type = ftype_ptr;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After : " + call_tn->ToString());
         }
         THROW_ASSERT(fei.indirect_call_points.empty(), "");
         THROW_ASSERT(fei.function_addresses.empty(), "");
         fun_id_to_restart.insert(caller_id);
      }
   }
   fun_id_to_restart.insert(function_id);
   fun_id_to_restartParm.insert(function_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Erasing unused parameters from function signature");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Before: " +
                      ir_helper::PrintType(TM->GetIRNode(function_id), true, false, ir_nodeRef(),
                                           std::make_unique<std_var_pp_functor>(
                                               AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper())));
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto fname = ir_helper::GetFunctionName(TM->GetIRNode(fd->index));
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
   if(func_arch)
   {
      for(auto i : unused_parm_indices)
      {
         const auto& pnode = fd->list_of_args.at(i);
         const auto pname = GetPointer<argument_val_node>(pnode)->name;
         THROW_ASSERT(pname, "Expected parameter name.");
         const auto pname_str = GetPointer<identifier_node>(pname)->strg;
         func_arch->parms.erase(pname_str);
      }
   }
   fd->list_of_args = loa;
   fd->type = ftype;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---After : " +
                      ir_helper::PrintType(TM->GetIRNode(function_id), true, false, ir_nodeRef(),
                                           std::make_unique<std_var_pp_functor>(
                                               AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper())));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return true;
}
