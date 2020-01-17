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
 * @file add_op_loop_flow_edges.hpp
 * @brief Analysis step which adds flow edges for scheduling to operation graphs
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "add_op_loop_flow_edges.hpp"

/// Algorithm include
#include "loop.hpp"
#include "loops.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// Tree include
#include "tree_basic_block.hpp"

/// Parameter include
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AddOpLoopFlowEdges::AddOpLoopFlowEdges(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_OP_LOOP_FLOW_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

AddOpLoopFlowEdges::~AddOpLoopFlowEdges() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AddOpLoopFlowEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_CONTROL_DEPENDENCE_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_REACHABILITY_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SCALAR_SSA_DATA_FLOW_ANALYSIS, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(AGGREGATE_DATA_FLOW_ANALYSIS, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
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

void AddOpLoopFlowEdges::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphConstRef flg = function_behavior->CGetOpGraph(FunctionBehavior::FLG);
      if(boost::num_vertices(*flg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*flg); edge != edge_end; edge++)
         {
            function_behavior->ogc->RemoveSelector(*edge, FLG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status AddOpLoopFlowEdges::InternalExec()
{
   /// The control flow graph of operation
   const OpGraphConstRef fcfg = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);

   /// The graph used for the scheduling
   const OpGraphConstRef fflsaodg = function_behavior->CGetOpGraph(FunctionBehavior::FFLSAODG);

   /// The control flow graph of basic blocks
   const BBGraphRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);

   /// Operations of each loop
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<vertex>> loop_operations;

   /// The loop structure
   const std::list<LoopConstRef>& loops = function_behavior->CGetLoops()->GetList();

   /// Adding edges from last operation of header to all operations of the loop
   std::list<LoopConstRef>::const_iterator loop, loop_end = loops.end();
   for(loop = loops.begin(); loop != loop_end; ++loop)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering loop " + boost::lexical_cast<std::string>((*loop)->GetId()));
      if(!(*loop)->IsReducible())
      {
         THROW_ERROR_CODE(IRREDUCIBLE_LOOPS_EC, "Irreducible loops not yet supported");
      }

      /// Operations which belong to the loop
      const CustomUnorderedSet<vertex>& loop_bb = (*loop)->get_blocks();
      CustomUnorderedSet<vertex>::const_iterator it, it_end = loop_bb.end();
      for(it = loop_bb.begin(); it != it_end; ++it)
      {
         const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(*it);
         /// Skip operation belonging to the header itself
         const std::list<vertex>& statements_list = bb_node_info->statements_list;
         std::list<vertex>::const_iterator it2, it2_end = statements_list.end();
         for(it2 = statements_list.begin(); it2 != it2_end; ++it2)
         {
            if(bb_node_info->block->number != bb_node_info->loop_id)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   " + GET_NAME(fcfg, *it2));
               loop_operations[(*loop)->GetId()].insert(*it2);
            }
         }
      }

      /// Searching last operation of the header
      const vertex header = (*loop)->GetHeader();
      const BBNodeInfoConstRef bb_node_info = fbb->CGetBBNodeInfo(header);
      const std::list<vertex>& statements_list = bb_node_info->statements_list;
      THROW_ASSERT(statements_list.size(), "Header of a loop " + boost::lexical_cast<std::string>((*loop)->GetId()) + " is empty");
      const vertex last_statement = *(statements_list.rbegin());

      /// add a flow edge from the last operation of the header and the operations of the loop
      /// useful only for the speculation graph
      auto it3_end = loop_operations[(*loop)->GetId()].end();
      for(auto it3 = loop_operations[(*loop)->GetId()].begin(); it3 != it3_end; ++it3)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding flow edge from " + GET_NAME(fcfg, last_statement) + " to " + GET_NAME(fcfg, *it3));
         function_behavior->ogc->AddEdge(last_statement, *it3, FLG_SELECTOR);
      }
      /// add a feedback flow edge from the operations of the loop to the first statement of the header
      const vertex first_statement = *(statements_list.begin());
      for(auto it3 = loop_operations[(*loop)->GetId()].begin(); it3 != it3_end; ++it3)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Adding a feedback flow edge from " + GET_NAME(fcfg, *it3) + " to " + GET_NAME(fcfg, first_statement));
         function_behavior->ogc->AddEdge(*it3, first_statement, FB_FLG_SELECTOR);
      }
   }

#ifndef NDEBUG
   const std::string function_name = function_behavior->CGetBehavioralHelper()->get_function_name();
#endif
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::FLG)->WriteDot("OP_FL.dot");
      function_behavior->CGetOpGraph(FunctionBehavior::FFLSAODG)->WriteDot("OP_FFLSAODG.dot");
   }
#ifndef NDEBUG
   try
   {
      const OpGraphConstRef flsaodg = function_behavior->CGetOpGraph(FunctionBehavior::FLSAODG);
      std::deque<vertex> vertices;
      boost::topological_sort(*flsaodg, std::front_inserter(vertices));
   }
   catch(const char* msg)
   {
      THROW_UNREACHABLE("flsaodg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::string& msg)
   {
      THROW_UNREACHABLE("flsaodg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::exception& ex)
   {
      THROW_UNREACHABLE("flsaodg graph of function " + function_name + " is not acyclic");
   }
   catch(...)
   {
      THROW_UNREACHABLE("flsaodg graph of function " + function_name + " is not acyclic");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
