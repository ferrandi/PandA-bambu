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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file Bit_Value.cpp
 * @brief Full implementation of Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 * This technical report is an extension of the following paper:
 * Mihai Budiu, Majd Sakr, Kip Walker, Seth Copen Goldstein: BitValue Inference: Detecting and Exploiting Narrow
 * Bitwidth Computations. Euro-Par 2000: 969-979
 *
 * Created on: May 27, 2014
 *
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietro.fezzardi@gmail.com>
 *
 */
#include "Bit_Value.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "Range.hpp"
#include "SemiNCADominance.hpp"
#include "application_frontend_flow_step.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include <cmath>
#include <deque>
#include <fstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

Bit_Value::Bit_Value(const ParameterConstRef params, const application_managerRef AM, unsigned int f_id,
                     const DesignFlowManager& dfm)
    : FunctionFrontendFlowStep(AM, f_id, BIT_VALUE, dfm, params),
      BitLatticeManipulator(AM->get_ir_manager(), parameters->get_class_debug_level(GET_CLASS(*this))),
      not_frontend(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
Bit_Value::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BIT_VALUE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, SAME_FUNCTION));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
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
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool Bit_Value::HasToBeExecuted() const
{
   return (bitvalue_version != function_behavior->GetBitValueVersion()) or FunctionFrontendFlowStep::HasToBeExecuted();
}

void Bit_Value::Initialize()
{
   const auto bambu_frontend_flow_signature =
       ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BAMBU_FRONTEND_FLOW);
   not_frontend = design_flow_manager.GetStatus(bambu_frontend_flow_signature) == DesignFlowStep_Status::EMPTY;

   const auto tn = TM->GetIRNode(function_id);
   THROW_ASSERT(tn->get_kind() == function_val_node_K, "Node is not a function");
   const auto fd = GetPointerS<const function_val_node>(tn);
   THROW_ASSERT(fd->body, "Function has not a body");
   const auto sl = GetPointerS<const statement_list_node>(fd->body);
   /// store the IR BB graph ala boost::graph
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map.insert(std::make_pair(block.first, bb_graphs_collection.AddVertex(BBNodeInfo(block.second))));
   }

   /// add edges
   for(const auto& curr_bb_pair : sl->list_of_bloc)
   {
      const auto curr_bbi = curr_bb_pair.first;
      const auto curr_bb = curr_bb_pair.second;
      for(const auto& lop : curr_bb->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)),
                      "BB" + STR(lop) + " (successor of BB" + STR(curr_bbi) + ") does not exist");
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bbi), CFG_SELECTOR);
      }

      for(const auto& los : curr_bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(curr_bb->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                      CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection.AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                CFG_SELECTOR);
   dominance<BBGraph> bb_dominators(cfg, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                    inverse_vertex_map.at(bloc::EXIT_BLOCK_ID));

   BBGraph dt(bb_graphs_collection, D_SELECTOR);
   bb_dominators.forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID))
          {
             bb_graphs_collection.AddEdge(dom_vertex, child, D_SELECTOR);
          }
       });
   dt.GetGraphInfo().bb_index_map = std::move(inverse_vertex_map);

   std::list<BBGraph::vertex_descriptor> v_topological;
   dt.TopologicalSort(v_topological);
   THROW_ASSERT(v_topological.size(), "");
   bb_topological.reserve(v_topological.size());
   std::transform(v_topological.begin(), v_topological.end(), std::back_inserter(bb_topological),
                  [&](const BBGraph::vertex_descriptor& v) { return dt.CGetNodeInfo(v).block; });
}

