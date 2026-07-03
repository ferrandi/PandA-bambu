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
 * @file function_frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "function_frontend_flow_step.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cdfg_edge_info.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step_factory.hpp"
#include "edge_info.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"
#include "symbolic_application_frontend_flow_step.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/tuple/tuple.hpp>

#include <iostream>
#include <utility>

FunctionFrontendFlowStep::FunctionFrontendFlowStep(const application_managerRef _AppM, const unsigned int _function_id,
                                                   const FrontendFlowStepType _frontend_flow_step_type,
                                                   const DesignFlowManager& _design_flow_manager,
                                                   const ParameterConstRef _parameters)
    : FrontendFlowStep(ComputeSignature(_frontend_flow_step_type, _function_id), _AppM, _frontend_flow_step_type,
                       _design_flow_manager, _parameters),
      function_behavior(_AppM->GetFunctionBehavior(_function_id)),
      function_id(_function_id),
      bb_version(0),
      bitvalue_version(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStep::signature_t
FunctionFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type,
                                           const unsigned int function_id)
{
   return DesignFlowStep::ComputeSignature(FUNCTION_FRONTEND, static_cast<unsigned short>(frontend_flow_step_type),
                                           function_id);
}

std::string FunctionFrontendFlowStep::GetName() const
{
   return "Frontend::" + GetKindText() + "::" + function_behavior->CGetBehavioralHelper()->GetFunctionName()
#ifndef NDEBUG
          + (bb_version != 0 ? ("(" + STR(bb_version) + ")") : "") +
          (bitvalue_version != 0 ? ("[" + STR(bitvalue_version) + "]") : "")
#endif
       ;
}

