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
 * @file liveness.cpp
 * @brief Class used to store liveness results
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Francesca Malcotti <francy_malco@virgilio.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "liveness.hpp"

///. include
#include "Parameter.hpp"
#include "constant_strings.hpp"

/// behavior include
#include "basic_block.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

/// tree includes
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "loop.hpp"
#include "loops.hpp"

#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

liveness::liveness(const HLS_managerRef _HLSMgr, const ParameterConstRef _Param) : TreeM(_HLSMgr->get_tree_manager()), Param(_Param), null_vertex_string("NULL_VERTEX"), HLSMgr(_HLSMgr)

{
}

liveness::~liveness() = default;

bool liveness::is_defined(unsigned int var) const
{
   if(var_op_definition.find(var) != var_op_definition.end())
      return true;

   return false;
}

void liveness::set_live_in(const vertex& v, unsigned int var)
{
   live_in[v].insert(var);
}

void liveness::set_live_in(const vertex& v, const CustomOrderedSet<unsigned int>& live_set)
{
   live_in[v].insert(live_set.begin(), live_set.end());
}

void liveness::set_live_in(const vertex& v, const CustomOrderedSet<unsigned int>::const_iterator first, const CustomOrderedSet<unsigned int>::const_iterator last)
{
   live_in[v].insert(first, last);
}

void liveness::erase_el_live_in(const vertex& v, unsigned int var)
{
   live_in[v].erase(var);
}

const CustomOrderedSet<unsigned int>& liveness::get_live_in(const vertex& v) const
{
   if(live_in.find(v) != live_in.end())
      return live_in.find(v)->second;
   else
      return empty_set;
}

void liveness::set_live_out(const vertex& v, unsigned int var)
{
   live_out[v].insert(var);
}

void liveness::set_live_out(const vertex& v, const CustomOrderedSet<unsigned int>& vars)
{
   live_out[v].insert(vars.begin(), vars.end());
}

void liveness::set_live_out(const vertex& v, const CustomOrderedSet<unsigned int>::const_iterator first, const CustomOrderedSet<unsigned int>::const_iterator last)
{
   live_out[v].insert(first, last);
}

void liveness::erase_el_live_out(const vertex& v, unsigned int var)
{
   live_out[v].erase(var);
}

const CustomOrderedSet<unsigned int>& liveness::get_live_out(const vertex& v) const
{
   if(live_out.find(v) != live_out.end())
      return live_out.find(v)->second;
   else
      return empty_set;
}

vertex liveness::get_op_where_defined(unsigned int var) const
{
   THROW_ASSERT(var_op_definition.find(var) != var_op_definition.end(), "var never defined " + TreeM->get_tree_node_const(var)->ToString());
   return var_op_definition.find(var)->second;
}

bool liveness::has_op_where_defined(unsigned int var) const
{
   return (var_op_definition.find(var) != var_op_definition.end());
}

const CustomOrderedSet<vertex>& liveness::get_state_in(vertex state, vertex op, unsigned int var) const
{
   THROW_ASSERT(state_in_definitions.find(state) != state_in_definitions.end(), "state never used " + get_name(state));
   THROW_ASSERT(state_in_definitions.find(state)->second.find(op) != state_in_definitions.find(state)->second.end(), "op never used in state " + get_name(state));
   THROW_ASSERT(state_in_definitions.find(state)->second.find(op)->second.find(var) != state_in_definitions.find(state)->second.find(op)->second.end(), "var never used in the given state. Var: " + std::to_string(var));
   return state_in_definitions.find(state)->second.find(op)->second.find(var)->second;
}

bool liveness::has_state_in(vertex state, vertex op, unsigned int var) const
{
   if(state_in_definitions.find(state) == state_in_definitions.end())
      return false;
   if(state_in_definitions.find(state)->second.find(op) == state_in_definitions.find(state)->second.end())
      return false;
   if(state_in_definitions.find(state)->second.find(op)->second.find(var) == state_in_definitions.find(state)->second.find(op)->second.end())
      return false;
   return true;
}

