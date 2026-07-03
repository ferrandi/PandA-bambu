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
 * @file BitValueIPA.cpp
 *
 * Created on: June 27, 2016
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#include "BitValueIPA.hpp"

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
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

#include <string>
#include <utility>

BitValueIPA::BitValueIPA(const application_managerRef AM, const DesignFlowManager& dfm, const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, BIT_VALUE_IPA, dfm, par),
      BitLatticeManipulator(AM->get_ir_manager(), parameters->get_class_debug_level(GET_CLASS(*this))),
      last_ver_sum(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BitValueIPA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BIT_VALUE, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
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

void BitValueIPA::ComputeRelationships(DesignFlowStepSet& relationships,
                                       const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      const auto DFG = design_flow_manager.CGetDesignFlowGraph();
      for(const auto& i : fun_id_to_restart)
      {
         const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, i);
         const auto frontend_step = design_flow_manager.GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
         const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
      fun_id_to_restart.clear();
      for(const auto& i : fun_id_to_restart_caller)
      {
         const auto step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, i);
         const auto frontend_step = design_flow_manager.GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
         const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
      fun_id_to_restart_caller.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool BitValueIPA::HasToBeExecuted() const
{
   const auto& CGM = AppM->CGetCallGraphManager();
   unsigned int curr_ver_sum = 0;
   for(const auto i : CGM.GetReachedBodyFunctions())
   {
      const auto FB = AppM->CGetFunctionBehavior(i);
      curr_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   return curr_ver_sum > last_ver_sum;
}

DesignFlowStep_Status BitValueIPA::Exec()
{
   THROW_ASSERT(parameters->isOption(OPT_bitvalue_ipa) && parameters->getOption<bool>(OPT_bitvalue_ipa),
                "Bit value IPA should not be executed");
   if(!AppM->ApplyNewTransformation())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }

   BitLatticeManipulator::clear();
   fun_id_to_restart.clear();
   fun_id_to_restart_caller.clear();

   const auto& CGM = AppM->CGetCallGraphManager();
   const auto& cg = CGM.GetCallGraph();
   const auto reached_body_fun_ids = CGM.GetReachedBodyFunctions();
   auto root_fun_ids = CGM.GetRootFunctions();
   const auto addressed_functions = CGM.GetAddressedFunctions();
   root_fun_ids.insert(addressed_functions.begin(), addressed_functions.end());

   /// In case of indirect calls (e.g., pointer to function) no Bit Value IPA can be done.
   CustomUnorderedSet<CallGraph::vertex_descriptor> vertex_subset;
   for(const auto& cvertex : reached_body_fun_ids)
   {
      vertex_subset.insert(CGM.GetVertex(cvertex));
   }
   const auto subgraph = CGM.CGetCallSubGraph(vertex_subset);
   for(const auto& e : subgraph.edges())
   {
      const auto& info = subgraph.CGetEdgeInfo(e);
      if(info.indirect_call_points.size())
      {
         return DesignFlowStep_Status::UNCHANGED;
      }
   }

   // ---- initialization phase ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Initialize data structures");
   for(const auto& fu_id : reached_body_fun_ids)
   {
      const auto fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->GetFunctionName();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Analyzing function \"" + fu_name + "\": id = " + STR(fu_id));
      const auto fu_node = TM->GetIRNode(fu_id);
      const auto fd = GetPointerS<function_val_node>(fu_node);
      THROW_ASSERT(fd->body, "Node is not a function or it hasn't a body");
      const auto fu_type = ir_helper::CGetType(fu_node);
      THROW_ASSERT(fu_type->get_kind() == function_ty_node_K, "node " + STR(fu_id) + " is " + fu_type->get_kind_text());
      const auto ft = GetPointerS<const function_ty_node>(fu_type);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "is_root = " + STR(root_fun_ids.find(fu_id) != root_fun_ids.end()));

      // -- process parameters --
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing parameters");
      for(const auto& parm_decl_node : fd->list_of_args)
      {
         const auto p_decl_id = AppM->getSSAFromParm(fu_id, parm_decl_node->index);
         const auto parmssa = TM->GetIRNode(p_decl_id);
         THROW_ASSERT(parmssa->get_kind() == ssa_node_K, "expected an ssa variable");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "argument_val_node ssa id: " + STR(p_decl_id) + " " + parmssa->ToString());
         const auto p = GetPointerS<ssa_node>(parmssa);
         if(!IsHandledByBitvalue(p->type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "parameter type is not considered: " + STR(p_decl_id));
            continue;
         }
         THROW_ASSERT(!p->bit_values.empty(),
                      "unexpected condition " + parmssa->ToString() + " for function " + fu_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "found bitvalue: " + p->bit_values);
         best[p_decl_id] = string_to_bitstring(p->bit_values);
         if(ir_helper::IsSignedIntegerType(p->type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
            signed_var.insert(p_decl_id);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed parameters");

      // -- process function returned value --
      if(!IsHandledByBitvalue(ft->retn) || !ir_helper::IsScalarType(ft->retn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--function return type is not considered: " + STR(ft->retn));
         continue;
      }
      THROW_ASSERT(!fd->bit_values.empty(), "not expected");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "found bitvalue: " + fd->bit_values);
      best[fu_id] = string_to_bitstring(fd->bit_values);

      if(ir_helper::IsSignedIntegerType(ft->retn))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
         signed_var.insert(fu_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function \"" + fu_name + "\": id = " + STR(fu_id));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Initialized data structures");

   // ---- propagation phase ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Start BitValueIPA propagation");
   for(const auto& fu_id : reached_body_fun_ids)
   {
      const auto fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->GetFunctionName();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Analyzing function \"" + fu_name + "\": id = " + STR(fu_id));

      const auto fu_node = TM->GetIRNode(fu_id);
      const auto fd = GetPointerS<const function_val_node>(fu_node);
      THROW_ASSERT(fd->body, "Node is not a function or it hasn't a body");
      const auto fu_type = ir_helper::CGetType(fu_node);
      THROW_ASSERT(fu_type->get_kind() == function_ty_node_K, "node " + STR(fu_id) + " is " + fu_type->get_kind_text());
      const auto ft = GetPointerS<const function_ty_node>(fu_type);
      const auto fret_type_node = ft->retn;

      if(root_fun_ids.find(fu_id) == root_fun_ids.end())
      {
         // --- propagation through return values ---
         if(best.find(fu_id) != best.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "-->Propagating bitvalue of the return value of function " + fu_name);

            /*
             * for root functions, don't perform backward propagation from assigned
             * ssa to returned value, because this could lead to unsafe
             * optimizations if some external piece of code that was not
             * synthesized with bambu calls the top function from the bus
             */
            // --- backward ----

            if(!AppM->ApplyNewTransformation())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               break;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Backward");

            current.insert(std::make_pair(fu_id, best.at(fu_id)));

            auto res = create_x_bitstring(1);

            const auto fu_cgv = CGM.GetVertex(fu_id);
            for(const auto& ie : cg.in_edges(fu_cgv))
            {
               const auto caller_id = CGM.get_function(cg.source(ie));
               if(reached_body_fun_ids.find(caller_id) == reached_body_fun_ids.cend())
               {
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "-->examining caller \"" +
                                  AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->GetFunctionName() +
                                  "\": id = " + STR(caller_id));
               const auto& call_edge_info = cg.CGetEdgeInfo(ie);
               for(const auto& i : call_edge_info.direct_call_points)
               {
                  THROW_ASSERT(i, "unexpected condition");
                  const auto call_stmt_node = TM->GetIRNode(i);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "-->examining direct call point " + call_stmt_node->ToString());
                  if(call_stmt_node->get_kind() == assign_stmt_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "assign_stmt");
                     const auto ga = GetPointerS<const assign_stmt>(call_stmt_node);
                     THROW_ASSERT(ga, STR(i) + " is not an assign statement");
                     if(ga->op0->get_kind() == ssa_node_K)
                     {
                        THROW_ASSERT(ga->op1->get_kind() == call_node_K,
                                     ga->op1->ToString() + " kind = " + ga->op1->get_kind_text());
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "assigns ssa_node");
                        const auto s = GetPointerS<const ssa_node>(ga->op0);
                        THROW_ASSERT(IsHandledByBitvalue(ga->op0), "ssa is not handled by bitvalue");
                        THROW_ASSERT(
                            ir_helper::IsSignedIntegerType(ga->op0) == static_cast<bool>(signed_var.count(fu_id)),
                            "function " +
                                AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->GetFunctionName() +
                                " calls function " + fu_name +
                                " with return type = " + STR(ir_helper::GetFunctionReturnType(fu_node)) +
                                " and assigns the return value to ssa " + STR(s) +
                                " of type = " + STR(ir_helper::CGetType(ga->op0)) + "\ndifferent signedness!");
                        THROW_ASSERT(!s->bit_values.empty(), "unexpected assignment of return value to ssa " + STR(s) +
                                                                 " with id " + STR(s->index) + " and empty bit_values");
                        const auto res_fanout = string_to_bitstring(s->bit_values);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "res_fanout from ssa " + STR(s) + " id: " + STR(s->index) +
                                           " bitstring: " + bitstring_to_string(res_fanout));
                        THROW_ASSERT(res_fanout.size(), "unexpected condition");
                        res = inf(res, res_fanout, fu_node);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "fu_id: " + STR(fu_id) + " bitstring: " + bitstring_to_string(res));
                        auto res_sup = sup(best.at(fu_id), res_fanout, fu_node);
                        auto res_sup_string = bitstring_to_string(res_sup);
                        if(isBetter(res_sup_string, s->bit_values))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "" + STR(fu_id) + " " + bitstring_to_string(best.at(fu_id)) +
                                              " is better than: " + s->bit_values);
                           fun_id_to_restart_caller.insert(caller_id);
                        }
                     }
                     else
                     {
                        THROW_UNREACHABLE(STR(ga->op0) +
                                          ": the assigned value is not an ssa_node: " + ga->op0->get_kind_text());
                     }
                  }
                  else if(call_stmt_node->get_kind() == call_stmt_K)
                  {
                     // do nothing
                  }
                  else
                  {
                     THROW_ERROR("unexpected condition " + call_stmt_node->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined call point " + STR(i));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "<--examined caller \"" +
                                  AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->GetFunctionName() +
                                  "\": id = " + STR(caller_id));
            }

            update_current(res, fu_node);

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Backward done");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---After backward id: " + STR(fu_id) +
                               " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));

            mix();
            current.clear();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---After mix id: " + STR(fu_id) +
                               " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));

            AppM->RegisterTransformation(GetName(), fu_node);

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "<--Propagated bitvalue of the return value of function " + fu_name);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Return value function " + fu_name + " not handled bitvalue");
         }

         // --- propagation through parameters ---
         int args_n = 0;
         for(const auto& pd : fd->list_of_args)
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            const auto pd_id = AppM->getSSAFromParm(fu_id, pd->index);
            const auto parmssa = TM->GetIRNode(pd_id);
            THROW_ASSERT(parmssa->get_kind() == ssa_node_K, "expected an ssa variable");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "argument_val_node ssa id: " + STR(pd_id) + " " + parmssa->ToString());

            args_n++;
            if(best.find(pd_id) != best.cend())
            {
               THROW_ASSERT(IsHandledByBitvalue(parmssa),
                            "param \"" + STR(pd) + "\" id: " + STR(pd_id) + " not handled by bitvalue");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "-->Propagating bitvalue through parameter " + STR(pd) + " of function " + fu_name +
                                  " parm id: " + STR(pd_id));

               /*
                * for root functions, don't perform forward propagation from actual
                * parameters to formal parameters, because this could lead to unsafe
                * optimizations if some external piece of code that was not
                * synthesized with bambu calls the top function from the bus
                */
               // --- forward ---
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Forward");
                  const auto parm_signed = signed_var.find(pd_id) != signed_var.cend();

                  current.insert(std::make_pair(pd_id, best.at(pd_id)));

                  auto res = create_x_bitstring(1);

                  const auto fu_cgv = CGM.GetVertex(fu_id);
                  for(const auto& ie : cg.in_edges(fu_cgv))
                  {
                     const auto caller_id = CGM.get_function(cg.source(ie));
                     if(reached_body_fun_ids.find(caller_id) == reached_body_fun_ids.cend())
                     {
                        continue;
                     }
                     const auto caller_name =
                         AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->GetFunctionName();
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "-->examining caller \"" + caller_name + "\": id = " + STR(caller_id));
                     const auto& call_edge_info = cg.CGetEdgeInfo(ie);
                     for(const auto& i : call_edge_info.direct_call_points)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->examining direct call point " + STR(i));
                        std::deque<bit_lattice> res_tmp;
                        THROW_ASSERT(i, "unexpected condition");
                        const auto call_stmt_node = TM->GetIRNode(i);
                        if(call_stmt_node->get_kind() == assign_stmt_K)
                        {
                           const auto ga = GetPointerS<const assign_stmt>(call_stmt_node);
                           THROW_ASSERT(ga->op1->get_kind() == call_node_K, "unexpected pattern");

                           const auto ce = GetPointerS<const call_node>(ga->op1);
                           const auto actual_parms = ce->args;
                           THROW_ASSERT(ce->args.size() == fd->list_of_args.size(),
                                        "actual parameters: " + STR(ce->args.size()) +
                                            " formal parameters: " + STR(fd->list_of_args.size()));
                           const auto ap = std::next(ce->args.cbegin(), args_n - 1);
                           const auto ap_id = (*ap)->index;
                           const auto& ap_node = *ap;
                           const auto ap_kind = ap_node->get_kind();
                           THROW_ASSERT(IsHandledByBitvalue(ap_node), "actual parameter not handled by bitvalue");
                           THROW_ASSERT(ir_helper::IsSignedIntegerType(ap_node) == parm_signed,
                                        "function " + caller_name + " calls function " + fu_name + "\nformal param " +
                                            STR(pd) + " type = " + STR(ir_helper::CGetType(pd)) + "\nactual param " +
                                            STR(ap_node) + " type = " + STR(ir_helper::CGetType(ap_node)) +
                                            "\ndifferent signedness!");
                           if(ap_kind == ssa_node_K)
                           {
                              const auto ssa = GetPointerS<const ssa_node>(ap_node);
                              res_tmp = ssa->bit_values.empty() ? create_u_bitstring(ir_helper::TypeSize(ap_node)) :
                                                                  string_to_bitstring(ssa->bit_values);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "actual parameter " + STR(ssa) + " id: " + STR(ssa->index) +
                                                 " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else if(ap_kind == constant_int_val_node_K)
                           {
                              const auto cst_val = ir_helper::GetConstValue(ap_node);
                              res_tmp =
                                  create_bitstring_from_constant(cst_val, ir_helper::TypeSize(ap_node), parm_signed);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "actual parameter " + STR(ap_node) + " is a constant value id: " +
                                                 STR(ap_node->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else
                           {
                              THROW_UNREACHABLE("unexpected actual parameter " + STR(ap_node) + " id : " + STR(ap_id) +
                                                " of kind: " + ir_node::GetString(ap_kind));
                           }
                        }
                        else if(call_stmt_node->get_kind() == call_stmt_K)
                        {
                           const auto gc = GetPointerS<const call_stmt>(call_stmt_node);
                           const auto actual_parms = gc->args;
                           THROW_ASSERT(gc->args.size() == fd->list_of_args.size(),
                                        "actual parameters: " + STR(gc->args.size()) +
                                            " formal parameters: " + STR(fd->list_of_args.size()));
                           const auto ap = std::next(gc->args.cbegin(), args_n - 1);
                           const auto ap_id = (*ap)->index;
                           const auto& ap_node = *ap;
                           const auto ap_kind = ap_node->get_kind();
                           THROW_ASSERT(IsHandledByBitvalue(ap_node), "actual parameter not handled by bitvalue");
                           THROW_ASSERT(ir_helper::IsSignedIntegerType(ap_node) == parm_signed,
                                        "function " + caller_name + " calls function " + fu_name + "\nformal param " +
                                            STR(pd) + " type = " + STR(ir_helper::CGetType(pd)) + "\nactual param " +
                                            STR(ap_node) + " type = " + STR(ir_helper::CGetType(ap_node)) +
                                            "\ndifferent signedness!");
                           if(ap_kind == ssa_node_K)
                           {
                              const auto ssa = GetPointerS<const ssa_node>(ap_node);
                              res_tmp = ssa->bit_values.empty() ? create_u_bitstring(ir_helper::TypeSize(ap_node)) :
                                                                  string_to_bitstring(ssa->bit_values);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "actual parameter " + STR(ssa) + " id: " + STR(ssa->index) +
                                                 " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else if(ap_kind == constant_int_val_node_K)
                           {
                              const auto cst_val = ir_helper::GetConstValue(ap_node);
                              res_tmp =
                                  create_bitstring_from_constant(cst_val, ir_helper::TypeSize(ap_node), parm_signed);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "actual parameter " + STR(ap_node) + " is a constant value id: " +
                                                 STR(ap_node->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else
                           {
                              THROW_UNREACHABLE("unexpected actual parameter " + STR(ap_node) + " id : " + STR(ap_id) +
                                                " of kind: " + ir_node::GetString(ap_kind));
                           }
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "this call point is not in the form (ssa_node = call_node)\n"
                                          "no way to retrieve the actual parameters of the call");
                           THROW_UNREACHABLE("unexpected pattern: function " + fu_name + " is called by " +
                                             caller_name + " in operation " + STR(call_stmt_node));
                        }
                        res = inf(res, res_tmp, parmssa);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "param id: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined call point " + STR(i));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "<--examined caller \"" + caller_name + "\": id = " + STR(caller_id));
                  }

                  update_current(res, parmssa);

                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Forward done");
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---After forward id: " + STR(pd_id) +
                                     " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));

                  mix();
                  current.clear();
                  AppM->RegisterTransformation(GetName(), pd);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---After mix id: " + STR(pd_id) +
                                     " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "<--Propagated bitvalue through parameter " + STR(pd) + " of function " + fu_name +
                                  " parm id: " + STR(pd_id));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "Parameter " + STR(pd) + " of function " + fu_name + " not handled by bitvalue");
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function \"" + fu_name + "\": id = " + STR(fu_id));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--End BitValueIPA propagation");

   // ---- update bivalues on IR ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating IR");
   for(const auto& b : best)
   {
      const auto& tn_id = b.first;
      const auto new_bitvalue = bitstring_to_string(b.second);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---updating node id: " + STR(tn_id) + " bitstring: " + new_bitvalue);
      const auto tn = TM->GetIRNode(tn_id);
      const auto kind = tn->get_kind();

      std::string null_string = "";
      std::string* old_bitvalue = &null_string;
      unsigned long long size = 0u;
      auto restart_fun_id = 0u;
      if(kind == function_val_node_K)
      {
         auto fd = GetPointerS<function_val_node>(tn);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---is a function_val_node: " +
                            AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()->GetFunctionName() +
                            " id: " + STR(fd->index));
         THROW_ASSERT(fd->body, "has no body");
         old_bitvalue = &fd->bit_values;
         const auto fu_type = ir_helper::CGetType(tn);
         THROW_ASSERT(fu_type->get_kind() == function_ty_node_K,
                      "node " + STR(tn_id) + " is " + fu_type->get_kind_text());
         const auto ft = GetPointerS<const function_ty_node>(fu_type);
         const auto fret_type_node = ft->retn;
         size = ir_helper::TypeSize(fret_type_node);
         restart_fun_id = fd->index;
      }
      else if(kind == ssa_node_K)
      {
         auto pd = GetPointerS<ssa_node>(tn);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---is a argument_val_node: " + STR(pd) + " id: " + STR(pd->index));
         old_bitvalue = &pd->bit_values;
         size = ir_helper::TypeSize(tn);
         THROW_ASSERT(pd->var && pd->var->get_kind() == argument_val_node_K, "unexpected pattern");
         const auto pdecl = GetPointerS<const argument_val_node>(pd->var);
         restart_fun_id = pdecl->parent->index;
      }
      else
      {
         THROW_UNREACHABLE("unexpected condition: variable of kind " + ir_node::GetString(kind));
      }

      bool restart = false;
      if(old_bitvalue->empty())
      {
         const auto full_bv = bitstring_to_string(create_u_bitstring(size));
         if(isBetter(new_bitvalue, full_bv))
         {
            restart = true;
         }
      }
      else if(isBetter(new_bitvalue, *old_bitvalue))
      {
         restart = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---old bitstring: " + *old_bitvalue + " new bitstring: " + new_bitvalue +
                         " restart = " + (restart ? "YES" : "NO") + " " +
                         AppM->CGetFunctionBehavior(restart_fun_id)->CGetBehavioralHelper()->GetFunctionName());
      *old_bitvalue = new_bitvalue;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---updated best id: " + STR(tn_id) + " bitstring: " + *old_bitvalue);

      if(restart)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "restart function " +
                            AppM->CGetFunctionBehavior(restart_fun_id)->CGetBehavioralHelper()->GetFunctionName());
         fun_id_to_restart.insert(restart_fun_id);
         const auto fu_cgv = CGM.GetVertex(restart_fun_id);
         for(const auto& ie : cg.in_edges(fu_cgv))
         {
            const auto caller_id = CGM.get_function(cg.source(ie));
            if(reached_body_fun_ids.find(caller_id) == reached_body_fun_ids.cend())
            {
               continue;
            }
            const auto& call_edge_info = cg.CGetEdgeInfo(ie);
            if(!call_edge_info.direct_call_points.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "restart caller " +
                                  AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->GetFunctionName());
               fun_id_to_restart_caller.insert(caller_id);
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Updated IR");

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "Functions to restart: " + STR(fun_id_to_restart.size() + fun_id_to_restart_caller.size()));
   BitLatticeManipulator::clear();

   for(const auto& i : fun_id_to_restart)
   {
      const auto FB = AppM->GetFunctionBehavior(i);
      FB->UpdateBitValueVersion();
   }
   for(const auto& i : fun_id_to_restart_caller)
   {
      const auto FB = AppM->GetFunctionBehavior(i);
      FB->UpdateBitValueVersion();
   }

   last_ver_sum = 0;
   for(const auto& i : CGM.GetReachedBodyFunctions())
   {
      const auto FB = AppM->CGetFunctionBehavior(i);
      last_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   return fun_id_to_restart.empty() ? DesignFlowStep_Status::UNCHANGED : DesignFlowStep_Status::SUCCESS;
}