void FunctionFrontendFlowStep::ComputeRelationships(DesignFlowStepSet& relationships,
                                                    const DesignFlowStep::RelationshipType relationship_type)
{
   const auto DFG = design_flow_manager.CGetDesignFlowGraph();
   const auto& CGM = AppM->CGetCallGraphManager();
   const auto frontend_flow_step_factory = GetPointerS<const FrontendFlowStepFactory>(CGetDesignFlowStepFactory());
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> frontend_relationships =
       ComputeFrontendRelationships(relationship_type);

   /// Precedence step whose symbolic application frontend flow step has to be executed can be considered as dependence
   /// step
   if(relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      const auto precedence_relationships = ComputeFrontendRelationships(PRECEDENCE_RELATIONSHIP);
      for(const auto& [step_type, rel_type] : precedence_relationships)
      {
         if(rel_type == SAME_FUNCTION)
         {
            const auto symbolic_signature = SymbolicApplicationFrontendFlowStep::ComputeSignature(step_type);
            const auto symbolic_step = design_flow_manager.GetDesignFlowStep(symbolic_signature);
            if(symbolic_step != DesignFlowGraph::null_vertex())
            {
#ifndef NDEBUG
               const auto step_sig = FunctionFrontendFlowStep::ComputeSignature(step_type, function_id);
               if(!(design_flow_manager.GetStatus(symbolic_signature) == DesignFlowStep_Status::UNEXECUTED ||
                    design_flow_manager.GetStatus(step_sig) == DesignFlowStep_Status::SUCCESS ||
                    design_flow_manager.GetStatus(step_sig) == DesignFlowStep_Status::UNCHANGED))
               {
                  DFG->writeDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "Design_Flow_Error");
                  const auto design_flow_step_info = DFG->CGetNodeInfo(symbolic_step);
                  THROW_UNREACHABLE("Symbolic step " + design_flow_step_info->design_flow_step->GetName() +
                                    " is not unexecuted");
               }
#endif
               frontend_relationships.emplace(step_type, rel_type);
            }
         }
      }
   }

   const auto ACG = CGM.CGetAcyclicCallGraph();
   const auto function_v = CGM.GetVertex(function_id);
   for(const auto& [step_type, rel_type] : frontend_relationships)
   {
      switch(rel_type)
      {
         case(CALLED_FUNCTIONS):
         {
            for(const auto& oe : ACG.out_edges(function_v))
            {
               const auto target = ACG.target(oe);
               const auto called_function = CGM.get_function(target);
               if(function_id != called_function &&
                  AppM->CGetFunctionBehavior(called_function)->CGetBehavioralHelper()->has_implementation())
               {
                  const auto function_frontend_flow_step = design_flow_manager.GetDesignFlowStep(
                      FunctionFrontendFlowStep::ComputeSignature(step_type, called_function));
                  DesignFlowStepRef design_flow_step;
                  if(function_frontend_flow_step != DesignFlowGraph::null_vertex())
                  {
                     design_flow_step = DFG->CGetNodeInfo(function_frontend_flow_step)->design_flow_step;
                  }
                  else
                  {
                     design_flow_step =
                         frontend_flow_step_factory->CreateFunctionFrontendFlowStep(step_type, called_function);
                  }
                  relationships.insert(design_flow_step);
               }
            }
            break;
         }
         case(CALLING_FUNCTIONS):
         {
            for(const auto& ie : ACG.in_edges(function_v))
            {
               const auto source = ACG.source(ie);
               const auto calling_function = CGM.get_function(source);
               if(calling_function != function_id)
               {
                  const auto function_frontend_flow_step = design_flow_manager.GetDesignFlowStep(
                      FunctionFrontendFlowStep::ComputeSignature(step_type, calling_function));
                  DesignFlowStepRef design_flow_step;
                  if(function_frontend_flow_step != DesignFlowGraph::null_vertex())
                  {
                     design_flow_step = DFG->CGetNodeInfo(function_frontend_flow_step)->design_flow_step;
                  }
                  else
                  {
                     design_flow_step =
                         frontend_flow_step_factory->CreateFunctionFrontendFlowStep(step_type, calling_function);
                  }
                  relationships.insert(design_flow_step);
               }
            }
            break;
         }
         case(SAME_FUNCTION):
         {
            const auto prec_step = design_flow_manager.GetDesignFlowStep(
                FunctionFrontendFlowStep::ComputeSignature(step_type, function_id));
            DesignFlowStepRef design_flow_step;
            if(prec_step != DesignFlowGraph::null_vertex())
            {
               design_flow_step = DFG->CGetNodeInfo(prec_step)->design_flow_step;
            }
            else
            {
               design_flow_step = frontend_flow_step_factory->CreateFunctionFrontendFlowStep(step_type, function_id);
            }
            relationships.insert(design_flow_step);
            break;
         }
         case(ALL_FUNCTIONS):
         case(WHOLE_APPLICATION):
         {
            /// This is managed by FrontendFlowStep::ComputeRelationships
            break;
         }
         default:
         {
            THROW_UNREACHABLE("Function relationship does not exist");
         }
      }
   }
   FrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

