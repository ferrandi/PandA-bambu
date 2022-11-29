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
 *              Copyright (C) 2016-2022 Politecnico di Milano
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

// include class header
#include "dead_code_eliminationIPA.hpp"

// include from src/
#include "Parameter.hpp"

// include from src/behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include from src/design_flow/
#include "application_manager.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

// include from src/frontend_analysis/
#include "function_frontend_flow_step.hpp"

#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

/// STD include
#include <string>
#include <utility>

dead_code_eliminationIPA::dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                                                   const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, DEAD_CODE_ELIMINATION_IPA, dfm, par)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dead_code_eliminationIPA::~dead_code_eliminationIPA() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
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
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
      {
         for(const auto i : fun_id_to_restart)
         {
            const std::string step_signature =
                FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, i);
            vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
            THROW_ASSERT(frontend_step != NULL_VERTEX, "step " + step_signature + " is not present");
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const DesignFlowStepRef design_flow_step =
                design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
            relationships.insert(design_flow_step);
         }
      }
      for(const auto i : fun_id_to_restartParm)
      {
         const std::string step_signature =
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::PARM2SSA, i);
         vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != NULL_VERTEX, "step " + step_signature + " is not present");
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const DesignFlowStepRef design_flow_step =
             design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
      fun_id_to_restart.clear();
      fun_id_to_restartParm.clear();
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
   const auto CGMan = AppM->CGetCallGraphManager();
   const auto reached_body_fun_ids = CGMan->GetReachedBodyFunctions();
   for(const auto fu_id : reached_body_fun_ids)
   {
      const auto is_root = AppM->CGetCallGraphManager()->GetRootFunctions().count(fu_id) ||
                           AppM->CGetCallGraphManager()->GetAddressedFunctions().count(fu_id);
      if(!is_root)
      {
         const auto fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->get_function_name();
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "-->Analyzing function \"" + fu_name + "\": id = " + STR(fu_id));
         const auto fu_node = TM->GetTreeNode(fu_id);
         auto fd = GetPointerS<function_decl>(fu_node);
         THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
         if(!fd->list_of_args.empty())
         {
            signature_opt(TM, fd, fu_id, reached_body_fun_ids);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function ");
      }
   }
   return fun_id_to_restart.empty() && fun_id_to_restartParm.empty() ? DesignFlowStep_Status::UNCHANGED :
                                                                       DesignFlowStep_Status::SUCCESS;
}

