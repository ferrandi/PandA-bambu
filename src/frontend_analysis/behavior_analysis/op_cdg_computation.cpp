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
 * @file op_cdg_computation.cpp
 * @brief Analysis step performing operation control dependence computation.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "op_cdg_computation.hpp"

///. include
#include "Parameter.hpp"

/// algorithms/dominance include
#include "Dominance.hpp"

/// behavior include
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

OpCdgComputation::OpCdgComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, OP_CONTROL_DEPENDENCE_COMPUTATION, _design_flow_manager, _Param)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

OpCdgComputation::~OpCdgComputation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> OpCdgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_CONTROL_DEPENDENCE_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_ORDER_COMPUTATION, SAME_FUNCTION));
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

void OpCdgComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const OpGraphConstRef cdg = function_behavior->CGetOpGraph(FunctionBehavior::CDG);
      if(boost::num_vertices(*cdg) != 0)
      {
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*cdg); edge != edge_end; edge++)
         {
            function_behavior->ogc->RemoveSelector(*edge, CDG_SELECTOR);
         }
      }
   }
}

DesignFlowStep_Status OpCdgComputation::InternalExec()
{
   const auto fcfg = function_behavior->fcfg;
   const auto bb_cdg = function_behavior->CGetBBGraph(FunctionBehavior::CDG_BB);
   EdgeIterator edge, edge_end;
   for(boost::tie(edge, edge_end) = boost::edges(*bb_cdg); edge != edge_end; edge++)
   {
      const auto source = boost::source(*edge, *bb_cdg);
      const auto target = boost::target(*edge, *bb_cdg);
      const auto source_operations = bb_cdg->CGetBBNodeInfo(source)->statements_list;
      const auto target_operations = bb_cdg->CGetBBNodeInfo(target)->statements_list;
      if(source_operations.size() and target_operations.size())
      {
         const auto labels = bb_cdg->CGetBBEdgeInfo(*edge)->get_labels(CFG_SELECTOR);
         const auto source_operation = source_operations.back();
         for(const auto target_operation : target_operations)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding Control Dependence " + GET_NAME(fcfg, source_operation) + "-->" + GET_NAME(fcfg, target_operation));
            function_behavior->ogc->AddEdge(source_operation, target_operation, CDG_SELECTOR);
            for(const auto label : labels)
            {
               function_behavior->ogc->add_edge_info(source_operation, target_operation, CDG_SELECTOR, label);
            }
         }
      }
   }

   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*bb_cdg); basic_block != basic_block_end; basic_block++)
   {
      const auto bb_node_info = bb_cdg->CGetBBNodeInfo(*basic_block);
      const auto cer_index = bb_node_info->cer;
      for(const auto statement : bb_node_info->statements_list)
      {
         fcfg->GetOpNodeInfo(statement)->cer = cer_index;
      }
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetOpGraph(FunctionBehavior::CDG)->WriteDot("OP_CDG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
