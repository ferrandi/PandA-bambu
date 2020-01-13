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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

#include "Parameter.hpp"           // for Parameter
#include "application_manager.hpp" // for application_m...
#include "basic_block.hpp"         // for BBGraphsColle...
#include "behavioral_helper.hpp"   // for BehavioralHelper
#include "call_graph.hpp"          // for CallGraph
#include "call_graph_manager.hpp"  // for CallGraphMana...
#include "cdfg_edge_info.hpp"      // for CFG_SELECTOR
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"                          // for DEBUG_LEVEL_V...
#include "design_flow_graph.hpp"                       // for DesignFlowGraph
#include "design_flow_manager.hpp"                     // for DesignFlowMan...
#include "design_flow_step_factory.hpp"                // for DesignFlowMan...
#include "edge_info.hpp"                               // for EdgeInfoRef
#include "exceptions.hpp"                              // for THROW_ASSERT
#include "ext_tree_node.hpp"                           // for gimple_multi_...
#include "frontend_flow_step_factory.hpp"              // for DesignFlowSte...
#include "function_behavior.hpp"                       // for FunctionBehavior
#include "graph.hpp"                                   // for SelectEdge
#include "hash_helper.hpp"                             // for hash
#include "string_manipulation.hpp"                     // for STR GET_CLASS
#include "symbolic_application_frontend_flow_step.hpp" // for SymbolicAppli...
#include "tree_basic_block.hpp"                        // for bloc, bloc::E...
#include "tree_common.hpp"                             // for gimple_multi_...
#include "tree_manager.hpp"                            // for ParameterCons...
#include "tree_node.hpp"                               // for function_decl
#include "tree_reindex.hpp"
#include <boost/iterator/iterator_facade.hpp> // for operator!=
#include <boost/lexical_cast.hpp>             // for lexical_cast
#include <boost/tuple/tuple.hpp>              // for tie
#include <iostream>                           // for ios_base::fai...
#include <utility>                            // for pair

