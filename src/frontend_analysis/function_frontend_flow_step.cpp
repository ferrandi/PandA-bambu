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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file function_frontend_flow_step.cpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
#include "ext_tree_node.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"
#include "symbolic_application_frontend_flow_step.hpp"
#include "tree_basic_block.hpp"
#include "tree_common.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/tuple/tuple.hpp>

#include <iostream>
#include <utility>

FunctionFrontendFlowStep::FunctionFrontendFlowStep(const application_managerRef _AppM, const unsigned int _function_id,
                                                   const FrontendFlowStepType _frontend_flow_step_type,
                                                   const DesignFlowManagerConstRef _design_flow_manager,
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

FunctionFrontendFlowStep::~FunctionFrontendFlowStep() = default;

DesignFlowStep::signature_t
FunctionFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type,
                                           const unsigned int function_id)
{
   return DesignFlowStep::ComputeSignature(FUNCTION_FRONTEND, static_cast<unsigned short>(frontend_flow_step_type),
                                           function_id);
}

std::string FunctionFrontendFlowStep::GetName() const
{
   return "Frontend::" + GetKindText() + "::" + function_behavior->CGetBehavioralHelper()->get_function_name()
#ifndef NDEBUG
          + (bb_version != 0 ? ("(" + STR(bb_version) + ")") : "") +
          (bitvalue_version != 0 ? ("[" + STR(bitvalue_version) + "]") : "")
#endif
       ;
}

void FunctionFrontendFlowStep::ComputeRelationships(DesignFlowStepSet& relationships,
                                                    const DesignFlowStep::RelationshipType relationship_type)
{
   const auto DFM = design_flow_manager.lock();
   const auto DFG = DFM->CGetDesignFlowGraph();
   const auto CGM = AppM->CGetCallGraphManager();
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
            const auto symbolic_step = DFM->GetDesignFlowStep(symbolic_signature);
            if(symbolic_step != DesignFlowGraph::null_vertex())
            {
#ifndef NDEBUG
               const auto step_sig = FunctionFrontendFlowStep::ComputeSignature(step_type, function_id);
               if(!(DFM->GetStatus(symbolic_signature) == DesignFlowStep_Status::UNEXECUTED ||
                    DFM->GetStatus(step_sig) == DesignFlowStep_Status::SUCCESS ||
                    DFM->GetStatus(step_sig) == DesignFlowStep_Status::UNCHANGED))
               {
                  DFG->WriteDot("Design_Flow_Error");
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

   const auto ACG = CGM->CGetAcyclicCallGraph();
   const auto function_v = CGM->GetVertex(function_id);
   for(const auto& [step_type, rel_type] : frontend_relationships)
   {
      switch(rel_type)
      {
         case(CALLED_FUNCTIONS):
         {
            for(const auto& oe : boost::make_iterator_range(boost::out_edges(function_v, *ACG)))
            {
               const auto target = boost::target(oe, *ACG);
               const auto called_function = CGM->get_function(target);
               if(function_id != called_function &&
                  AppM->CGetFunctionBehavior(called_function)->CGetBehavioralHelper()->has_implementation())
               {
                  const auto function_frontend_flow_step =
                      DFM->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(step_type, called_function));
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
            for(const auto& ie : boost::make_iterator_range(boost::in_edges(function_v, *ACG)))
            {
               const auto source = boost::source(ie, *ACG);
               const auto calling_function = CGM->get_function(source);
               if(calling_function != function_id)
               {
                  const auto function_frontend_flow_step =
                      DFM->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(step_type, calling_function));
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
            const auto prec_step =
                DFM->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(step_type, function_id));
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
   if(!function_id || AppM->CGetCallGraphManager()->GetReachedBodyFunctions().count(function_id))
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

void FunctionFrontendFlowStep::WriteBBGraphDot(const std::string& filename) const
{
   auto bb_graph_info = BBGraphInfoRef(new BBGraphInfo(AppM, function_id));
   BBGraphsCollectionRef GCC_bb_graphs_collection(new BBGraphsCollection(bb_graph_info, parameters));
   BBGraphRef GCC_bb_graph(new BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR));
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   const auto function_tree_node = AppM->get_tree_manager()->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(function_tree_node);
   const auto sl = GetPointerS<const statement_list>(fd->body);
   /// add vertices
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      inverse_vertex_map[bbi] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(bb)));
   }
   /// Set entry and exit
   if(inverse_vertex_map.find(bloc::ENTRY_BLOCK_ID) == inverse_vertex_map.end())
   {
      inverse_vertex_map[bloc::ENTRY_BLOCK_ID] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo()));
   }
   bb_graph_info->entry_vertex = inverse_vertex_map[bloc::ENTRY_BLOCK_ID];
   if(inverse_vertex_map.find(bloc::EXIT_BLOCK_ID) == inverse_vertex_map.end())
   {
      inverse_vertex_map[bloc::EXIT_BLOCK_ID] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo()));
   }
   bb_graph_info->exit_vertex = inverse_vertex_map[bloc::EXIT_BLOCK_ID];

   /// add edges
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto pred : bb->list_of_pred)
      {
         if(pred == bloc::ENTRY_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[pred], inverse_vertex_map[bbi], CFG_SELECTOR);
         }
      }
      for(const auto succ : bb->list_of_succ)
      {
         THROW_ASSERT(inverse_vertex_map.find(bbi) != inverse_vertex_map.end(), "BB" + STR(bbi) + " does not exist");
         THROW_ASSERT(inverse_vertex_map.find(succ) != inverse_vertex_map.end(), "BB" + STR(succ) + " does not exist");
         if(bb->CGetStmtList().size() and bb->CGetStmtList().back()->get_kind() == gimple_multi_way_if_K)
         {
            const auto gmwi = GetPointerS<const gimple_multi_way_if>(bb->CGetStmtList().back());
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
            THROW_ASSERT(conds.size(), "Inconsistency between cfg and output of gimple_multi_way_if " +
                                           gmwi->ToString() + "- condition for BB" + STR(succ) + " not found");
            const EdgeInfoRef edge_info(new BBEdgeInfo());
            for(auto cond : conds)
            {
               GetPointerS<BBEdgeInfo>(edge_info)->add_nodeID(cond, CFG_SELECTOR);
            }
            GCC_bb_graphs_collection->InternalAddEdge(inverse_vertex_map[bbi], inverse_vertex_map[succ], CFG_SELECTOR,
                                                      edge_info);
         }
         else
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(bb->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bbi], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                           CFG_SELECTOR);
      }
   }

   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                     CFG_SELECTOR);
   GCC_bb_graph->WriteDot(filename);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written " + filename);
   /// add edges
#ifndef NDEBUG
   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      for(const auto& phi : bb->CGetPhiList())
      {
         const auto gp = GetPointerS<const gimple_phi>(phi);
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
