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
 * @file StateTransitionGraph_constructor.cpp
 * @brief File containing the structures necessary to manage a graph that will represent a state transition graph
 *
 * This file contains the necessary data structures used to represent a state transition graph
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "StateTransitionGraph_constructor.hpp"

#include "hls_manager.hpp"
#include "op_graph.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

StateTransitionGraph_constructor::StateTransitionGraph_constructor(const StateTransitionGraphsCollectionRef _state_transition_graphs_collection, const HLS_managerConstRef _HLSMgr, unsigned int _funId)
    : state_index(0), state_transition_graphs_collection(_state_transition_graphs_collection), state_transition_graph(new StateTransitionGraph(state_transition_graphs_collection, -1)), HLSMgr(_HLSMgr), funId(_funId)
{
}

void StateTransitionGraph_constructor::create_entry_state()
{
   vertex newVertex = state_transition_graphs_collection->AddVertex(NodeInfoRef(new StateInfo()));
   const StateInfoRef state_info = state_transition_graph->GetStateInfo(newVertex);
   const OpGraphInfoConstRef cfg = HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG)->CGetOpGraphInfo();
   state_info->HLSMgr = HLSMgr;
   state_info->funId = funId;
   state_info->name = "ENTRY";
   state_info->executing_operations.push_back(cfg->entry_vertex);
   state_info->starting_operations.push_back(cfg->entry_vertex);
   state_info->ending_operations.push_back(cfg->entry_vertex);
   state_info->BB_ids.insert(HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG)->CGetOpNodeInfo(cfg->entry_vertex)->bb_index);
   //   state_info->BB_ids.insert(GET_BB_INDEX(HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG).get(), cfg->entry_vertex));
   state_transition_graph->GetStateTransitionGraphInfo()->entry_node = newVertex;
}

void StateTransitionGraph_constructor::create_exit_state()
{
   vertex newVertex = state_transition_graphs_collection->AddVertex(NodeInfoRef(new StateInfo()));
   const StateInfoRef state_info = state_transition_graph->GetStateInfo(newVertex);
   const OpGraphInfoConstRef cfg = HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG)->CGetOpGraphInfo();
   state_info->HLSMgr = HLSMgr;
   state_info->funId = funId;
   state_info->name = "EXIT";
   state_info->executing_operations.push_back(cfg->exit_vertex);
   state_info->starting_operations.push_back(cfg->exit_vertex);
   state_info->ending_operations.push_back(cfg->exit_vertex);
   state_info->BB_ids.insert(GET_BB_INDEX(HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG).get(), cfg->exit_vertex));

   state_transition_graph->GetStateTransitionGraphInfo()->exit_node = newVertex;
}

vertex StateTransitionGraph_constructor::create_state(const std::list<vertex>& exec_op, const std::list<vertex>& start_op, const std::list<vertex>& end_op, const CustomOrderedSet<unsigned int>& BB_ids)
{
   vertex newVertex = state_transition_graphs_collection->AddVertex(NodeInfoRef(new StateInfo()));
   const StateInfoRef state_info = state_transition_graph->GetStateInfo(newVertex);
   state_info->HLSMgr = HLSMgr;
   state_info->funId = funId;
   state_info->name = std::string(STATE_NAME_PREFIX + boost::lexical_cast<std::string>(state_index));
   state_transition_graph->GetStateTransitionGraphInfo()->state_id_to_vertex[state_index] = newVertex;
   state_transition_graph->GetStateTransitionGraphInfo()->vertex_to_state_id[newVertex] = state_index;
   state_info->executing_operations = exec_op;
   state_info->starting_operations = start_op;
   state_info->ending_operations = end_op;
   state_info->BB_ids = BB_ids;

   state_index++;

   return newVertex;
}