DesignFlowStep_Status Bit_Value::InternalExec()
{
   if((parameters->IsParameter("bitvalue") && !parameters->GetParameter<unsigned int>("bitvalue")) ||
      !AppM->ApplyNewTransformation())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   initialize();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing initial backward");
   backward();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed initial backward");
   mix();
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of initial backward:");
   print_bitstring_map(best);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
   bool restart;
   do
   {
      clear_current();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing forward");
      forward();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed forward");
      mix();
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of forward:");
      print_bitstring_map(best);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing backward");
      backward();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed backward");
      restart = mix();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at end of backward:");
      print_bitstring_map(best);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
   } while(restart);
   bb_topological.clear();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of alg:");
   print_bitstring_map(best);
   auto changed = update_IR();
   BitLatticeManipulator::clear();
   direct_call_id_to_called_id.clear();
   arguments.clear();
   if(changed)
   {
      function_behavior->UpdateBitValueVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

unsigned long long Bit_Value::pointer_resizing(const ir_nodeRef& tn) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Pointer resizing starting from " + tn->ToString());
   unsigned long long address_bitsize;
   if(not_frontend)
   {
      const auto& Rmem = GetPointerS<HLS_manager>(AppM)->Rmem;
      const auto var = ir_helper::GetBaseVariable(tn);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Base variable is " + (var ? var->ToString() : "not there"));
      if(Rmem && var && function_behavior->is_variable_mem(var->index))
      {
         const auto max_addr = Rmem->get_base_address(var->index, function_id) + ir_helper::TypeSize(var) / 8;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Maximum address is " + STR(max_addr - 1));
         for(address_bitsize = 1; max_addr > (1ull << address_bitsize); ++address_bitsize)
         {
            ;
         }
         /// added to consider the 64bit alignment of the allocated variables
         if(address_bitsize < 4)
         {
            address_bitsize = 4;
         }
         /// check if it clash with the alignment:
         if(var->get_kind() == variable_val_node_K && Rmem->get_base_address(var->index, function_id) == 0)
         {
            const auto vd = GetPointerS<const variable_val_node>(var);
            const auto align = vd->algn < 8U ? 1U : (vd->algn / 8U);
            auto index = 0u;
            bool found = false;
            for(; index < address_bitsize; ++index)
            {
               if((1ULL << index) & align)
               {
                  found = true;
                  break;
               }
            }
            if(!found)
            {
               ++address_bitsize;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Memory variable " + STR(address_bitsize));
      }
      else
      {
         address_bitsize = AppM->get_address_bitsize();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Address bitsize " + STR(address_bitsize));
      }
   }
   else
   {
      address_bitsize = static_cast<unsigned int>(CompilerWrapper::CGetPointerSize(parameters));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Pointer bitsize " + STR(address_bitsize));
   }
   return address_bitsize;
}

unsigned int Bit_Value::lsb_to_zero(const addr_node* ae, bool safe, bool is_private) const
{
   if(ae->op->get_kind() != variable_val_node_K)
   {
      return 0;
   }
   const auto vd = GetPointerS<const variable_val_node>(ae->op);

   auto align = is_private ? ir_helper::SizeAlloc(ae->op) / 8 : (vd->algn < 64U ? 8U : (vd->algn / 8U));
   if(safe)
   {
      align = 1U;
   }
   auto index = 0u;
   bool found = false;
   auto nbits = AppM->get_address_bitsize();
   for(; index < nbits; ++index)
   {
      if((1ULL << index) & align)
      {
         found = true;
         break;
      }
   }
   return found ? index : nbits;
}

// prints the content of a bitstring map
void Bit_Value::print_bitstring_map(const CustomMap<unsigned int, std::deque<bit_lattice>>&
#ifndef NDEBUG
                                        map
#endif
) const
{
#ifndef NDEBUG
   const auto BH = function_behavior->CGetBehavioralHelper();
   for(const auto& m : map)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "var_uid: " + STR(m.first) + ":" + BH->PrintVariable(m.first) +
                         " bitstring: " + bitstring_to_string(m.second));
   }
#endif
}

bool Bit_Value::update_IR()
{
   bool res = false;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating IR");
   for(const auto& [idx, bv] : best)
   {
      const auto tn = TM->GetIRNode(idx);
      const auto kind = tn->get_kind();
      if(kind == ssa_node_K)
      {
         auto ssa = GetPointerS<ssa_node>(tn);
         if(ssa->bit_values.empty() || isBetter(bitstring_to_string(bv), ssa->bit_values))
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Variable: " + ssa->ToString() + " bitstring: " + ssa->bit_values + " -> " +
                               bitstring_to_string(bv));
            ssa->bit_values = bitstring_to_string(bv);
            // ssa->range = Range::fromBitValues(bv, static_cast<Range::bw_t>(ir_helper::TypeSize(tn)),
            // signed_var.count(ssa->index));
            res = true;
            AppM->RegisterTransformation(GetName(), tn);
         }
      }
      else if(kind == function_val_node_K)
      {
         auto fd = GetPointerS<function_val_node>(tn);
         if(fd->bit_values.empty() || isBetter(bitstring_to_string(bv), fd->bit_values))
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Function: " + ir_helper::GetFunctionName(tn) + " bitstring: " + fd->bit_values + " -> " +
                               bitstring_to_string(bv));
            fd->bit_values = bitstring_to_string(bv);
            res = true;
            AppM->RegisterTransformation(GetName(), tn);
         }
      }
      else if(kind == constant_int_val_node_K || kind == constant_fp_val_node_K)
      {
         // do nothing, constants are recomputed every time
      }
      else
      {
         THROW_ERROR("unexpected condition: variable of kind " + tn->get_kind_text());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Updated IR");

   return res;
}