FunctionFrontendFlowStep::FunctionFrontendFlowStep(const application_managerRef _AppM, const unsigned int _function_id, const FrontendFlowStepType _frontend_flow_step_type, const DesignFlowManagerConstRef _design_flow_manager,
                                                   const ParameterConstRef _parameters)
    : FrontendFlowStep(_AppM, _frontend_flow_step_type, _design_flow_manager, _parameters), function_behavior(_AppM->GetFunctionBehavior(_function_id)), function_id(_function_id), bb_version(0), bitvalue_version(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

FunctionFrontendFlowStep::~FunctionFrontendFlowStep() = default;

const std::string FunctionFrontendFlowStep::GetSignature() const
{
   return ComputeSignature(frontend_flow_step_type, function_id);
}

const std::string FunctionFrontendFlowStep::ComputeSignature(const FrontendFlowStepType frontend_flow_step_type, const unsigned int function_id)
{
   return "Frontend::" + boost::lexical_cast<std::string>(frontend_flow_step_type) + "::" + boost::lexical_cast<std::string>(function_id);
}

const std::string FunctionFrontendFlowStep::GetName() const
{
#ifndef NDEBUG
   const std::string version = bb_version != 0 ? ("(" + STR(bb_version) + ")") : "";
   const std::string bv_version = bitvalue_version != 0 ? ("[" + STR(bitvalue_version) + "]") : "";
#else
   const std::string version = "";
   const std::string bv_version = "";
#endif
   return "Frontend::" + GetKindText() + "::" + function_behavior->CGetBehavioralHelper()->get_function_name() + version + bv_version;
}

void FunctionFrontendFlowStep::ComputeRelationships(DesignFlowStepSet& relationships, const DesignFlowStep::RelationshipType relationship_type)
{
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(CGetDesignFlowStepFactory());
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> frontend_relationships = ComputeFrontendRelationships(relationship_type);

   /// Precedence step whose symbolic application frontend flow step has to be executed can be considered as dependence step
   if(relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      const auto precedence_relationships = ComputeFrontendRelationships(PRECEDENCE_RELATIONSHIP);
      for(const auto& precedence_relationship : precedence_relationships)
      {
         if(precedence_relationship.second == SAME_FUNCTION)
         {
            const auto signature = FunctionFrontendFlowStep::ComputeSignature(precedence_relationship.first, function_id);
            const auto symbolic_signature = SymbolicApplicationFrontendFlowStep::ComputeSignature(precedence_relationship.first);
            const auto symbolic_step = design_flow_manager.lock()->GetDesignFlowStep(symbolic_signature);
            if(symbolic_step)
            {
#ifndef NDEBUG
               if(not(design_flow_manager.lock()->GetStatus(symbolic_signature) == DesignFlowStep_Status::UNEXECUTED or design_flow_manager.lock()->GetStatus(signature) == DesignFlowStep_Status::SUCCESS or
                      design_flow_manager.lock()->GetStatus(signature) == DesignFlowStep_Status::UNCHANGED))
               {
                  design_flow_manager.lock()->CGetDesignFlowGraph()->WriteDot("Design_Flow_Error");
                  const auto design_flow_step_info = design_flow_graph->CGetDesignFlowStepInfo(symbolic_step);
                  THROW_UNREACHABLE("Symbolic step " + design_flow_step_info->design_flow_step->GetName() + " is not unexecuted");
               }
#endif
               frontend_relationships.insert(precedence_relationship);
            }
         }
      }
   }
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>::const_iterator frontend_relationship, frontend_relationship_end = frontend_relationships.end();
   for(frontend_relationship = frontend_relationships.begin(); frontend_relationship != frontend_relationship_end; ++frontend_relationship)
   {
      switch(frontend_relationship->second)
      {
         case(ALL_FUNCTIONS):
         {
            /// This is managed by FrontendFlowStep::ComputeRelationships
            break;
         }
         case(CALLED_FUNCTIONS):
         {
            const CallGraphManagerConstRef call_graph_manager = AppM->CGetCallGraphManager();
            const CallGraphConstRef acyclic_call_graph = call_graph_manager->CGetAcyclicCallGraph();
            const vertex function_vertex = call_graph_manager->GetVertex(function_id);
            OutEdgeIterator oe, oe_end;
            for(boost::tie(oe, oe_end) = boost::out_edges(function_vertex, *acyclic_call_graph); oe != oe_end; oe++)
            {
               const vertex target = boost::target(*oe, *acyclic_call_graph);
               const unsigned int called_function = call_graph_manager->get_function(target);
               if(AppM->CGetFunctionBehavior(called_function)->CGetBehavioralHelper()->has_implementation() and function_id != called_function)
               {
                  vertex function_frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(frontend_relationship->first, called_function));
                  DesignFlowStepRef design_flow_step;
                  if(function_frontend_flow_step)
                  {
                     design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(function_frontend_flow_step)->design_flow_step;
                     relationships.insert(design_flow_step);
                  }
                  else
                  {
                     design_flow_step = frontend_flow_step_factory->CreateFunctionFrontendFlowStep(frontend_relationship->first, called_function);
                     relationships.insert(design_flow_step);
                  }
               }
            }
            break;
         }
         case(CALLING_FUNCTIONS):
         {
            const CallGraphManagerConstRef call_graph_manager = AppM->CGetCallGraphManager();
            const CallGraphConstRef acyclic_call_graph = call_graph_manager->CGetAcyclicCallGraph();
            const vertex function_vertex = call_graph_manager->GetVertex(function_id);
            InEdgeIterator ie, ie_end;
            for(boost::tie(ie, ie_end) = boost::in_edges(function_vertex, *acyclic_call_graph); ie != ie_end; ie++)
            {
               const vertex source = boost::source(*ie, *acyclic_call_graph);
               const unsigned int calling_function = call_graph_manager->get_function(source);
               if(calling_function != function_id)
               {
                  vertex function_frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(frontend_relationship->first, calling_function));
                  DesignFlowStepRef design_flow_step;
                  if(function_frontend_flow_step)
                  {
                     design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(function_frontend_flow_step)->design_flow_step;
                     relationships.insert(design_flow_step);
                  }
                  else
                  {
                     design_flow_step = frontend_flow_step_factory->CreateFunctionFrontendFlowStep(frontend_relationship->first, calling_function);
                     relationships.insert(design_flow_step);
                  }
               }
            }
            break;
         }
         case(SAME_FUNCTION):
         {
            vertex prec_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(frontend_relationship->first, function_id));
            DesignFlowStepRef design_flow_step;
            if(prec_step)
            {
               design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(prec_step)->design_flow_step;
               relationships.insert(design_flow_step);
            }
            else
            {
               design_flow_step = frontend_flow_step_factory->CreateFunctionFrontendFlowStep(frontend_relationship->first, function_id);
               relationships.insert(design_flow_step);
            }
            break;
         }
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
   const auto status = InternalExec();
   bb_version = function_behavior->GetBBVersion();
   bitvalue_version = function_behavior->GetBitValueVersion();
   return status;
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
   const tree_nodeConstRef function_tree_node = AppM->get_tree_manager()->CGetTreeNode(function_id);
   const auto fd = GetPointer<const function_decl>(function_tree_node);
   const auto sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
   /// add vertices
   for(auto block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = GCC_bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)));
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
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto pred : block.second->list_of_pred)
      {
         if(pred == bloc::ENTRY_BLOCK_ID)
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[pred], inverse_vertex_map[block.first], CFG_SELECTOR);
         }
      }
      for(const auto succ : block.second->list_of_succ)
      {
         THROW_ASSERT(inverse_vertex_map.find(block.first) != inverse_vertex_map.end(), "BB" + STR(block.first) + " does not exist");
         THROW_ASSERT(inverse_vertex_map.find(succ) != inverse_vertex_map.end(), "BB" + STR(succ) + " does not exist");
         if(block.second->CGetStmtList().size() and GET_NODE(block.second->CGetStmtList().back())->get_kind() == gimple_multi_way_if_K)
         {
            const auto gmwi = GetPointer<const gimple_multi_way_if>(GET_NODE(block.second->CGetStmtList().back()));
            CustomSet<unsigned int> conds;
            for(auto gmwi_cond : gmwi->list_of_cond)
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
            THROW_ASSERT(conds.size(), "Inconsistency between cfg and output of gimple_multi_way_if " + gmwi->ToString() + "- condition for BB" + STR(succ) + " not found");
            const EdgeInfoRef edge_info(new BBEdgeInfo());
            for(auto cond : conds)
            {
               GetPointer<BBEdgeInfo>(edge_info)->add_nodeID(cond, CFG_SELECTOR);
            }
            GCC_bb_graphs_collection->InternalAddEdge(inverse_vertex_map[block.first], inverse_vertex_map[succ], CFG_SELECTOR, edge_info);
         }
         else
         {
            GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[block.first], inverse_vertex_map[succ], CFG_SELECTOR);
         }
      }
      if(block.second->list_of_succ.empty())
      {
         GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[block.first], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
      }
   }

   /// add a connection between entry and exit thus avoiding problems with non terminating code
   GCC_bb_graphs_collection->AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID], CFG_SELECTOR);
   BBGraph(GCC_bb_graphs_collection, CFG_SELECTOR).WriteDot(filename);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written " + filename);
   /// add edges
#ifndef NDEBUG
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& phi : block.second->CGetPhiList())
      {
         const auto gp = GetPointer<const gimple_phi>(GET_CONST_NODE(phi));
         THROW_ASSERT(gp->CGetDefEdgesList().size() == block.second->list_of_pred.size(), "BB" + STR(block.second->number) + " has " + STR(block.second->list_of_pred.size()) + " incoming edges but contains " + STR(phi));
      }
   }
#endif
}

unsigned int FunctionFrontendFlowStep::CGetBBVersion() const
{
   return bb_version;
}

void FunctionFrontendFlowStep::PrintInitialIR() const
{
   WriteBBGraphDot("BB_Before_" + GetName() + ".dot");
   FrontendFlowStep::PrintInitialIR();
}

void FunctionFrontendFlowStep::PrintFinalIR() const
{
   WriteBBGraphDot("BB_After_" + GetName() + ".dot");
   FrontendFlowStep::PrintFinalIR();
}