DesignFlowStep_Status FunctionFrontendFlowStep::Exec()
{
   if(!function_id || AppM->CGetCallGraphManager().GetReachedBodyFunctions().count(function_id))
   {
      const auto status = InternalExec();
      bb_version = function_behavior->GetBBVersion();
      bitvalue_version = function_behavior->GetBitValueVersion();
      return status;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

bool FunctionFrontendFlowStep::HasToBeExecuted() const
{
   return bb_version != function_behavior->GetBBVersion();
}

void FunctionFrontendFlowStep::WriteBBGraphDot(const std::filesystem::path& filename) const
{
   BBGraphsCollection ir_bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph ir_bb_graph(ir_bb_graphs_collection, CFG_SELECTOR);
   auto& bb_graph_info = ir_bb_graphs_collection.GetGraphInfo();
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   const auto function_ir_node = AppM->get_ir_manager()->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(function_ir_node);
   const auto sl = GetPointerS<const statement_list_node>(fd->body);
   /// add vertices
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      inverse_vertex_map[bbi] = ir_bb_graphs_collection.AddVertex(BBNodeInfo(bb));
   }
   /// Set entry and exit
   if(inverse_vertex_map.find(bloc::ENTRY_BLOCK_ID) == inverse_vertex_map.end())
   {
      inverse_vertex_map[bloc::ENTRY_BLOCK_ID] = ir_bb_graphs_collection.AddVertex();
   }
   bb_graph_info.entry_vertex = inverse_vertex_map[bloc::ENTRY_BLOCK_ID];
   if(inverse_vertex_map.find(bloc::EXIT_BLOCK_ID) == inverse_vertex_map.end())
   {
      inverse_vertex_map[bloc::EXIT_BLOCK_ID] = ir_bb_graphs_collection.AddVertex();
   }
   bb_graph_info.exit_vertex = inverse_vertex_map[bloc::EXIT_BLOCK_ID];

   /// add edges
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto pred : bb->list_of_pred)
      {
         if(pred == bloc::ENTRY_BLOCK_ID)
         {
            ir_bb_graphs_collection.AddEdge(inverse_vertex_map[pred], inverse_vertex_map[bbi], CFG_SELECTOR);
         }
      }
      for(const auto succ : bb->list_of_succ)
      {
         THROW_ASSERT(inverse_vertex_map.find(bbi) != inverse_vertex_map.end(), "BB" + STR(bbi) + " does not exist");
         THROW_ASSERT(inverse_vertex_map.find(succ) != inverse_vertex_map.end(), "BB" + STR(succ) + " does not exist");
         if(bb->CGetStmtList().size() and bb->CGetStmtList().back()->get_kind() == multi_way_if_stmt_K)
         {
            const auto gmwi = GetPointerS<const multi_way_if_stmt>(bb->CGetStmtList().back());
            CustomSet<unsigned int> conds;
            for(const auto& gmwi_cond : gmwi->list_of_cond)
            {
               if(gmwi_cond.second == succ)
               {
                  if(gmwi_cond.first)
                  {
                     conds.insert(gmwi_cond.first->index);
                  }
                  else
                  {
                     conds.insert(default_COND);
                  }
               }
            }
            THROW_ASSERT(conds.size(), "Inconsistency between cfg and output of multi_way_if_stmt " + gmwi->ToString() +
                                           "- condition for BB" + STR(succ) + " not found");
            BBEdgeInfo edge_info;
            for(auto cond : conds)
            {
               edge_info.add_nodeID(cond, CFG_SELECTOR);
            }
            ir_bb_graphs_collection.AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[succ], CFG_SELECTOR, edge_info);
         }
         else
         {
            ir_bb_graphs_collection.AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(bb->list_of_succ.empty())
      {
         ir_bb_graphs_collection.AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                         CFG_SELECTOR);
      }
   }

   /// add a connection between entry and exit thus avoiding problems with non terminating code
   ir_bb_graphs_collection.AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                   CFG_SELECTOR);
   ir_bb_graph.writeDot(function_behavior->GetDotPath() / filename);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written " + filename.string());
   /// add edges
#if HAVE_ASSERTS
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto& phi : bb->CGetPhiList())
      {
         const auto gp = GetPointerS<const phi_stmt>(phi);
         THROW_ASSERT(gp->CGetDefEdgesList().size() == bb->list_of_pred.size(),
                      "BB" + STR(bb->number) + " has " + STR(bb->list_of_pred.size()) +
                          " incoming edges but contains " + STR(phi));
      }
   }
#endif
}

unsigned int FunctionFrontendFlowStep::CGetBBVersion() const
{
   return bb_version;
}

unsigned int FunctionFrontendFlowStep::GetBitValueVersion() const
{
   return bitvalue_version;
}

void FunctionFrontendFlowStep::PrintInitialIR() const
{
   if(!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF"))
   {
      WriteBBGraphDot("BB_Before_" + GetName() + ".dot");
   }
   FrontendFlowStep::PrintInitialIR();
}

void FunctionFrontendFlowStep::PrintFinalIR() const
{
   if(!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF"))
   {
      WriteBBGraphDot("BB_After_" + GetName() + ".dot");
   }
   FrontendFlowStep::PrintFinalIR();
}
