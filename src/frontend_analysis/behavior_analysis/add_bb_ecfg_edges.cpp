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
 * @file add_bb_ecfg_edges.cpp
 * @brief Analysis step which extends basic blocks cfg
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "add_bb_ecfg_edges.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "tree_basic_block.hpp"

/// Graph include
#include "graph.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STL include
#include "custom_set.hpp"
#include <list>

/// Utility include
#include "boost/lexical_cast.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AddBbEcfgEdges::AddBbEcfgEdges(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, ADD_BB_ECFG_EDGES, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

AddBbEcfgEdges::~AddBbEcfgEdges() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> AddBbEcfgEdges::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_FEEDBACK_EDGES_IDENTIFICATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BB_ORDER_COMPUTATION, SAME_FUNCTION));
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

DesignFlowStep_Status AddBbEcfgEdges::InternalExec()
{
   /// The behavioral helper
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();

   /// The function name
#ifndef NDEBUG
   const std::string function_name = behavioral_helper->get_function_name();
#endif
   /// The control flow graph with feedback of basic blocks
   const BBGraphRef fbb = function_behavior->GetBBGraph(FunctionBehavior::FBB);

   /// The loop structure
   const std::list<LoopConstRef>& loops = function_behavior->CGetLoops()->GetList();

   /// Adding edges in basic block graphs from sources of feedback edges to landing pads
   std::list<LoopConstRef>::const_iterator loop, loop_end = loops.end();
   for(loop = loops.begin(); loop != loop_end; ++loop)
   {
      if((*loop)->GetId() == 0)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering loop " + boost::lexical_cast<std::string>((*loop)->GetId()));

      /// add Extended edges to manage in/out dependencies when we have multi-entries in a loop (aka irreducible loop)
      CustomUnorderedSet<vertex> loop_bbs;
      (*loop)->get_recursively_bb(loop_bbs);
      for(auto cur_bb1 : (*loop)->get_entries())
      {
         for(auto cur_bb2 : (*loop)->get_entries())
         {
            if(cur_bb1 != cur_bb2)
            {
               InEdgeIterator ie, ie_end;
               for(boost::tie(ie, ie_end) = boost::in_edges(cur_bb1, *fbb); ie != ie_end; ie++)
               {
                  vertex source = boost::source(*ie, *fbb);
                  if(loop_bbs.find(source) == loop_bbs.end())
                  {
                     function_behavior->bbgc->AddEdge(source, cur_bb2, ECFG_SELECTOR);
#ifndef NDEBUG
                     if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                     {
                        try
                        {
                           const BBGraphRef ebb_graph = function_behavior->GetBBGraph(FunctionBehavior::EBB);
                           std::list<vertex> vertices;
                           ebb_graph->TopologicalSort(vertices);
                        }
                        catch(const char* msg)
                        {
                           THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
                        }
                        catch(const std::string& msg)
                        {
                           THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
                        }
                        catch(const std::exception& ex)
                        {
                           THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
                        }
                        catch(...)
                        {
                           THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
                        }
                     }
#endif
                  }
               }
            }
         }
      }

      /// Sources of feedback loop
      CustomUnorderedSet<vertex> sources;

      /// The targets of the flow edges they can be different from landing_pads of this loop if the edge which connects a block
      /// of the loop to a landing_pads is the feedback edge of an external loop. In this case the block must be connected to the
      /// landing pads of the external loop
      CustomUnorderedSet<vertex> targets;

      /// compute sources
      for(auto sp_back_edge : (*loop)->get_sp_back_edges())
      {
         /// Check if the target belongs to the current loop
         if(fbb->CGetBBNodeInfo(sp_back_edge.second)->loop_id == (*loop)->GetId())
         {
            sources.insert(sp_back_edge.first);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Found source BB" + boost::lexical_cast<std::string>(fbb->CGetBBNodeInfo(sp_back_edge.first)->block->number) + " (Target is BB" + STR(fbb->CGetBBNodeInfo(sp_back_edge.second)->block->number) + ")");
         }
      }
      /// Landing pads
      CustomUnorderedSet<vertex> landing_pads = (*loop)->GetLandingPadBlocks();
      LoopConstRef other_loop = *loop;
      /// While at least one landing pad of the current loop or of one of its ancestor is reached with a feedback edge
      while([&]() -> bool {
         for(const auto landing_pad : landing_pads)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing landing pad " + STR(fbb->CGetBBNodeInfo(landing_pad)->block->number));
            InEdgeIterator ie, ie_end;
            for(boost::tie(ie, ie_end) = boost::in_edges(landing_pad, *fbb); ie != ie_end; ie++)
            {
               const auto source = boost::source(*ie, *fbb);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing Edge BB" + STR(fbb->CGetBBNodeInfo(source)->block->number) + "-->BB" + STR(fbb->CGetBBNodeInfo(landing_pad)->block->number));
               if(fbb->GetSelector(*ie) & FB_CFG_SELECTOR)
               {
                  if(fbb->CGetBBNodeInfo(source)->loop_id == other_loop->GetId())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Going to landing pad BB" + STR(fbb->CGetBBNodeInfo(landing_pad)->block->number) + " of loop " + STR(other_loop->GetId()) + " is feedback edge. Going up one level");
                     return true;
                  }
                  else
                  {
                     CustomUnorderedSet<vertex> bb_loops;
                     other_loop->get_recursively_bb(bb_loops);
                     if(bb_loops.find(source) != bb_loops.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Going to landing pad BB" + STR(fbb->CGetBBNodeInfo(landing_pad)->block->number) + " of loop " + STR(other_loop->GetId()) + " is feedback edge. Going up one level");
                        return true;
                     }
                  }
               }
            }
         }
         return false;
      }())
      {
         other_loop = other_loop->Parent();
         landing_pads = other_loop->GetLandingPadBlocks();
      }
      targets.insert(landing_pads.begin(), landing_pads.end());

      CustomUnorderedSet<vertex>::const_iterator s, s_end = sources.end(), t, t_end = targets.end();
      for(s = sources.begin(); s != s_end; ++s)
      {
         for(t = targets.begin(); t != t_end; ++t)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding edge from BB" + boost::lexical_cast<std::string>(fbb->CGetBBNodeInfo(*s)->block->number) + " to BB" + boost::lexical_cast<std::string>(fbb->CGetBBNodeInfo(*t)->block->number));
            function_behavior->bbgc->AddEdge(*s, *t, ECFG_SELECTOR);
#ifndef NDEBUG
            if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
            {
               try
               {
                  const BBGraphRef ebb_graph = function_behavior->GetBBGraph(FunctionBehavior::EBB);
                  std::list<vertex> vertices;
                  ebb_graph->TopologicalSort(vertices);
               }
               catch(const char* msg)
               {
                  THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
               }
               catch(const std::string& msg)
               {
                  THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
               }
               catch(const std::exception& ex)
               {
                  THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
               }
               catch(...)
               {
                  THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
               }
            }
#endif
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered loop " + boost::lexical_cast<std::string>((*loop)->GetId()));
   }

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::EBB)->WriteDot("BB_EBB.dot");
   }

#ifndef NDEBUG
   try
   {
      const BBGraphRef ebb_graph = function_behavior->GetBBGraph(FunctionBehavior::EBB);
      std::list<vertex> vertices;
      ebb_graph->TopologicalSort(vertices);
   }
   catch(const char* msg)
   {
      THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::string& msg)
   {
      THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::exception& ex)
   {
      THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
   }
   catch(...)
   {
      THROW_UNREACHABLE("ecfg graph of function " + function_name + " is not acyclic");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
