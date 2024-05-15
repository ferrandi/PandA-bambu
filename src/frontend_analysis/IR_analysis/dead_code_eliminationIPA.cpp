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
 *              Copyright (C) 2016-2024 Politecnico di Milano
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
 * @file dead_code_eliminationIPA.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <string>
#include <utility>

dead_code_eliminationIPA::dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                                                   const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, DEAD_CODE_ELIMINATION_IPA, dfm, par)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dead_code_eliminationIPA::~dead_code_eliminationIPA() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
dead_code_eliminationIPA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BIT_VALUE_OPT, ALL_FUNCTIONS));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
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
      const auto DFM = design_flow_manager.lock();
      const auto DFG = DFM->CGetDesignFlowGraph();
      std::vector<FrontendFlowStepType> step_types = {FrontendFlowStepType::DEAD_CODE_ELIMINATION};
      if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
      {
         step_types.push_back(FrontendFlowStepType::BIT_VALUE);
      }
      for(const auto i : fun_id_to_restart)
      {
         for(auto step_type : step_types)
         {
            const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(step_type, i);
            const auto frontend_step = DFM->GetDesignFlowStep(step_signature);
            THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
            const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
            relationships.insert(design_flow_step);
         }
      }
      for(const auto i : fun_id_to_restartParm)
      {
         const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::PARM2SSA, i);
         const auto frontend_step = DFM->GetDesignFlowStep(step_signature);
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
   const auto TM = AppM->get_tree_manager();
   const auto CGM = AppM->CGetCallGraphManager();
   CustomSet<unsigned int> interface_functions;
   {
      const auto top_functions = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
      std::transform(top_functions.begin(), top_functions.end(),
                     std::inserter(interface_functions, interface_functions.end()),
                     [&](const auto& fname) { return TM->GetFunction(fname)->index; });
      const auto addr_funcs = CGM->GetAddressedFunctions();
      interface_functions.insert(addr_funcs.begin(), addr_funcs.end());
   }
   const auto reached_body_fun_ids = CGM->GetReachedBodyFunctions();
   for(const auto f_id : reached_body_fun_ids)
   {
      const auto is_root = interface_functions.find(f_id) != interface_functions.end();
      if(!is_root)
      {
         const auto fu_name = AppM->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_function_name();
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "-->Analyzing function \"" + fu_name + "\": id = " + STR(f_id));
         const auto fu_node = TM->GetTreeNode(f_id);
         auto fd = GetPointerS<function_decl>(fu_node);
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

bool dead_code_eliminationIPA::signature_opt(const tree_managerRef& TM, function_decl* fd, unsigned int function_id,
                                             const CustomOrderedSet<unsigned int>& rFunctions)
{
   const auto& parms = fd->list_of_args;
   std::vector<unsigned int> unused_parm_indices;
   {
      auto idx = static_cast<unsigned int>(parms.size() - 1);
      for(auto it = parms.rbegin(); it != parms.rend(); ++it, --idx)
      {
         const auto ssa = AppM->getSSAFromParm(function_id, (*it)->index);
         if(GetPointer<const ssa_name>(TM->GetTreeNode(ssa))->CGetUseStmts().empty())
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
   const auto arg_eraser = [&](std::vector<tree_nodeRef>& arg_list, const tree_nodeRef& call_stmt) {
      for(const auto& idx : unused_parm_indices)
      {
         const auto arg_it = std::next(arg_list.begin(), idx);
         auto ssa = GetPointer<ssa_name>(*arg_it);
         if(ssa)
         {
            THROW_ASSERT(ssa->CGetUseStmts().count(call_stmt),
                         "ssa " + ssa->ToString() + " not used in " + call_stmt->ToString());

            if(ssa->virtual_flag)
            {
               const auto gn = GetPointerS<gimple_node>(call_stmt);
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
   const auto CGM = AppM->CGetCallGraphManager();
   const auto CG = CGM->CGetCallGraph();
   const auto function_v = CGM->GetVertex(function_id);

   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
   std::vector<tree_nodeRef> loa = fd->list_of_args;
   std::vector<tree_nodeConstRef> argsT;
   arg_eraser(loa, nullptr);
   std::transform(loa.cbegin(), loa.cend(), std::back_inserter(argsT),
                  [&](const tree_nodeRef& arg) { return tree_helper::CGetType(arg); });
   const auto ftype = tree_man->GetFunctionType(tree_helper::GetFunctionReturnType(fd->type, false), argsT);
   const auto ftype_ptr = tree_man->GetPointerType(ftype);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Erasing unused arguments from call points");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   BOOST_FOREACH(EdgeDescriptor ie, boost::in_edges(function_v, *CG))
   {
      const auto caller_id = CGM->get_function(ie.m_source);
      if(rFunctions.find(caller_id) != rFunctions.end())
      {
         const auto fei = CG->CGetFunctionEdgeInfo(ie);
         INDENT_DBG_MEX(
             DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
             "Analysing call points from " +
                 tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(TM->GetTreeNode(caller_id))));
         for(const auto& call_id : fei->direct_call_points)
         {
            const auto call_stmt = TM->GetTreeNode(call_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before: " + call_stmt->ToString());
            tree_nodeRef fn;
            if(call_stmt->get_kind() == gimple_call_K)
            {
               auto gc = GetPointerS<gimple_call>(call_stmt);
               THROW_ASSERT(gc->args.size() == parms.size(), "");
               fn = gc->fn;
               arg_eraser(gc->args, call_stmt);
            }
            else if(call_stmt->get_kind() == gimple_assign_K)
            {
               const auto ga = GetPointerS<const gimple_assign>(call_stmt);
               auto ce = GetPointer<call_expr>(ga->op1);
               fn = ce->fn;
               THROW_ASSERT(ce, "Unexpected call expression: " + ga->op1->get_kind_text());
               THROW_ASSERT(ce->args.size() == parms.size(), "");
               arg_eraser(ce->args, call_stmt);
            }
            else
            {
               THROW_UNREACHABLE("Call point statement not handled: " + call_stmt->get_kind_text());
            }
            auto ae = GetPointer<addr_expr>(fn);
            if(ae)
            {
               ae->type = ftype_ptr;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After : " + call_stmt->ToString());
         }
         THROW_ASSERT(fei->indirect_call_points.empty(), "");
         THROW_ASSERT(fei->function_addresses.empty(), "");
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
                      tree_helper::print_type(TM, function_id, false, true, false, 0U,
                                              var_pp_functorConstRef(new std_var_pp_functor(
                                                  AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()))));
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
   if(func_arch)
   {
      for(auto i : unused_parm_indices)
      {
         const auto& pnode = fd->list_of_args.at(i);
         const auto pname = GetPointer<parm_decl>(pnode)->name;
         THROW_ASSERT(pname, "Expected parameter name.");
         const auto pname_str = GetPointer<identifier_node>(pname)->strg;
         func_arch->parms.erase(pname_str);
      }
   }
   fd->list_of_args = loa;
   fd->type = ftype;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---After : " +
                      tree_helper::print_type(TM, function_id, false, true, false, 0U,
                                              var_pp_functorConstRef(new std_var_pp_functor(
                                                  AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()))));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return true;
}