void liveness::add_state_in_for_var(unsigned int var, vertex op, vertex state, vertex state_in)
{
   state_in_definitions[state][op][var].insert(state_in);
}

const CustomOrderedSet<vertex>& liveness::get_state_out(vertex state, vertex op, unsigned int var) const
{
   THROW_ASSERT(state_out_definitions.find(state) != state_out_definitions.end(), "state never used " + get_name(state));
   THROW_ASSERT(state_out_definitions.find(state)->second.find(op) != state_out_definitions.find(state)->second.end(), "op never used in state " + get_name(state));
   THROW_ASSERT(state_out_definitions.find(state)->second.find(op)->second.find(var) != state_out_definitions.find(state)->second.find(op)->second.end(), "var never used in the given state. Var: " + std::to_string(var));
   return state_out_definitions.find(state)->second.find(op)->second.find(var)->second;
}

bool liveness::has_state_out(vertex state, vertex op, unsigned int var) const
{
   if(state_out_definitions.find(state) == state_out_definitions.end())
      return false;
   if(state_out_definitions.find(state)->second.find(op) == state_out_definitions.find(state)->second.end())
      return false;
   if(state_out_definitions.find(state)->second.find(op)->second.find(var) == state_out_definitions.find(state)->second.find(op)->second.end())
      return false;
   return true;
}

void liveness::add_state_out_for_var(unsigned int var, vertex op, vertex state, vertex state_in)
{
   state_out_definitions[state][op][var].insert(state_in);
}

const CustomOrderedSet<vertex>& liveness::get_state_where_end(vertex op) const
{
   THROW_ASSERT(ending_operations.find(op) != ending_operations.end(), "op never ending in a state ");
   return ending_operations.find(op)->second;
}

const CustomOrderedSet<vertex>& liveness::get_state_where_run(vertex op) const
{
   THROW_ASSERT(running_operations.find(op) != running_operations.end(), "op never running in a state ");
   return running_operations.find(op)->second;
}

const std::string& liveness::get_name(vertex v) const
{
   if(v == NULL_VERTEX)
      return null_vertex_string;
   THROW_ASSERT(names.find(v) != names.end(), "state without a name");
   return names.find(v)->second;
}

