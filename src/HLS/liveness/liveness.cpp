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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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

liveness::liveness(const HLS_managerRef _HLSMgr, const ParameterConstRef _Param)
    : TreeM(_HLSMgr->get_tree_manager()), Param(_Param), null_vertex_string("NULL_VERTEX"), HLSMgr(_HLSMgr)

{
}

liveness::~liveness() = default;

bool liveness::is_defined(unsigned int var) const
{
   if(var_op_definition.find(var) != var_op_definition.end())
   {
      return true;
   }

   return false;
}

void liveness::set_live_in(const vertex& v, unsigned int var, unsigned int step)
{
   live_in[v].insert(std::make_pair(var, step));
}

void liveness::set_live_in(const vertex& v, const CustomOrderedSet<std::pair<unsigned int, unsigned int>>& live_set)
{
   live_in[v].insert(live_set.begin(), live_set.end());
}

void liveness::set_live_in(const vertex& v,
                           const CustomOrderedSet<std::pair<unsigned int, unsigned int>>::const_iterator first,
                           const CustomOrderedSet<std::pair<unsigned int, unsigned int>>::const_iterator last)
{
   live_in[v].insert(first, last);
}

void liveness::erase_el_live_in(const vertex& v, unsigned int var, unsigned int step)
{
   live_in[v].erase(std::make_pair(var, step));
}

const CustomOrderedSet<std::pair<unsigned int, unsigned int>>& liveness::get_live_in(const vertex& v) const
{
   if(live_in.find(v) != live_in.end())
   {
      return live_in.find(v)->second;
   }
   else
   {
      return empty_set;
   }
}

void liveness::set_live_out(const vertex& v, unsigned int var, unsigned int step)
{
   live_out[v].insert(std::make_pair(var, step));
}

void liveness::set_live_out(const vertex& v, const CustomOrderedSet<std::pair<unsigned int, unsigned int>>& vars)
{
   live_out[v].insert(vars.begin(), vars.end());
}

void liveness::set_live_out(const vertex& v,
                            const CustomOrderedSet<std::pair<unsigned int, unsigned int>>::const_iterator first,
                            const CustomOrderedSet<std::pair<unsigned int, unsigned int>>::const_iterator last)
{
   live_out[v].insert(first, last);
}

void liveness::erase_el_live_out(const vertex& v, unsigned int var, unsigned int step)
{
   live_out[v].erase(std::make_pair(var, step));
}

const CustomOrderedSet<std::pair<unsigned int, unsigned int>>& liveness::get_live_out(const vertex& v) const
{
   if(live_out.find(v) != live_out.end())
   {
      return live_out.find(v)->second;
   }
   else
   {
      return empty_set;
   }
}

vertex liveness::get_op_where_defined(unsigned int var) const
{
   THROW_ASSERT(var_op_definition.find(var) != var_op_definition.end(),
                "var never defined " + TreeM->get_tree_node_const(var)->ToString());
   return var_op_definition.find(var)->second;
}

bool liveness::has_op_where_defined(unsigned int var) const
{
   return (var_op_definition.find(var) != var_op_definition.end());
}

const CustomOrderedSet<vertex>& liveness::get_state_in(vertex state, vertex op, unsigned int var) const
{
   THROW_ASSERT(state_in_definitions.find(state) != state_in_definitions.end(), "state never used " + get_name(state));
   THROW_ASSERT(state_in_definitions.find(state)->second.find(op) != state_in_definitions.find(state)->second.end(),
                "op never used in state " + get_name(state));
   THROW_ASSERT(state_in_definitions.find(state)->second.find(op)->second.find(var) !=
                    state_in_definitions.find(state)->second.find(op)->second.end(),
                "var never used in the given state. Var: " + std::to_string(var));
   return state_in_definitions.find(state)->second.find(op)->second.find(var)->second;
}

bool liveness::has_state_in(vertex state, vertex op, unsigned int var) const
{
   if(state_in_definitions.find(state) == state_in_definitions.end())
   {
      return false;
   }
   if(state_in_definitions.find(state)->second.find(op) == state_in_definitions.find(state)->second.end())
   {
      return false;
   }
   if(state_in_definitions.find(state)->second.find(op)->second.find(var) ==
      state_in_definitions.find(state)->second.find(op)->second.end())
   {
      return false;
   }
   return true;
}

void liveness::add_state_in_for_var(unsigned int var, vertex op, vertex state, vertex state_in)
{
   state_in_definitions[state][op][var].insert(state_in);
}

