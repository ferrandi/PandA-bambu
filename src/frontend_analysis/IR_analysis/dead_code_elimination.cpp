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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @file dead_code_elimination.cpp
 * @brief Eliminates unuseful definitions
 *
 * @author Andrea Cuoccio <andrea.cuoccio@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/

///header include
#include "dead_code_elimination.hpp"

///STD include
#include <fstream>

///STL include
#include <map>
#include <vector>
#include <string>

#include "application_manager.hpp"

// includes from behavior
#include "call_graph_manager.hpp"

///Tree include
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "tree_basic_block.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

dead_code_elimination::dead_code_elimination(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager) :
   FunctionFrontendFlowStep(_AppM, _function_id, DEAD_CODE_ELIMINATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

dead_code_elimination::~dead_code_elimination()
{}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship> > dead_code_elimination::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP) :
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP) :
      {
#if HAVE_ZEBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ARRAY_REF_FIX, SAME_FUNCTION));
#endif
         break;
      }
      case(INVALIDATION_RELATIONSHIP) :
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

///single sweep analisys, block by block, from the bottom to up. Each ssa which is used zero times is eliminated and the uses of the variables used in the assignment are recompuded
DesignFlowStep_Status dead_code_elimination::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();

   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   function_decl * fd = GetPointer<function_decl>(curr_tn);
   statement_list * sl = GetPointer<statement_list>(GET_NODE(fd->body));
   ///Retrive the list of block
   std::map<unsigned int, blocRef> &blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator block_it, block_it_end;
   block_it_end = blocks.end();

   bool modified = false;
   bool restart_analysis;
   do
   {
      restart_analysis = false;
      for (block_it = blocks.begin();block_it != block_it_end; ++block_it)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing BB" + boost::lexical_cast<std::string>(block_it->second->number));
         ///Retrive the list of statement of the block
         const auto & stmt_list = block_it->second->CGetStmtList();
         std::list<tree_nodeRef> stmts_to_be_removed;
         ///for each statement, if it is a gimple_assign there could be an indirect_ref in both of the operands
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing assignments");
         for(auto stmt = stmt_list.rbegin(); stmt != stmt_list.rend(); stmt++)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
               break;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*stmt)->ToString());
            ///find out if it is a gimple_assign
            if (GET_NODE(*stmt)->get_kind() == gimple_assign_K)
            {
               auto ga = GetPointer<gimple_assign>(GET_NODE(*stmt));
               ///in case of virtual uses it is better not perform the elimination
               if (not ga->vdef)
               {
                  ///op0 is the left side of the assignemt, op1 is the right side
                  const tree_nodeRef op0 = GET_NODE(ga->op0);
                  if (op0->get_kind() == ssa_name_K)
                  {
                     ssa_name *ssa = GetPointer<ssa_name>(op0);
                     ///very strict condition for the elimination
                     if (ssa->CGetNumberUses() == 0 and ssa->CGetDefStmts().size() == 1)
                     {
                        stmts_to_be_removed.push_back(*stmt);
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), *stmt);
#endif
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed assignments");
         if(stmts_to_be_removed.size())
         {
            modified = true;
            restart_analysis = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(stmts_to_be_removed.size()) + " dead assignments");
         for(auto curr_el : stmts_to_be_removed)
         {
            gimple_assign * ga =  GetPointer<gimple_assign>(GET_NODE(curr_el));
            if (ga and (GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC,  debug_level, "---Call expr in the right part can be removed");
               const CallGraphManagerRef cg_man = AppM->GetCallGraphManager();
               const vertex fun_cg_vertex = cg_man->GetVertex(function_id);
               const CallGraphConstRef cg = cg_man->CGetCallGraph();
               std::set<EdgeDescriptor> to_remove;
               OutEdgeIterator oei, oei_end;
               boost::tie(oei, oei_end) = boost::out_edges(fun_cg_vertex, *cg);
               const unsigned int call_id = GET_INDEX_NODE(curr_el);
               for (; oei != oei_end; oei++)
               {
                  const std::set<unsigned int> & direct_calls = cg->CGetFunctionEdgeInfo(*oei)->direct_call_points;
                  const std::set<unsigned int>::iterator call_it = direct_calls.find(call_id);
                  if (call_it != direct_calls.end())
                  {
                     to_remove.insert(*oei);
                  }
               }
               THROW_ASSERT(to_remove.size(), "Call to be removed not found in call graph");
               for (const EdgeDescriptor & e : to_remove)
               {
                  cg_man->RemoveCallPoint(e, call_id);
               }
               if (parameters->getOption<bool>(OPT_print_dot) or debug_level > DEBUG_LEVEL_PEDANTIC)
               {
                  AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph" + GetSignature() + ".dot");
               }
            }
            block_it->second->RemoveStmt(curr_el);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + STR(curr_el));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead assignments");
         /*
          * check also phi operations. if a phi assigns an ssa which is not used
          * anymore, the phi can be removed
          */
         const auto & phi_list = block_it->second->CGetPhiList();
         std::list<tree_nodeRef> phis_to_be_removed;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing phis");
         for(auto phi = phi_list.rbegin(); phi != phi_list.rend(); phi++)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
               break;
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*phi)->ToString());
            THROW_ASSERT(GET_NODE(*phi)->get_kind() == gimple_phi_K,
               GET_NODE(*phi)->ToString() + " is of kind " + tree_node::GetString(GET_NODE(*phi)->get_kind()));
            auto gphi = GetPointer<gimple_phi>(GET_NODE(*phi));
            const tree_nodeRef res = GET_NODE(gphi->res);
            THROW_ASSERT(res->get_kind() == ssa_name_K,
               res->ToString() + " is of kind " + tree_node::GetString(res->get_kind()));
            const ssa_name * ssa = GetPointer<ssa_name>(res);
            // very strict condition for the elimination
            if (ssa->CGetNumberUses() == 0 and ssa->CGetDefStmts().size() == 1)
            {
               phis_to_be_removed.push_back(*phi);
#ifndef NDEBUG
               AppM->RegisterTransformation(GetName(), *phi);
#endif
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phi");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed phis");
         if(phis_to_be_removed.size())
         {
            modified = true;
            restart_analysis = true;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Removing " + STR(phis_to_be_removed.size()) + " dead phis");
         for(auto curr_phi : phis_to_be_removed)
         {
            block_it->second->RemovePhi(curr_phi);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + STR(curr_phi));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Removed dead phis");

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed BB" + boost::lexical_cast<std::string>(block_it->second->number));
      }
   }
   while (restart_analysis);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "End of dead_code_elimination step");
   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   else
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
}
