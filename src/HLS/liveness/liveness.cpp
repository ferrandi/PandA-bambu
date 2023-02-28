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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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

#include "state_transition_graph_manager.hpp"

liveness::liveness(const HLS_managerRef _HLSMgr, const ParameterConstRef _Param)
    : TreeM(_HLSMgr->get_tree_manager()), Param(_Param), null_vertex_string("NULL_VERTEX"), HLSMgr(_HLSMgr)

{
}

liveness::~liveness() = default;

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
}

unsigned liveness::satStep(unsigned BB_index, unsigned step) const
{
   return std::min(BB2MaxStep.at(BB_index) + 1, step);
}

unsigned liveness::get_step(vertex v, vertex op, unsigned int var, bool in) const
{
   unsigned int step = 0;
   if(in)
   {
      THROW_ASSERT(!phi_vertices.count(op), "unexpected condition");
      auto running_op = op;
      if(vertex_to_op_step_in_map.count(v))
      {
         if(var_op_definition.count(var))
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
            else
            {
               /// parameters are "not" having a def_op
               THROW_ASSERT(vertex_to_op_step_in_map.at(v).count(running_op), "unexpected condition");
               step = vertex_to_op_step_in_map.at(v).at(running_op);
            }
         }
      }
      else
      {
         if(var_op_definition.count(var))
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

unsigned liveness::GetStep(vertex v, vertex op, unsigned int var, bool in) const
{
   if(var_op_definition.count(var))
   {
      auto def_op = get_op_where_defined(var);
      auto def_op_BB_index = vertex_BB.at(def_op);
      if(BB2MaxStep.at(def_op_BB_index))
      {
         /// the def state is pipelined
         auto op_BB_index = vertex_BB.at(op);
         if(def_op_BB_index == op_BB_index)
         {
            return satStep(def_op_BB_index, get_step(v, op, var, in));
         }
         else
         {
            return BB2MaxStep.at(def_op_BB_index) + 1;
         }
      }
      else
      {
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

unsigned liveness::GetStepPhiIn(vertex op, unsigned int var, unsigned BB_src, unsigned int BB_src_state) const
{
   auto def_op = get_op_where_defined(var);
   auto def_op_BB_index = vertex_BB.at(def_op);
   if(BB2MaxStep.at(def_op_BB_index))
   {
      /// the def state is pipelined
      auto op_BB_index = vertex_BB.at(op);
      if(def_op_BB_index == op_BB_index)
      {
         THROW_ASSERT(BB2II.count(op_BB_index) && BB2II.at(op_BB_index), "unexpected condition");
         THROW_ASSERT(op_step.count(def_op), "unexpected condition");
         THROW_ASSERT(op_step.count(op), "unexpected condition");
         auto II = BB2II.at(op_BB_index);
         auto step = op_step.at(def_op);
         auto ostep = op_step.at(op);
         THROW_ASSERT((ostep % II == 0 ? II : ostep % II) > (step % II), "unexpected condition");
         auto offset = (ostep % II == 0 ? II : ostep % II) - (step % II);
         THROW_ASSERT(offset > 0, "unexpected condition");
         return step + offset - 1;
      }
      else
      {
         return BB2MaxStep.at(def_op_BB_index) + (BB_src != BB_src_state || def_op_BB_index != BB_src_state ? 1 : 0);
      }
   }
   else
   {
      return 0;
   }
}

unsigned liveness::GetStepPhiOut(vertex op, unsigned int var) const
{
   auto def_op = get_op_where_defined(var);
   auto def_op_BB_index = vertex_BB.at(def_op);
   if(BB2MaxStep.at(def_op_BB_index))
   {
      /// the def state is pipelined
      auto op_BB_index = vertex_BB.at(op);
      if(def_op_BB_index == op_BB_index)
      {
         THROW_ASSERT(BB2II.count(op_BB_index) && BB2II.at(op_BB_index), "unexpected condition");
         THROW_ASSERT(op_step.count(def_op), "unexpected condition");
         THROW_ASSERT(op_step.count(op), "unexpected condition");
         auto II = BB2II.at(op_BB_index);
         auto step = op_step.at(def_op);
         auto ostep = op_step.at(op);
         THROW_ASSERT((ostep % II == 0 ? II : ostep % II) > (step % II), "unexpected condition");
         auto offset = (ostep % II == 0 ? II : ostep % II) - (step % II);
         THROW_ASSERT(offset > 0, "unexpected condition");
         return step + offset;
      }
      else
      {
         return BB2MaxStep.at(def_op_BB_index) + 1;
      }
   }
   else
   {
      return 0;
   }
}

unsigned liveness::GetStepWrite(vertex v, vertex def_op) const
{
   auto def_op_BB_index = vertex_BB.at(def_op);
   if(BB2MaxStep.at(def_op_BB_index))
   {
      /// the def state is pipelined
      THROW_ASSERT(vertex_to_op_step_out_map.count(v), "unexpected condition");
      THROW_ASSERT(vertex_to_op_step_out_map.at(v).count(def_op), "unexpected condition");
      return satStep(def_op_BB_index, 1 + vertex_to_op_step_out_map.at(v).at(def_op));
   }
   else
   {
      return 0;
   }
}

std::pair<bool, unsigned> liveness::GetStepIn(unsigned int BB_index, unsigned int var, vertex v) const
{
   if(var_op_definition.count(var))
   {
      auto def_op = var_op_definition.at(var);
      auto def_op_BB_index = vertex_BB.at(def_op);
      if(BB2MaxStep.at(def_op_BB_index))
      {
         /// def state is a pipelined state
         if(BB_index != def_op_BB_index)
         {
            return std::make_pair(true, BB2MaxStep.at(def_op_BB_index) + 1);
         }
         else
         {
            THROW_ASSERT(phi_vertices.count(def_op), "unexpected condition");
            if(vertex_to_op_step_out_map.at(v).count(def_op))
            {
               auto step = vertex_to_op_step_out_map.at(v).at(def_op);
               return std::make_pair(true, step);
            }
            else
            {
               return std::make_pair(false, 0);
            }
         }
      }
      else
      {
         return std::make_pair(true, 0);
      }
   }
   else
   {
      return std::make_pair(true, 0);
   }
}

unsigned liveness::GetStepOut(unsigned int var) const
{
   if(var_op_definition.count(var))
   {
      auto def_op = var_op_definition.at(var);
      auto def_op_BB_index = vertex_BB.at(def_op);
      if(BB2MaxStep.at(def_op_BB_index))
      {
         /// def state is a pipelined state
         return BB2MaxStep.at(def_op_BB_index) + 1;
      }
      else
      {
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

unsigned liveness::GetStepDef(unsigned int BB_index, unsigned int var) const
{
   if(var_op_definition.count(var))
   {
      auto def_op = var_op_definition.at(var);
      auto def_op_BB_index = vertex_BB.at(def_op);
      if(BB2MaxStep.at(def_op_BB_index))
      {
         /// def state is a pipelined state
         if(BB_index != def_op_BB_index)
         {
            return BB2MaxStep.at(def_op_BB_index) + 1;
         }
         else
         {
            THROW_ASSERT(ending_operations.count(def_op), "unexpected condition");
            auto& def_state = *ending_operations.at(def_op).begin();
            THROW_ASSERT(vertex_to_op_step_out_map.count(def_state), "unexpected condition");
            return vertex_to_op_step_out_map.at(def_state).at(def_op);
         }
      }
      else
      {
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

std::pair<bool, unsigned> liveness::GetPrevStep(unsigned int BB_index, unsigned int var, unsigned curr_step,
                                                unsigned offset) const
{
   if(var_op_definition.count(var))
   {
      auto def_op = var_op_definition.at(var);
      auto def_op_BB_index = vertex_BB.at(def_op);
      if(BB2MaxStep.at(def_op_BB_index))
      {
         /// def state is a pipelined state
         if(BB_index != def_op_BB_index)
         {
            return std::make_pair(true, BB2MaxStep.at(def_op_BB_index) + 1);
         }
         else
         {
            if(curr_step)
            {
               THROW_ASSERT(ending_operations.count(def_op), "unexpected condition");
               auto& def_state = *ending_operations.at(def_op).begin();
               THROW_ASSERT(vertex_to_op_step_out_map.count(def_state), "unexpected condition");
               auto def_step = vertex_to_op_step_out_map.at(def_state).at(def_op);
               auto step = curr_step - offset;
               if(def_step < step)
               {
                  return std::make_pair(true, step);
               }
               else
               {
                  return std::make_pair(false, curr_step);
               }
            }
            else
            {
               return std::make_pair(false, curr_step);
            }
         }
      }
      else
      {
         return std::make_pair(true, curr_step);
      }
   }
   else
   {
      /// relevant only for parameters
      return std::make_pair(curr_step > 1, curr_step > 1 ? curr_step - 1 : 0);
   }
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

unsigned liveness::GetStepOp(vertex v, vertex exec_op) const
{
   auto op_BB_index = vertex_BB.at(exec_op);
   if(BB2MaxStep.at(op_BB_index))
   {
      THROW_ASSERT(vertex_to_op_step_in_map.count(v), "unexpected condition");
      THROW_ASSERT(vertex_to_op_step_in_map.at(v).count(exec_op), "unexpected condition");
      auto step = vertex_to_op_step_in_map.at(v).at(exec_op);
      return step;
   }
   else
   {
      return 0;
   }
}