const CustomOrderedSet<vertex>& liveness::get_state_out(vertex state, vertex op, unsigned int var) const
{
   THROW_ASSERT(state_out_definitions.find(state) != state_out_definitions.end(),
                "state never used " + get_name(state));
   THROW_ASSERT(state_out_definitions.find(state)->second.find(op) != state_out_definitions.find(state)->second.end(),
                "op never used in state " + get_name(state));
   THROW_ASSERT(state_out_definitions.find(state)->second.find(op)->second.find(var) !=
                    state_out_definitions.find(state)->second.find(op)->second.end(),
                "var never used in the given state. Var: " + std::to_string(var));
   return state_out_definitions.find(state)->second.find(op)->second.find(var)->second;
}

bool liveness::has_state_out(vertex state, vertex op, unsigned int var) const
{
   if(state_out_definitions.find(state) == state_out_definitions.end())
   {
      return false;
   }
   if(state_out_definitions.find(state)->second.find(op) == state_out_definitions.find(state)->second.end())
   {
      return false;
   }
   if(state_out_definitions.find(state)->second.find(op)->second.find(var) ==
      state_out_definitions.find(state)->second.find(op)->second.end())
   {
      return false;
   }
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
   {
      return null_vertex_string;
   }
   THROW_ASSERT(names.find(v) != names.end(), "state without a name");
   return names.find(v)->second;
}

bool liveness::are_in_conflict(vertex op1, vertex op2) const
{
   const CustomOrderedSet<vertex>& op1_run = get_state_where_run(op1);
   const CustomOrderedSet<vertex>& op2_run = get_state_where_run(op2);

   for(const auto s1 : op1_run)
   {
      if(op2_run.find(s1) != op2_run.end())
      {
         return true;
      }
   }
   return false;

#if 0 && HAVE_EXPERIMENTAL
   else
   {
      auto FB = HLSMgr->GetFunctionBehavior(HLS->functionId);
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
   THROW_ASSERT(start_op.find(state) != start_op.end(),
                "start_op map does not have this chained vertex " + get_name(state));
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
         {
            return current_res;
         }
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
         {
            return current_res;
         }
      }
      return true;
   }
}

unsigned liveness::get_step(vertex v, vertex op, unsigned int var, bool in) const
{
   unsigned int step = 0;
   if(in)
   {
      auto running_op = op;
      if(vertex_to_op_step_in_map.count(v))
      {
         if(var_op_definition.count(var))
         {
            auto def_op = var_op_definition.at(var);
            THROW_ASSERT(ending_operations.count(def_op), "unexpected condition");
            auto& def_state = *ending_operations.at(def_op).begin();
            if(vertex_to_op_step_out_map.count(def_state))
            {
               THROW_ASSERT(vertex_to_op_step_in_map.at(v).count(running_op), "unexpected condition");
               step = vertex_to_op_step_in_map.at(v).at(running_op);
            }
         }
      }
      else
      {
         auto def_op = var_op_definition.at(var);
         THROW_ASSERT(ending_operations.count(def_op), "unexpected condition");
         auto& def_state = *ending_operations.at(def_op).begin();
         if(vertex_to_op_step_out_map.count(def_state))
         {
            THROW_ASSERT(vertex_to_op_step_out_map.at(def_state).count(def_op), "unexpected condition");
            step = vertex_to_op_step_out_map.at(def_state).at(def_op);
         }
      }
   }
   else
   {
      auto def_op = op;
      if(vertex_to_op_step_out_map.count(v))
      {
         THROW_ASSERT(vertex_to_op_step_out_map.at(v).count(def_op), "unexpected condition");
         step = vertex_to_op_step_out_map.at(v).at(def_op);
      }
   }
   return step;
}

unsigned liveness::get_prev_step(unsigned int var, unsigned cur_step) const
{
   auto step = cur_step;
   if(var_op_definition.count(var))
   {
      auto def_op = var_op_definition.at(var);
      THROW_ASSERT(ending_operations.count(def_op), "unexpected condition");
      auto& def_state = *ending_operations.at(def_op).begin();
      if(vertex_to_op_step_out_map.count(def_state))
      {
         auto def_step = vertex_to_op_step_out_map.at(def_state).at(def_op);
         if(def_step < cur_step)
         {
            return cur_step - 1;
         }
      }
   }
   return step;
}

void liveness::set_step(vertex v, vertex running_op, unsigned int step, bool in)
{
   if(in)
   {
      vertex_to_op_step_in_map[v][running_op] = step;
   }
   else
   {
      vertex_to_op_step_out_map[v][running_op] = step;
   }
}
