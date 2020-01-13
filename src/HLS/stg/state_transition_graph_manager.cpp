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
 * @file StateTransitionGraphManager.cpp
 * @brief File contanining the structures necessary to manage a graph that will represent a state transition graph
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
#include "state_transition_graph_manager.hpp"

/// Behavior include
#include "behavioral_helper.hpp"
#include "op_graph.hpp"

#include "structural_manager.hpp"
#include "target_device.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

/// hls include
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// Parameter include
#include "Parameter.hpp"

/// State transition graph include
#include "StateTransitionGraph_constructor.hpp"
#include "state_transition_graph.hpp"

#include "funit_obj.hpp"
#include "multi_unbounded_obj.hpp"

/// Tree include
#include "var_pp_functor.hpp"

/// Utility include
#include "boost/graph/topological_sort.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "simple_indent.hpp"

StateTransitionGraphManager::StateTransitionGraphManager(const HLS_managerConstRef _HLSMgr, hlsRef _HLS, const ParameterConstRef _Param)
    : state_transition_graphs_collection(
          StateTransitionGraphsCollectionRef(new StateTransitionGraphsCollection(StateTransitionGraphInfoRef(new StateTransitionGraphInfo(_HLSMgr->CGetFunctionBehavior(_HLS->functionId)->CGetOpGraph(FunctionBehavior::CFG))), _Param))),
      ACYCLIC_STG_graph(StateTransitionGraphRef(new StateTransitionGraph(state_transition_graphs_collection, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL))),
      STG_graph(StateTransitionGraphRef(new StateTransitionGraph(state_transition_graphs_collection, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL | TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK))),
      EPP_STG_graph(StateTransitionGraphRef(new StateTransitionGraph(state_transition_graphs_collection, TransitionInfo::StateTransitionType::ST_EDGE_NORMAL | TransitionInfo::StateTransitionType::ST_EDGE_EPP))),
      op_function_graph(_HLSMgr->CGetFunctionBehavior(_HLS->functionId)->CGetOpGraph(FunctionBehavior::CFG)),
      Param(_Param),
      output_level(_Param->getOption<int>(OPT_output_level)),
      debug_level(_Param->getOption<int>(OPT_debug_level)),
      HLS(_HLS),
      STG_builder(StateTransitionGraph_constructorRef(new StateTransitionGraph_constructor(state_transition_graphs_collection, _HLSMgr, _HLS->functionId)))
{
   STG_builder->create_entry_state();
   STG_builder->create_exit_state();
}

StateTransitionGraphManager::~StateTransitionGraphManager() = default;

const StateTransitionGraphConstRef StateTransitionGraphManager::CGetAstg() const
{
   return ACYCLIC_STG_graph;
}

const StateTransitionGraphConstRef StateTransitionGraphManager::CGetStg() const
{
   return STG_graph;
}

const StateTransitionGraphConstRef StateTransitionGraphManager::CGetEPPStg() const
{
   return EPP_STG_graph;
}

void StateTransitionGraphManager::compute_min_max()
{
   StateTransitionGraphInfoRef info = STG_graph->GetStateTransitionGraphInfo();
   if(!info->is_a_dag)
      return;
   std::list<vertex> sorted_vertices;
   ACYCLIC_STG_graph->TopologicalSort(sorted_vertices);
   CustomUnorderedMap<vertex, unsigned int> CSteps_min;
   CustomUnorderedMap<vertex, unsigned int> CSteps_max;
   const std::list<vertex>::iterator it_sv_end = sorted_vertices.end();
   for(auto it_sv = sorted_vertices.begin(); it_sv != it_sv_end; ++it_sv)
   {
      CSteps_min[*it_sv] = 0;
      CSteps_max[*it_sv] = 0;
      InEdgeIterator ie, ie_first, iend;
      boost::tie(ie, iend) = boost::in_edges(*it_sv, *ACYCLIC_STG_graph);
      ie_first = ie;
      for(; ie != iend; ie++)
      {
         vertex src = boost::source(*ie, *ACYCLIC_STG_graph);
         CSteps_max[*it_sv] = std::max(CSteps_max[*it_sv], 1 + CSteps_max[src]);
         if(ie == ie_first)
            CSteps_min[*it_sv] = 1 + CSteps_min[src];
         else
            CSteps_min[*it_sv] = std::min(CSteps_min[*it_sv], 1 + CSteps_max[src]);
      }
   }
   THROW_ASSERT(CSteps_min.find(info->exit_node) != CSteps_min.end(), "Exit node not reachable");
   THROW_ASSERT(CSteps_max.find(info->exit_node) != CSteps_max.end(), "Exit node not reachable");
   info->min_cycles = CSteps_min.find(info->exit_node)->second - 1;
   info->max_cycles = CSteps_max.find(info->exit_node)->second - 1;
}

vertex StateTransitionGraphManager::get_entry_state() const
{
   return STG_graph->CGetStateTransitionGraphInfo()->entry_node;
}