bool dead_code_eliminationIPA::signature_opt(const tree_managerRef& TM, function_decl* fd, unsigned int function_id,
                                             const CustomOrderedSet<unsigned int>& rFunctions)
{
   const auto& args = fd->list_of_args;
   std::vector<tree_nodeConstRef> real_parm(args.size(), nullptr);
   bool binding_completed = false;

   const auto parm_bind = [&](const tree_nodeRef& stmt) -> void {
      const auto ssa_uses = tree_helper::ComputeSsaUses(stmt);
      for(const auto& use : ssa_uses)
      {
         const auto SSA = GetPointer<const ssa_name>(GET_CONST_NODE(use.first));
         // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function
         // parameter inside the function body
         if(SSA->var != nullptr && GET_CONST_NODE(SSA->var)->get_kind() == parm_decl_K &&
            GET_CONST_NODE(SSA->CGetDefStmt())->get_kind() == gimple_nop_K)
         {
            auto argIt = std::find_if(args.begin(), args.end(), [&](const tree_nodeRef& arg) {
               return GET_INDEX_CONST_NODE(arg) == GET_INDEX_CONST_NODE(SSA->var);
            });
            THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
            size_t arg_pos = static_cast<size_t>(argIt - args.begin());
            if(real_parm[arg_pos] != nullptr)
            {
               THROW_ASSERT(SSA->index == GET_INDEX_CONST_NODE(real_parm[arg_pos]), "");
               continue;
            }
            THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Parameter " + STR(arg_pos) + "(" + GET_CONST_NODE(*argIt)->ToString() +
                               ") is binded to ssa variable " + SSA->ToString());
            real_parm[arg_pos] = use.first;
            binding_completed = std::find(real_parm.begin(), real_parm.end(), nullptr) == real_parm.end();
         }
      }
   };

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Signature parameter lookup started...");
   const auto sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
   for(const auto& bb : sl->list_of_bloc)
   {
      for(const auto& phi : bb.second->CGetPhiList())
      {
         parm_bind(phi);
         if(binding_completed)
         {
            break;
         }
      }
      if(binding_completed)
      {
         break;
      }

      for(const auto& stmt : bb.second->CGetStmtList())
      {
         parm_bind(stmt);
         if(binding_completed)
         {
            break;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Signature parameter lookup completed");
   if(binding_completed)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No unused parameter found");
      return false;
   }

   const auto unused_arg_index = [&]() {
      std::vector<unsigned int> uai;
      for(auto i = static_cast<unsigned int>(real_parm.size()); i > 0;)
      {
         const auto index = --i;
         if(real_parm.at(index) == nullptr)
         {
            uai.push_back(index);
         }
      }
      return uai;
   }();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Unused parameter indexes: " + convert_vector_to_string(unused_arg_index, ", ", false));
   const auto arg_eraser = [&](std::vector<tree_nodeRef>& arg_list, const tree_nodeRef& call_stmt) {
      for(const auto& uai : unused_arg_index)
      {
         const auto arg_it = arg_list.begin() + uai;
         auto ssa = GetPointer<ssa_name>(GET_NODE(*arg_it));
         if(ssa)
         {
            THROW_ASSERT(ssa->CGetUseStmts().count(call_stmt),
                         "ssa " + ssa->ToString() + " not used in " + call_stmt->ToString());

            if(ssa->virtual_flag)
            {
               const auto gn = GetPointerS<gimple_node>(GET_NODE(call_stmt));
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

   InEdgeIterator ie, ie_end;
   tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
   std::vector<tree_nodeRef> loa = fd->list_of_args;
   std::vector<tree_nodeConstRef> argsT;
   arg_eraser(loa, nullptr);
   std::transform(loa.cbegin(), loa.cend(), std::back_inserter(argsT),
                  [&](const tree_nodeRef& arg) { return tree_helper::CGetType(arg); });
   const auto ftype = tree_man->GetFunctionType(tree_helper::GetFunctionReturnType(fd->type), argsT);
   const auto ftype_ptr = tree_man->GetPointerType(ftype);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Erasing unused arguments from call points");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(boost::tie(ie, ie_end) = boost::in_edges(function_v, *CG); ie != ie_end; ie++)
   {
      const auto caller_id = CGM->get_function(ie->m_source);
      if(rFunctions.find(caller_id) != rFunctions.end())
      {
         const auto fei = CG->CGetFunctionEdgeInfo(*ie);
         INDENT_DBG_MEX(
             DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
             "Analysing call points from " +
                 tree_helper::print_function_name(TM, GetPointerS<const function_decl>(TM->CGetTreeNode(caller_id))));
         for(const auto& call_id : fei->direct_call_points)
         {
            auto call_rdx = TM->GetTreeReindex(call_id);
            auto call_stmt = GET_NODE(call_rdx);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before erase: " + call_stmt->ToString());
            tree_nodeRef fn;
            if(call_stmt->get_kind() == gimple_call_K)
            {
               auto gc = GetPointerS<gimple_call>(call_stmt);
               THROW_ASSERT(gc->args.size() == args.size(), "");
               fn = gc->fn;
               arg_eraser(gc->args, call_rdx);
            }
            else if(call_stmt->get_kind() == gimple_assign_K)
            {
               const auto ga = GetPointerS<const gimple_assign>(call_stmt);
               auto ce = GetPointer<call_expr>(GET_NODE(ga->op1));
               fn = ce->fn;
               THROW_ASSERT(ce, "Unexpected call expression: " + GET_NODE(ga->op1)->get_kind_text());
               THROW_ASSERT(ce->args.size() == args.size(), "");
               arg_eraser(ce->args, call_rdx);
            }
            else
            {
               THROW_UNREACHABLE("Call point statement not handled: " + call_stmt->get_kind_text());
            }
            auto ae = GetPointer<addr_expr>(GET_NODE(fn));
            if(ae)
            {
               ae->type = ftype_ptr;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After erase: " + call_stmt->ToString());
         }
         THROW_ASSERT(fei->indirect_call_points.empty(), "");
         THROW_ASSERT(fei->function_addresses.empty(), "");
         fun_id_to_restart.insert(caller_id);
      }
   }
   fun_id_to_restart.insert(function_id);
   fun_id_to_restartParm.insert(function_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Erasing parameters from function signature");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Before erase: " +
                      tree_helper::print_type(TM, function_id, false, true, false, 0U,
                                              var_pp_functorConstRef(new std_var_pp_functor(
                                                  AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()))));
   fd->list_of_args = loa;
   fd->type = ftype;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---After erase: " +
                      tree_helper::print_type(TM, function_id, false, true, false, 0U,
                                              var_pp_functorConstRef(new std_var_pp_functor(
                                                  AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()))));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Function signature optimization completed");
   return true;
}
