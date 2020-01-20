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
 * @file loops_computation.cpp
 * @brief Analysis step performing loops computation.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "loops_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "tree_basic_block.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <iosfwd>

loops_computation::loops_computation(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LOOPS_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

loops_computation::~loops_computation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> loops_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BASIC_BLOCKS_CFG_COMPUTATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DOM_POST_DOM_COMPUTATION, SAME_FUNCTION));
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

void loops_computation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
      function_behavior->loops = LoopsRef();
}

DesignFlowStep_Status loops_computation::InternalExec()
{
   const BBGraphRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);

   THROW_ASSERT(function_behavior->dominators, "Dominators has to be computed!");
   function_behavior->loops = LoopsRef(new Loops(function_behavior, parameters));
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->CGetLoops()->WriteDot("LF.dot");
   }
   std::list<LoopConstRef> loops = function_behavior->CGetLoops()->GetList();
   std::list<LoopConstRef>::const_iterator loop_end = loops.end();
   for(std::list<LoopConstRef>::const_iterator loop = loops.begin(); loop != loop_end; ++loop)
   {
      /// FIXME: zero loop
      if((*loop)->GetId() == 0)
         continue;
      const CustomUnorderedSet<vertex> blocks = (*loop)->get_blocks();
      CustomUnorderedSet<vertex>::const_iterator bb_it, bb_it_end = blocks.end();
      for(bb_it = blocks.begin(); bb_it != bb_it_end; ++bb_it)
      {
         const BBNodeInfoRef bb_node_info = fbb->GetBBNodeInfo(*bb_it);
         bb_node_info->loop_id = (*loop)->GetId();
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Basic block " + boost::lexical_cast<std::string>(bb_node_info->block->number));
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Number of reducible loops: " + boost::lexical_cast<std::string>(function_behavior->CGetLoops()->NumLoops()));
   return DesignFlowStep_Status::SUCCESS;
}