bool liveness::are_in_conflict(vertex op1, vertex op2) const
{
   // if(!HLS)
   const CustomOrderedSet<vertex>& op1_run = get_state_where_run(op1);
   const CustomOrderedSet<vertex>& op2_run = get_state_where_run(op2);

   auto FB = HLSMgr->GetFunctionBehavior(HLS->functionId);
   if(FB->is_pipelining_enabled() && !FB->build_simple_pipeline())
   {
      const OpGraphConstRef dfg = FB->CGetOpGraph(FunctionBehavior::DFG);
      unsigned int bb_index1 = GET_BB_INDEX(dfg, op1);
      unsigned int bb_index2 = GET_BB_INDEX(dfg, op2);
      const CustomUnorderedMap<unsigned int, vertex>& bb_index_map = FB->CGetBBGraph(FunctionBehavior::FBB)->CGetBBGraphInfo()->bb_index_map;
      vertex bb_1 = bb_index_map.find(bb_index1)->second;
      vertex bb_2 = bb_index_map.find(bb_index2)->second;

      auto loops = HLSMgr->GetFunctionBehavior(HLS->functionId)->GetLoops()->GetList();
      for(auto loop : loops)
      {
         int initiation_time = FB->get_initiation_time();
         THROW_ASSERT(loop->num_blocks() != 1, "The loop has more than one basic block");
         auto bbs = loop->get_blocks();
         for(auto bb : bbs)
         {
            for(auto s_pair : HLS->STG->GetAstg()->GetStateTransitionGraphInfo()->vertex_to_state_id)
            {
               auto ids = HLS->STG->CGetAstg()->CGetStateInfo(std::get<0>(s_pair))->BB_ids;
               for(auto id : ids)
               {
                  if(id == HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBBGraph()->CGetBBNodeInfo(bb)->get_bb_index())
                  {
                     if(HLS->STG->GetAstg()->GetStateInfo(std::get<0>(s_pair))->loopId != 0 && HLS->STG->GetAstg()->GetStateInfo(std::get<0>(s_pair))->loopId != HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBBGraph()->CGetBBNodeInfo(bb)->loop_id)
                        THROW_ERROR("Attempting to change the loopId of state " + HLS->STG->GetAstg()->GetStateInfo(std::get<0>(s_pair))->name);
                     HLS->STG->GetAstg()->GetStateInfo(std::get<0>(s_pair))->loopId = HLSMgr->CGetFunctionBehavior(HLS->functionId)->CGetBBGraph()->CGetBBNodeInfo(bb)->loop_id;
                  }
               }
            }
         }

         bool cond1 = false;
         bool cond2 = false;
         if(bbs.find(bb_1) != bbs.end())
         {
            cond1 = true;
            for(auto s1 : op1_run)
            {
               auto info = HLS->STG->GetAstg()->GetStateInfo(s1);
               THROW_ASSERT(info->loopId == 0 || info->loopId == loop->GetId(), "The same operation is performed in multiple loops");
            }
         }
         if(bbs.find(bb_2) != bbs.end())
         {
            cond2 = true;
            for(auto s2 : op2_run)
            {
               auto info = HLS->STG->GetAstg()->GetStateInfo(s2);
               THROW_ASSERT(info->loopId == 0 || info->loopId == loop->GetId(), "The same operation is performed in multiple loops");
            }
         }

         if(cond1 && cond2)
         {
            auto stg = HLS->STG->GetAstg();
            for(auto s1 : op1_run)
            {
               std::queue<vertex> to_analyze;
               std::set<vertex> analyzed;
               std::queue<vertex> next_frontier;
               to_analyze.push(s1);
               vertex src;
               int distance = 1;
               graph::out_edge_iterator out_edge, out_edge_end;
               while(to_analyze.size() > 0)
               {
                  src = to_analyze.front();
                  to_analyze.pop();
                  analyzed.insert(src);
                  for(boost::tie(out_edge, out_edge_end) = boost::out_edges(src, *stg); out_edge != out_edge_end; ++out_edge)
                  {
                     vertex tgt = boost::target(*out_edge, *stg);
                     if(op1_run.find(tgt) != op1_run.end())
                     {
                        continue;
                     }

                     if(op2_run.find(tgt) != op2_run.end())
                     {
                        if(distance % initiation_time == 0)
                           return false;
                        continue;
                     }

                     if(analyzed.find(tgt) != analyzed.end())
                     {
                        next_frontier.push(tgt);
                     }
                  }
                  if(to_analyze.size() == 0)
                  {
                     to_analyze = next_frontier;
                     distance++;
                  }
               }
            }
         }
      }
   }

   {
      const CustomOrderedSet<vertex>::const_iterator op1_run_it_end = op1_run.end();
      for(auto op1_run_it = op1_run.begin(); op1_run_it != op1_run_it_end; ++op1_run_it)
         if(op2_run.find(*op1_run_it) != op2_run.end())
            return true;
      return false;
   }
#if 0 && HAVE_EXPERIMENTAL
   else
   {
      const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
      if(std::find(in_conflict_ops[op1].begin(), in_conflict_ops[op1].end(), op2)!= in_conflict_ops[op1].end())
         return true;
      else if(std::find(compatible_ops[op1].begin(), compatible_ops[op1].end(), op2)!= compatible_ops[op1].end())
         return false;
      else
      {
         const OpGraphConstRef dfg = FB->CGetOpGraph(FunctionBehavior::DFG);
         unsigned int bb_index1 = GET_BB_INDEX(dfg, op1);
         unsigned int bb_index2 = GET_BB_INDEX(dfg, op2);
         const CustomUnorderedMap<unsigned int, vertex> & bb_index_map = FB->CGetBBGraph(FunctionBehavior::FBB)->CGetBBGraphInfo()->bb_index_map;
         vertex bb_1 = bb_index_map.find(bb_index1)->second;
         vertex bb_2 = bb_index_map.find(bb_index2)->second;
         if(bb_1 != bb_2 && non_in_parallel(bb_1, bb_2, FB->CGetBBGraph(FunctionBehavior::CDG_BB)))
         {
            compatible_ops[op1].insert(op2);
            compatible_ops[op2].insert(op1);
            return false;
         }
         unsigned int li_1 = FB->CGetBBGraph(FunctionBehavior::FBB)->CGetBBNodeInfo(bb_1)->loop_id;
         unsigned int li_2 = FB->CGetBBGraph(FunctionBehavior::FBB)->CGetBBNodeInfo(bb_2)->loop_id;
         unsigned int l_depth1 = FB->CGetLoops()->CGetLoop(li_1)->depth;
         unsigned int l_depth2 = FB->CGetLoops()->CGetLoop(li_2)->depth;
         if(li_1 == li_2)
         {
            const OpGraphConstRef saodg = FB->CGetOpGraph(FunctionBehavior::FLSAODG);
            if(!((saodg->IsReachable(op1, op2) && FB->CheckReachability(op1, op2)) || (saodg->IsReachable(op2, op1)  && FB->CheckReachability(op2, op1))))
            {
               in_conflict_ops[op1].insert(op2);
               in_conflict_ops[op2].insert(op1);
               return true;
            }
            else
            {
               compatible_ops[op1].insert(op2);
               compatible_ops[op2].insert(op1);
               return false;
            }
         }
         else if(l_depth1 == l_depth2 && l_depth1 == 1)
         {
            const OpGraphConstRef fsaodg = FB->CGetOpGraph(FunctionBehavior::FFLSAODG);

            if(!((fsaodg->IsReachable(op1, op2) && FB->CheckReachability(op1, op2)) || (fsaodg->IsReachable(op2, op1)  && FB->CheckReachability(op2, op1))))
            {
               in_conflict_ops[op1].insert(op2);
               in_conflict_ops[op2].insert(op1);
               return true;
            }
            else
            {
               compatible_ops[op1].insert(op2);
               compatible_ops[op2].insert(op1);
               return false;
            }
         }
         else
            THROW_ERROR("unexpected pattern");
         return false;
      }
   }
#else
   return false;

#endif
}