EdgeDescriptor StateTransitionGraph_constructor::connect_state(const vertex& src, const vertex& tgt, int type)
{
   if(type == TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
      state_transition_graph->GetStateTransitionGraphInfo()->is_a_dag = false;
   // get the vertex iterator
   VertexIterator vIterBeg, vIterEnd;
   boost::tie(vIterBeg, vIterEnd) = boost::vertices(*state_transition_graph);
   // check that source and target have been already added
   THROW_ASSERT(std::find(vIterBeg, vIterEnd, src) != vIterEnd and std::find(vIterBeg, vIterEnd, tgt) != vIterEnd, "Source vertex or target one is not present into graph");
   EdgeDescriptor e;
   bool exists;
   boost::tie(e, exists) = boost::edge(src, tgt, *state_transition_graphs_collection);
   THROW_ASSERT((not exists) or (not(state_transition_graph->GetSelector(e) & type)), "transition already present with the same selector");
   // edge creation
   if(not exists)
   {
      const TransitionInfoRef eInfo = TransitionInfoRef(new TransitionInfo(HLSMgr.lock()->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::CFG)));
      e = state_transition_graphs_collection->AddEdge(src, tgt, type, eInfo);
   }
   else
   {
      state_transition_graphs_collection->AddSelector(e, type);
   }
   return e;
}

void StateTransitionGraph_constructor::set_condition(const EdgeDescriptor& e, transition_type t, vertex op)
{
   state_transition_graph->GetTransitionInfo(e)->t = t;
   state_transition_graph->GetTransitionInfo(e)->ops.insert(op);
}

void StateTransitionGraph_constructor::set_unbounded_condition(const EdgeDescriptor& e, transition_type t, const CustomOrderedSet<vertex>& ops, vertex ref_state)
{
   state_transition_graph->GetTransitionInfo(e)->t = t;
   state_transition_graph->GetTransitionInfo(e)->ops = ops;
   state_transition_graph->GetTransitionInfo(e)->ref_state = ref_state;
}

void StateTransitionGraph_constructor::set_switch_condition(const EdgeDescriptor& e, vertex op, const CustomOrderedSet<unsigned>& labels, bool has_default)
{
   state_transition_graph->GetTransitionInfo(e)->t = CASE_COND;
   state_transition_graph->GetTransitionInfo(e)->ops.insert(op);
   state_transition_graph->GetTransitionInfo(e)->has_default = has_default;
   state_transition_graph->GetTransitionInfo(e)->labels = labels;
}

void StateTransitionGraph_constructor::copy_condition(const EdgeDescriptor& dest, const EdgeDescriptor& source)
{
   state_transition_graph->GetTransitionInfo(dest)->t = state_transition_graph->GetTransitionInfo(source)->t;
   state_transition_graph->GetTransitionInfo(dest)->ops = state_transition_graph->GetTransitionInfo(source)->ops;
   state_transition_graph->GetTransitionInfo(dest)->has_default = state_transition_graph->GetTransitionInfo(source)->has_default;
   state_transition_graph->GetTransitionInfo(dest)->labels = state_transition_graph->GetTransitionInfo(source)->labels;
}

//*********************************************************************

/** Removes the specified edge from the graph.
 * If the graph does not contain the specified edge, this method throws an exception. */
void StateTransitionGraph_constructor::delete_edge(const vertex& src, const vertex& tgt)
{
   state_transition_graphs_collection->RemoveSelector(boost::edge(src, tgt, *state_transition_graphs_collection).first);
}

/** Removes the specified state from the graph.
 * If the graph does not contain a vertex representing that state, this method throws an exception. */
void StateTransitionGraph_constructor::delete_state(const vertex& src)
{
   std::map<unsigned int, vertex>& id_to_v = state_transition_graph->GetStateTransitionGraphInfo()->state_id_to_vertex;
   std::map<vertex, unsigned int>& v_to_id = state_transition_graph->GetStateTransitionGraphInfo()->vertex_to_state_id;

   unsigned int id = v_to_id.at(src);
   v_to_id.erase(src);
   id_to_v.erase(id);
   state_transition_graphs_collection->RemoveVertex(src);
}