void Bit_Value::initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Internal initialize");
   BitLatticeManipulator::clear();
   direct_call_id_to_called_id.clear();
   arguments.clear();

   const auto& CGM = AppM->CGetCallGraphManager();
   const auto& cg = CGM.GetCallGraph();
   const auto v = CGM.GetVertex(function_id);

   const auto rbf = CGM.GetReachedBodyFunctions();
   for(const auto& oe : cg.out_edges(v))
   {
      const auto& call_edge_info = cg.CGetEdgeInfo(oe);
      for(const auto& i : call_edge_info.direct_call_points)
      {
         const auto called_id = CGM.get_function(cg.target(oe));
         if(i == 0)
         {
            // never analyze artificial calls
            THROW_ASSERT(AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->GetFunctionName() == MEMCPY,
                         "function " + function_behavior->CGetBehavioralHelper()->GetFunctionName() +
                             " calls function " +
                             AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->GetFunctionName() +
                             " with an artificial call: this should not happen");
            continue;
         }
         if(rbf.find(called_id) != rbf.end())
         {
            direct_call_id_to_called_id[i] = called_id;
         }
      }
   }

   const auto tn = TM->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(tn);

   /*
    * loop on the list of arguments and initialize best
    */
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      const auto parmssa_id = AppM->getSSAFromParm(function_id, parm_decl_node->index);
      const auto parm_type = ir_helper::CGetType(parm_decl_node);
      if(!IsHandledByBitvalue(parm_type))
      {
         continue;
      }
      const auto parmssa = TM->GetIRNode(parmssa_id);
      const auto p = GetPointerS<const ssa_node>(parmssa);
      const auto b = p->CGetUseStmts().empty() ?
                         create_x_bitstring(1) :
                         (p->bit_values.empty() ? create_u_bitstring(ir_helper::TypeSize(parmssa)) :
                                                  string_to_bitstring(p->bit_values));
      best[parmssa_id] = b;
      arguments.insert(parmssa_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "Parameter " + STR(parm_decl_node) + "(" + STR(parm_decl_node->index) + ") bound to " +
                         parmssa->ToString() + ": " + bitstring_to_string(b) + "");
   }

   /*
    * initialize the bitvalue strings for the return value that have been set on
    * the function_val_node from the BitValueIPA
    */
   const auto fu_type = ir_helper::CGetType(tn);
   const auto fret_type_node = ir_helper::GetFunctionReturnType(fu_type);

   if(!fret_type_node || !IsHandledByBitvalue(fret_type_node))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "Function returning " + (fret_type_node ? STR(fret_type_node) : "void") + " not considered");
   }
   else
   {
      const auto is_signed = ir_helper::IsSignedIntegerType(fret_type_node);
      if(fd->bit_values.empty())
      {
         best[function_id] = !fd->range.isUnknown() ? fd->range.getBitValues(is_signed) :
                                                      create_u_bitstring(ir_helper::TypeSize(fret_type_node));
      }
      else
      {
         best[function_id] = string_to_bitstring(fd->bit_values);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Return value: " + bitstring_to_string(best.at(function_id)));
      if(is_signed)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
         signed_var.insert(function_id);
      }
   }

   {
      /*
       * Compute initialization bitstrings for ssa loaded from ROMs. This
       * initialization has to be performed before the other ssa are initialized
       * because we must be sure that bitstrings computed in previous executions
       * of bitvalues analysis are not thrown away before computing inf() on
       * bitvalues used for ROMs. If this initialization is interleaved with the
       * initialization of bitvalues of other ssa we may lose some information,
       * because some of the old bitstrings attached to ssa are cleared during the
       * initialization. If this happens, optimizations on ROMs cannot be
       * aggressive enough, with worse cycles and DSP usage for CHStone benchmarks
       */
      CustomMap<unsigned int, std::deque<bit_lattice>> private_variables;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initializing ROMs loaded ssa bitvalues");
      for(const auto& B : bb_topological)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzing BB" + STR(B->number));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->");
         for(const auto& stmt : B->CGetStmtList())
         {
            const auto stmt_node = stmt;
            if(stmt_node->get_kind() == assign_stmt_K)
            {
               const auto ga = GetPointerS<const assign_stmt>(stmt_node);
               const auto lhs = ga->op0;
               // handle lhs
               if(lhs->get_kind() == ssa_node_K)
               {
                  const auto lhs_nid = lhs->index;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "Analyzing " + stmt_node->get_kind_text() + "(" + STR(stmt_node->index) +
                                     "): " + STR(stmt_node));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LHS: " + STR(lhs));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---RHS: " + ga->op1->get_kind_text());

                  if(!IsHandledByBitvalue(lhs))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---variable " + STR(GetPointerS<const ssa_node>(lhs)) + " of type " +
                                        STR(ir_helper::CGetType(lhs)) + " not considered");
                  }
                  else
                  {
                     const auto lhs_signed = ir_helper::IsSignedIntegerType(lhs);
                     if(lhs_signed)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
                        signed_var.insert(lhs_nid);
                     }
                     /// check if this assignment is a load from a constant array
                     const auto ga_op1_kind = ga->op1->get_kind();
                     if(ga_op1_kind == mem_access_node_K || ga_op1_kind == variable_val_node_K)
                     {
                        const auto hm = GetPointerS<const HLS_manager>(AppM);
                        const auto var_node = ir_helper::GetBaseVariable(ga->op1);
                        if(var_node && var_node->get_kind() == variable_val_node_K &&
                           GetPointerS<variable_val_node>(var_node)->init &&
                           AppM->get_written_objects().find(var_node->index) == AppM->get_written_objects().end() &&
                           hm->Rmem && hm->Rmem->get_enable_hls_bit_value() &&
                           function_behavior->is_variable_mem(var_node->index) && hm->Rmem->is_sds_var(var_node->index))
                        {
                           const auto vd = GetPointerS<variable_val_node>(var_node);
                           std::deque<bit_lattice> current_inf;
                           if(vd->init->get_kind() == constructor_node_K)
                           {
                              current_inf =
                                  constructor_bitstring(vd->init, lhs, hm->Rmem->get_sds_var_size(var_node->index));
                              if(current_inf.size() == 1 && current_inf.at(0) == bit_lattice::X)
                              {
                                 current_inf = create_u_bitstring(ir_helper::TypeSize(lhs));
                              }
                           }
                           else if(vd->init->get_kind() == constant_int_val_node_K)
                           {
                              const auto cst_val = ir_helper::GetConstValue(vd->init);
                              current_inf = create_bitstring_from_constant(cst_val, ir_helper::TypeSize(vd->init),
                                                                           ir_helper::IsSignedIntegerType(vd->init));
                           }
                           else
                           {
                              current_inf = create_u_bitstring(ir_helper::TypeSize(lhs));
                           }

                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---Used the init bitstring " + bitstring_to_string(current_inf));
                           best[lhs_nid] = current_inf;
                        }
                        else
                        {
                           best[lhs_nid] = create_u_bitstring(ir_helper::TypeSize(lhs));
                        }
                        /// and now something for the written variables
                        if(var_node && var_node->get_kind() == variable_val_node_K &&
                           AppM->get_written_objects().find(var_node->index) != AppM->get_written_objects().end() &&
                           hm && hm->Rmem && hm->Rmem->get_enable_hls_bit_value() &&
                           function_behavior->is_variable_mem(var_node->index) &&
                           hm->Rmem->is_private_memory(var_node->index) && hm->Rmem->is_sds_var(var_node->index))
                        {
                           if(!private_variables.count(var_node->index))
                           {
                              const auto vd = GetPointerS<variable_val_node>(var_node);
                              std::deque<bit_lattice> current_inf;
                              if(vd->init)
                              {
                                 if(vd->init->get_kind() == constructor_node_K)
                                 {
                                    current_inf = constructor_bitstring(vd->init, lhs,
                                                                        hm->Rmem->get_sds_var_size(var_node->index));
                                 }
                                 else if(vd->init->get_kind() == constant_int_val_node_K)
                                 {
                                    const auto cst_val = ir_helper::GetConstValue(vd->init);
                                    current_inf =
                                        create_bitstring_from_constant(cst_val, ir_helper::TypeSize(vd->init),
                                                                       ir_helper::IsSignedIntegerType(vd->init));
                                 }
                                 else
                                 {
                                    current_inf = create_u_bitstring(ir_helper::TypeSize(lhs));
                                 }
                              }
                              else
                              {
                                 current_inf.push_back(bit_lattice::X);
                              }
                              INDENT_DBG_MEX(
                                  DEBUG_LEVEL_PEDANTIC, debug_level,
                                  "---Computed the init bitstring for " +
                                      function_behavior->CGetBehavioralHelper()->PrintVariable(var_node->index) +
                                      " = " + bitstring_to_string(current_inf));
                              for(const auto& cur_var : hm->Rmem->get_source_values(var_node->index))
                              {
                                 const auto cur_node = TM->GetIRNode(cur_var);
                                 const auto source_is_signed = ir_helper::IsSignedIntegerType(cur_node);
                                 const auto source_type = ir_helper::CGetType(cur_node);
                                 const auto source_type_size = ir_helper::TypeSize(source_type);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                "---source node: " + STR(cur_node) + " source is signed: " +
                                                    STR(source_is_signed) + " loaded is signed: " + STR(lhs_signed));
                                 std::deque<bit_lattice> cur_bitstring;
                                 if(cur_node->get_kind() == ssa_node_K)
                                 {
                                    const auto ssa = GetPointerS<const ssa_node>(cur_node);
                                    if(!IsHandledByBitvalue(source_type))
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Not handled by bitvalue");
                                       cur_bitstring = create_u_bitstring(ir_helper::TypeSize(cur_node));
                                    }
                                    else
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is handled by bitvalue");
                                       cur_bitstring = string_to_bitstring(ssa->bit_values);
                                    }
                                 }
                                 else if(cur_node->get_kind() == constant_int_val_node_K)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Integer constant");
                                    const auto cst_val = ir_helper::GetConstValue(cur_node);
                                    cur_bitstring =
                                        create_bitstring_from_constant(cst_val, source_type_size, lhs_signed);
                                 }
                                 else
                                 {
                                    cur_bitstring = create_u_bitstring(ir_helper::TypeSize(cur_node));
                                 }
                                 if(cur_bitstring.size() != 0)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                   "---bitstring = " + bitstring_to_string(cur_bitstring));
                                    if(cur_bitstring.size() < source_type_size && source_is_signed != lhs_signed)
                                    {
                                       cur_bitstring =
                                           sign_extend_bitstring(cur_bitstring, source_is_signed, source_type_size);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                      "---bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    sign_reduce_bitstring(cur_bitstring, lhs_signed);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                   "---bitstring = " + bitstring_to_string(cur_bitstring));
                                 }
                                 else
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---bitstring empty --> using U");
                                    cur_bitstring = create_u_bitstring(ir_helper::TypeSize(cur_node));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                   "---bitstring = " + bitstring_to_string(cur_bitstring));
                                 }
                                 current_inf = inf(current_inf, cur_bitstring, lhs);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                "---inf = " + bitstring_to_string(current_inf));
                              }
                              while(current_inf.front() == bit_lattice::X)
                              {
                                 current_inf.pop_front();
                              }
                              if(current_inf.empty())
                              {
                                 current_inf.push_back(bit_lattice::ZERO);
                              }
                              THROW_ASSERT(std::find(current_inf.begin(), current_inf.end(), bit_lattice::X) ==
                                               current_inf.end(),
                                           "Init bitstring must not contain X: " + bitstring_to_string(current_inf));
                              INDENT_DBG_MEX(
                                  DEBUG_LEVEL_PEDANTIC, debug_level,
                                  "---Bit Value: variable " +
                                      function_behavior->CGetBehavioralHelper()->PrintVariable(var_node->index) +
                                      " trimmed to bitsize: " + STR(current_inf.size()) +
                                      " with bit-value pattern: " + bitstring_to_string(current_inf));
                              private_variables[var_node->index] = current_inf;
                           }
                           const auto var_inf = private_variables.at(var_node->index);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---Init bitstring for a private written memory variable " +
                                              bitstring_to_string(var_inf));
                           best[lhs_nid] = var_inf;
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "Analyzed " + stmt_node->get_kind_text() + ": " + STR(stmt_node));
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed BB" + STR(B->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initialized ROMs loaded ssa bitvalues");
   }

   const auto set_value = [&](unsigned int node_id) {
      if(node_id == 0)
      {
         return;
      }
      auto use_node = TM->GetIRNode(node_id);
      if(!IsHandledByBitvalue(use_node))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---variable " + STR(use_node) + " of type " + STR(ir_helper::CGetType(use_node)) +
                            " not considered");
         return;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Use: " + STR(use_node));
      if(use_node->get_kind() == ssa_node_K)
      {
         const auto ssa = GetPointerS<ssa_node>(use_node);
         const auto ssa_is_signed = ir_helper::IsSignedIntegerType(use_node);
         if(ssa_is_signed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
            signed_var.insert(node_id);
         }

         const auto def = ssa->GetDefStmt();
         if(!def ||
            (ssa->var != nullptr && ((def->get_kind() == nop_stmt_K)) && ssa->var->get_kind() == variable_val_node_K))
         {
            best[node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed);
            if(ssa->var)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---first version of uninitialized var " + STR(ssa->var));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---uninitialized ssa");
            }
            if(AppM->ApplyNewTransformation())
            {
               const auto cst_value = TM->CreateUniqueIntegerCst(0, ssa->type);
               const auto uses = ssa->CGetUseStmts();
               for(const auto& stmt_use : uses)
               {
                  TM->ReplaceIRNode(stmt_use.first, use_node, cst_value);
               }
               AppM->RegisterTransformation(GetName(), use_node);
               use_node = cst_value;
               node_id = cst_value->index;
            }
         }
      }

      if(use_node->get_kind() == constant_int_val_node_K)
      {
         const auto cst_val = ir_helper::GetConstValue(use_node);
         const auto is_signed = ir_helper::IsSignedIntegerType(use_node);
         best[node_id] = create_bitstring_from_constant(cst_val, ir_helper::TypeSize(use_node), is_signed);
         if(is_signed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
            signed_var.insert(node_id);
         }
         sign_reduce_bitstring(best.at(node_id), is_signed);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---updated bitstring: " + bitstring_to_string(best.at(node_id)));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   };

   /*
    * now do the real initialization on all the basic blocks
    */
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initializing all ssa bitvalues");
   for(const auto& B : bb_topological)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzing BB" + STR(B->number));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->");

      for(const auto& phi : B->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzing phi(" + STR(phi->index) + "): " + STR(phi));
         const auto pn = GetPointerS<const phi_stmt>(phi);
         const auto is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            const auto res_nid = pn->res->index;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LHS: " + STR(res_nid));
            THROW_ASSERT(pn->res->get_kind() == ssa_node_K, "unexpected condition");
            auto ssa = GetPointerS<ssa_node>(pn->res);
            if(!IsHandledByBitvalue(pn->res))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---variable " + STR(ssa) + " of type " + STR(ir_helper::CGetType(pn->res)) +
                                  " not considered id: " + STR(res_nid));
               continue;
            }
            if(ir_helper::IsSignedIntegerType(pn->res))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
               signed_var.insert(res_nid);
            }

            if(ssa->CGetUseStmts().empty())
            {
               best[res_nid] = create_x_bitstring(1);
               if(best.count(res_nid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(res_nid)));
               }
            }
            else
            {
               if(bb_version == 0 || bb_version != function_behavior->GetBBVersion())
               {
                  best[res_nid] = create_u_bitstring(ir_helper::TypeSize(pn->res));
               }
               else
               {
                  THROW_ASSERT(!ssa->bit_values.empty(), "unexpected case");
                  best[res_nid] = string_to_bitstring(ssa->bit_values);
               }
               if(best.count(res_nid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(res_nid)));
               }

               for(const auto& def_edge : pn->CGetDefEdgesList())
               {
                  set_value(def_edge.first->index);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed phi: " + STR(phi));
         }
      }

      for(const auto& stmt : B->CGetStmtList())
      {
         // ga->op1 is equal to variable_val_node when it binds a newly declared variable to an ssa variable. ie. int a;
         // we can skip this assignment and focus on the ssa variable
         const auto stmt_node = stmt;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Analyzing " + stmt_node->get_kind_text() + "(" + STR(stmt->index) + "): " + STR(stmt_node));
         if(stmt_node->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointerS<const assign_stmt>(stmt_node);

            // handle lhs
            if(ga->op0->get_kind() == ssa_node_K)
            {
               auto lhs_ssa = GetPointerS<ssa_node>(ga->op0);
               const auto lhs_nid = ga->op0->index;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->LHS: " + STR(lhs_ssa));
               if(!IsHandledByBitvalue(ga->op0))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---variable " + STR(lhs_ssa) + " of type " + STR(ir_helper::CGetType(ga->op0)) +
                                     " not considered");
               }
               else
               {
                  const auto lhs_signed = ir_helper::IsSignedIntegerType(ga->op0);
                  if(lhs_signed)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
                     signed_var.insert(lhs_nid);
                  }

                  const auto ga_op1_kind = ga->op1->get_kind();
                  if(lhs_ssa->CGetUseStmts().empty())
                  {
                     best[lhs_nid] = create_x_bitstring(1);
                  }
                  /// check if this assignment is a load from a constant array
                  else if(ga_op1_kind == mem_access_node_K || ga_op1_kind == variable_val_node_K)
                  {
                     // this computation was moved above, to make sure that no
                     // bitstrings (computed by previous execution of the same
                     // bitvalue analysis step) are cleared from ssa variables
                     // before computation of bitvalues for ssa read from ROMs
                     THROW_ASSERT(best.find(lhs_nid) != best.end(), "");
                  }
                  else if(ga_op1_kind == call_node_K)
                  {
                     const auto ce = GetPointerS<const call_node>(ga->op1);
                     if(ce->fn->get_kind() == addr_node_K)
                     {
                        const auto addr_ref = ce->fn;
                        const auto ae = GetPointerS<const addr_node>(addr_ref);
                        THROW_ASSERT(ae->op->get_kind() == function_val_node_K, "node  " + STR(ae->op) +
                                                                                    " is not function_val_node but " +
                                                                                    ae->op->get_kind_text());
                        const auto ret_type_node = ir_helper::GetFunctionReturnType(ae->op);
                        if(IsHandledByBitvalue(ret_type_node))
                        {
                           const auto called_fd = GetPointerS<const function_val_node>(ae->op);
                           const auto new_bitvalue =
                               called_fd->bit_values.empty() ?
                                   (!called_fd->range.isUnknown() ?
                                        called_fd->range.getBitValues(ir_helper::IsSignedIntegerType(ret_type_node)) :
                                        create_u_bitstring(ir_helper::TypeSize(ga->op0))) :
                                   string_to_bitstring(called_fd->bit_values);
                           if(best[lhs_nid].empty())
                           {
                              best[lhs_nid] = new_bitvalue;
                           }
                           else
                           {
                              best[lhs_nid] = sup(new_bitvalue, best[lhs_nid], ga->op0);
                           }
                        }
                     }
                     else if(ce->fn->get_kind() != ssa_node_K)
                     {
                        THROW_UNREACHABLE("call node  " + STR(ce->fn) + " is a " + ce->fn->get_kind_text());
                     }
                  }
                  else if(ga_op1_kind == lut_node_K)
                  {
                     best[lhs_nid] = create_u_bitstring(1);
                  }
                  else if(ga_op1_kind == extract_bit_node_K)
                  {
                     best[lhs_nid] = create_u_bitstring(1);
                  }
                  else
                  {
                     if(bb_version == 0 || bb_version != function_behavior->GetBBVersion())
                     {
                        auto u_string = create_u_bitstring(ir_helper::TypeSize(ga->op0));
                        if(lhs_signed && ir_helper::IsPositiveIntegerValue(ga->op0))
                        {
                           u_string.pop_front();
                           u_string.push_front(bit_lattice::ZERO);
                        }
                        best[lhs_nid] = u_string;
                     }
                     else
                     {
                        THROW_ASSERT(!lhs_ssa->bit_values.empty(), "unexpected case");
                        best[lhs_nid] = string_to_bitstring(lhs_ssa->bit_values);
                     }
                  }
                  THROW_ASSERT(best.count(lhs_nid), "");
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(lhs_nid)));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "RHS: " + ga->op1->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---LHS unhandled for statement kind: " + STR(stmt_node->index));
         }

         std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
         ir_helper::get_required_values(vars_read, stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Requires " + STR(vars_read.size()) + " values");
         for(const auto& var_pair : vars_read)
         {
            const auto ssa_use_node_id = std::get<0>(var_pair);
            set_value(ssa_use_node_id);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Analyzed " + stmt_node->get_kind_text() + ": " + STR(stmt_node));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed BB" + STR(B->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initialized best with all variables in the function:");
   print_bitstring_map(best);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended internal initialize");
}

void Bit_Value::clear_current()
{
   for(const auto& [idx, bv] : best)
   {
      if(arguments.find(idx) != arguments.end() || idx == function_id)
      {
         current[idx] = bv;
      }
      else
      {
         const auto position_in_current = current.find(idx);
         if(position_in_current != current.end())
         {
            current.erase(position_in_current);
         }
      }
   }
}