vertex liveness::get_start_op(vertex state) const
{
   THROW_ASSERT(start_op.find(state) != start_op.end(), "start_op map does not have this chained vertex " + get_name(state));
   return start_op.find(state)->second;
}

void liveness::set_start_op(vertex state, vertex op)
{
   start_op[state] = op;
}

bool liveness::non_in_parallel(vertex v1, vertex v2, const BBGraphConstRef cdg) const
{
   if(cdg->CGetBBNodeInfo(v1)->cer == cdg->CGetBBNodeInfo(v2)->cer)
   {
      return v1 == v2;
   }
   else if(cdg->CGetBBNodeInfo(v1)->cer > cdg->CGetBBNodeInfo(v2)->cer)
   {
      InEdgeIterator ie_it, ie_it_end;
      for(boost::tie(ie_it, ie_it_end) = boost::in_edges(v1, *cdg); ie_it != ie_it_end; ++ie_it)
      {
         vertex cer0_v1 = boost::source(*ie_it, *cdg);
         bool current_res = non_in_parallel(cer0_v1, v2, cdg);
         if(!current_res)
            return current_res;
      }
      return true;
   }
   else
   {
      InEdgeIterator ie_it, ie_it_end;
      for(boost::tie(ie_it, ie_it_end) = boost::in_edges(v2, *cdg); ie_it != ie_it_end; ++ie_it)
      {
         vertex cer0_v2 = boost::source(*ie_it, *cdg);
         bool current_res = non_in_parallel(v1, cer0_v2, cdg);
         if(!current_res)
            return current_res;
      }
      return true;
   }
}