CustomOrderedSet<vertex> StateTransitionGraphManager::get_states(const vertex& op, StateTypes statetypes) const
{
   CustomOrderedSet<vertex> vertex_set;
   VertexIterator v, vend;
   for(boost::tie(v, vend) = boost::vertices(*STG_graph); v != vend; v++)
   {
      const std::list<vertex>* operations;
      switch(statetypes)
      {
         case EXECUTING:
            operations = &(STG_graph->CGetStateInfo(*v)->executing_operations);
            break;
         case STARTING:
            operations = &(STG_graph->CGetStateInfo(*v)->starting_operations);
            break;
         case ENDING:
            operations = &(STG_graph->CGetStateInfo(*v)->ending_operations);
            break;
         default:
            THROW_UNREACHABLE("Unknown state type. Which one are you lookig for?");
      }
      auto op_it_end = operations->end();
      bool stop_flag;
      stop_flag = false;
      for(auto op_it = operations->begin(); op_it != op_it_end && !stop_flag; ++op_it)
      {
         if(*op_it == op)
         {
            stop_flag = true;
            vertex_set.insert(*v);
         }
      }
   }
   THROW_ASSERT(vertex_set.size() > 0, "Something wrong! Operation " + GET_NAME(op_function_graph, op) + " is executed " + "into no states");
   return vertex_set;
}

CustomOrderedSet<vertex> StateTransitionGraphManager::get_ending_states(const vertex& op) const
{
   return get_states(op, StateTransitionGraphManager::StateTypes::ENDING);
}

CustomOrderedSet<vertex> StateTransitionGraphManager::get_starting_states(const vertex& op) const
{
   return get_states(op, StateTransitionGraphManager::StateTypes::STARTING);
}

CustomOrderedSet<vertex> StateTransitionGraphManager::get_execution_states(const vertex& op) const
{
   return get_states(op, StateTypes::EXECUTING);
}

vertex StateTransitionGraphManager::get_exit_state() const
{
   return STG_graph->CGetStateTransitionGraphInfo()->exit_node;
}

std::string StateTransitionGraphManager::get_state_name(vertex state) const
{
   return STG_graph->CGetStateInfo(state)->name;
}

StateTransitionGraphRef StateTransitionGraphManager::GetAstg()
{
   return ACYCLIC_STG_graph;
}

StateTransitionGraphRef StateTransitionGraphManager::GetStg()
{
   return STG_graph;
}

StateTransitionGraphRef StateTransitionGraphManager::GetEPPStg()
{
   return EPP_STG_graph;
}

unsigned int StateTransitionGraphManager::get_number_of_states() const
{
   return static_cast<unsigned int>(boost::num_vertices(*state_transition_graphs_collection) - 2);
}

void StateTransitionGraphManager::print_statistics() const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of states: " + STR(get_number_of_states()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---" + std::string("Is a DAG: ") + (STG_graph->CGetStateTransitionGraphInfo()->is_a_dag ? "T" : "F"));
   if(STG_graph->CGetStateTransitionGraphInfo()->min_cycles != 0 && STG_graph->CGetStateTransitionGraphInfo()->max_cycles != 0)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Minimum number of cycles: " + STR(STG_graph->CGetStateTransitionGraphInfo()->min_cycles));
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Maximum number of cycles " + STR(STG_graph->CGetStateTransitionGraphInfo()->max_cycles));
   }
}

void StateTransitionGraphManager::add_multi_unbounded_obj(vertex s, const CustomOrderedSet<vertex>& ops)
{
   if(multi_unbounded_table.find(s) == multi_unbounded_table.end())
   {
      multi_unbounded_table[s] = generic_objRef(new multi_unbounded_obj(s, ops, std::string("mu_") + STG_graph->CGetStateInfo(s)->name));
   }
}

void StateTransitionGraphManager::specialise_mu(structural_objectRef& mu_mod, generic_objRef mu) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, HLS->debug_level, "---Specializing " + mu_mod->get_path());
   auto mut = GetPointer<multi_unbounded_obj>(mu);
   THROW_ASSERT(mut, "unexpected condition");
   structural_objectRef inOps = mu_mod->find_member("ops", port_vector_o_K, mu_mod);
   port_o* port = GetPointer<port_o>(inOps);
   const auto& ops = mut->get_ops();
   auto n_in_ports = static_cast<unsigned int>(ops.size());
   port->add_n_ports(n_in_ports, inOps);
}

void StateTransitionGraphManager::add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port)
{
   const auto& SM = HLS->datapath;
   const auto& circuit = SM->get_circ();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding :multi-unbounded controllers");
   for(auto state2mu : multi_unbounded_table)
   {
      auto mu = state2mu.second;
      std::string name = mu->get_string();
      std::string library = HLS->HLS_T->get_technology_manager()->get_library(SIMPLEJOIN_STD);
      structural_objectRef mu_mod = SM->add_module_from_technology_library(name, SIMPLEJOIN_STD, library, circuit, HLS->HLS_T->get_technology_manager());
      specialise_mu(mu_mod, mu);

      structural_objectRef port_ck = mu_mod->find_member(CLOCK_PORT_NAME, port_o_K, mu_mod);
      SM->add_connection(clock_port, port_ck);
      structural_objectRef port_rst = mu_mod->find_member(RESET_PORT_NAME, port_o_K, mu_mod);
      SM->add_connection(reset_port, port_rst);
      mu->set_structural_obj(mu_mod);
      auto p_obj = mu_mod->find_member("sop", port_o_K, mu_mod);
      mu->set_out_sign(p_obj);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Adding :multi-unbounded controllers");
}
