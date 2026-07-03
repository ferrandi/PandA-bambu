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
 *              Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file FSMInfo.cpp
 * @brief Container for FSM state metadata associated with FSM states and edges
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "FSMInfo.hpp"
#include "IR/behavioral_helper.hpp"
#include "IR/var_pp_functor.hpp"
#include "behavior/function_behavior.hpp"
#include "funit_obj.hpp"
#include "generic_device.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "memory.hpp"
#include "multi_unbounded_obj.hpp"
#include "scheduling/schedule.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility/string_manipulation.hpp"
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

FSMInfo::edgeData FSMInfo::buildUnboundedCondition(transitionType t, const std::vector<operation_descriptor>& ops,
                                                   state_descriptor referenceState, stateTransitionType type)
{
   edgeData ed;
   ed.edgeFSMType = t;
   ed.edgeOperations = ops;
   ed.referenceState = referenceState;
   ed.isElseEdge = false;
   ed.edgeSelector = type;
   return ed;
}

FSMInfo::edgeData FSMInfo::buildElseIfCondition(operation_descriptor op, const CustomOrderedSet<unsigned>& labels,
                                                bool isElseEdge, stateTransitionType type)
{
   edgeData ed;
   ed.edgeFSMType = elseifEdgeCondition;
   ed.edgeOperations.push_back(op);
   ed.edgeConditions = labels;
   ed.isElseEdge = isElseEdge;
   ed.edgeSelector = type;
   return ed;
}

FSMInfo::successorsRange FSMInfo::successors(state_descriptor src) const
{
   return successors(src, false);
}

FSMInfo::successorsRange FSMInfo::successors(state_descriptor src, bool skip_feedback) const
{
   auto oit = edgeDataMap.find(src);
   if(oit == edgeDataMap.end())
   {
      return successorsRange{nullptr, skip_feedback};
   }
   return successorsRange{&oit->second, skip_feedback};
}

FSMInfo::successorsWithDataRange FSMInfo::successorsWithData(state_descriptor src) const
{
   return successorsWithData(src, false);
}

FSMInfo::successorsWithDataRange FSMInfo::successorsWithData(state_descriptor src, bool skip_feedback) const
{
   auto oit = edgeDataMap.find(src);
   if(oit == edgeDataMap.end())
   {
      return successorsWithDataRange{nullptr, skip_feedback};
   }
   return successorsWithDataRange{&oit->second, skip_feedback};
}

FSMInfo::predecessorsRange FSMInfo::predecessors(state_descriptor dst) const
{
   return predecessors(dst, false);
}

FSMInfo::predecessorsRange FSMInfo::predecessors(state_descriptor dst, bool skip_feedback) const
{
   auto it = reverseEdgeMap.find(dst);
   const std::set<state_descriptor>* sources = it != reverseEdgeMap.end() ? &it->second : nullptr;
   return predecessorsRange{this, dst, sources, skip_feedback};
}

void FSMInfo::topologicalOrder(std::list<state_descriptor>& order) const
{
   order.clear();
   enum class VisitState
   {
      Unvisited,
      Visiting,
      Visited
   };
   std::map<state_descriptor, VisitState> visit;
   for(auto v : vertices())
   {
      visit.emplace(v, VisitState::Unvisited);
   }
   struct Frame
   {
      state_descriptor node;
      successorsRange range;
      successorsRange::const_iterator current;
   };
   std::vector<Frame> stack;
   stack.reserve(visit.size());
   for(auto v : vertices())
   {
      if(visit[v] != VisitState::Unvisited)
      {
         continue;
      }
      visit[v] = VisitState::Visiting;
      successorsRange range = successors(v, true);
      stack.push_back(Frame{v, range, range.begin()});
      while(!stack.empty())
      {
         auto& frame = stack.back();
         const auto end = frame.range.end();
         bool advanced = false;
         while(frame.current != end)
         {
            const auto succ = *frame.current;
            ++frame.current;
            auto [succ_it, inserted] = visit.emplace(succ, VisitState::Unvisited);
            auto& succ_state = succ_it->second;
            THROW_ASSERT(succ_state != VisitState::Visiting,
                         "FSM contains a cycle when computing topological order" + getState(succ).name);

            if(succ_state == VisitState::Unvisited)
            {
               succ_state = VisitState::Visiting;
               successorsRange succ_range = successors(succ, true);
               stack.push_back(Frame{succ, succ_range, succ_range.begin()});
               advanced = true;
               break;
            }
         }
         if(advanced)
         {
            continue;
         }
         visit[frame.node] = VisitState::Visited;
         order.push_front(frame.node);
         stack.pop_back();
      }
   }
}

void FSMInfo::reverseTopologicalOrder(std::list<state_descriptor>& order) const
{
   topologicalOrder(order);
   order.reverse();
}

std::vector<std::set<FSMInfo::state_descriptor>> FSMInfo::depthMap(bool skip_feedback) const
{
   std::vector<std::set<state_descriptor>> depth_states;
   if(states.empty())
   {
      return depth_states;
   }

   std::list<state_descriptor> order;
   topologicalOrder(order);
   if(order.empty())
   {
      return depth_states;
   }

   std::map<state_descriptor, std::set<unsigned>> state_depths;
   for(const auto state : order)
   {
      const auto preds = predecessors(state, skip_feedback);
      auto& depths = state_depths[state];
      if(preds.begin() == preds.end())
      {
         if(depth_states.empty())
         {
            depth_states.emplace_back();
         }
         depths.insert(0U);
         depth_states[0].insert(state);
         continue;
      }
      for(const auto pred : preds)
      {
         const auto pred_it = state_depths.find(pred);
         THROW_ASSERT(pred_it != state_depths.end(), "Predecessor depth information missing");
         for(const auto pred_depth : pred_it->second)
         {
            const auto new_depth = pred_depth + 1U;
            if(depth_states.size() <= new_depth)
            {
               depth_states.resize(new_depth + 1U);
            }
            if(depths.insert(new_depth).second)
            {
               depth_states[new_depth].insert(state);
            }
         }
      }
   }
   return depth_states;
}

void FSMInfo::writeDot(const std::filesystem::path& file_name, FunctionBehaviorConstRef FB, hlsRef HLS,
                       const int detail_level) const
{
   std::ofstream dot(file_name);
   if(!dot.is_open())
      return;

   dot << "digraph FSM {\n";
   dot << "node [shape=Mrecord];\n";

   const auto& op_function_graph = FB->GetOpGraph(FunctionBehavior::CFG);
   const auto schedule = HLS->Rsch;
   auto BH = FB->CGetBehavioralHelper();
   const std::unique_ptr<var_pp_functor> vpp = std::make_unique<std_var_pp_functor>(BH);

   auto print_state_label = [&](std::ostream& os, state_descriptor v) {
      const auto& state_data = getState(v);
      const auto critical_paths = schedule->ComputeCriticalPath(state_data);

      if(!state_data.executingOperations.empty() &&
         op_function_graph.CGetNodeInfo(state_data.executingOperations.front()).node_type == TYPE_ENTRY)
      {
         os << "START";
         return;
      }
      if(!state_data.executingOperations.empty() &&
         op_function_graph.CGetNodeInfo(state_data.executingOperations.front()).node_type == TYPE_EXIT)
      {
         os << "END";
         return;
      }

      os << "< " << state_data.name << " | { ";
      for(const auto& op : state_data.executingOperations)
      {
         const auto& op_node_info = op_function_graph.CGetNodeInfo(op);
         const auto first_index = op_node_info.GetNodeId();
         const bool critical = critical_paths.find(first_index) != critical_paths.end();
         if(detail_level == 0)
         {
            if(std::find(state_data.endingOperations.begin(), state_data.endingOperations.end(), op) ==
               state_data.endingOperations.end())
            {
               os << "<font color=\"gold2\">";
            }
            else if(critical)
            {
               os << "<font color=\"red3\">";
            }
            else if(op_node_info.node_type & TYPE_STORE)
            {
               os << "<font color=\"blue\">";
            }
         }
         const auto first_starting_time = schedule->GetStartingTime(first_index);
         const auto first_ending_time = schedule->GetEndingTime(first_index);
         if(detail_level == 0)
         {
            os << op_node_info.vertex_name << " [" << NumberToString(first_starting_time, 2, 7) << "---"
               << NumberToString(first_ending_time, 2, 7) << "("
               << NumberToString(first_ending_time - first_starting_time, 2, 7) << ")"
               << "]";
            if(state_data.lpII)
            {
               const auto step = state_data.stepIn.at(op);
               os << "(" << state_data.lpII << ")(" << step << ")";
            }
            os << " --&gt; ";
         }
         std::string vertex_print = BH->print_vertex(op_function_graph, op, vpp, true);
         boost::replace_all(vertex_print, "&", "&amp;");
         boost::replace_all(vertex_print, "|", "\\|");
         boost::replace_all(vertex_print, ">", "&gt;");
         boost::replace_all(vertex_print, "<", "&lt;");
         boost::replace_all(vertex_print, R"(\\")", "&#92;&quot;");
         boost::replace_all(vertex_print, "\\\"", "&quot;");
         boost::replace_all(vertex_print, ":", "&#58;");
         boost::replace_all(vertex_print, "\\n", "");
         boost::replace_all(vertex_print, "{", "\\{");
         boost::replace_all(vertex_print, "}", "\\}");
         os << vertex_print;
         if(detail_level == 0)
         {
            if(critical ||
               std::find(state_data.endingOperations.begin(), state_data.endingOperations.end(), op) ==
                   state_data.endingOperations.end() ||
               op_node_info.node_type & TYPE_STORE)
            {
               os << " </font>";
            }
         }
         os << "<br align=\"left\"/>";
      }
      if(!detail_level)
      {
         os << " | ";
         for(const auto& op : state_data.endingOperations)
         {
            const auto& op_node_info = op_function_graph.CGetNodeInfo(op);
            const auto first_index = op_node_info.GetNodeId();
            const bool critical = critical_paths.find(first_index) != critical_paths.end();
            if(std::find(state_data.executingOperations.begin(), state_data.executingOperations.end(), op) ==
               state_data.executingOperations.end())
            {
               os << "<font color=\"green2\">";
            }
            else if(critical)
            {
               os << "<font color=\"red3\">";
            }
            else if(op_node_info.node_type & TYPE_STORE)
            {
               os << "<font color=\"blue\">";
            }
            const auto first_starting_time = schedule->GetStartingTime(first_index);
            const auto first_ending_time = schedule->GetEndingTime(first_index);
            os << op_node_info.vertex_name << " [" << NumberToString(first_starting_time, 2, 7) << "---"
               << NumberToString(first_ending_time, 2, 7) << "("
               << NumberToString(first_ending_time - first_starting_time, 2, 7) << ")"
               << "]";
            if(state_data.lpII)
            {
               const auto step = opStepOut.at(op);
               os << "(" << state_data.lpII << ")(" << step << ")";
            }
            os << " --&gt; ";
            std::string vertex_print = BH->print_vertex(op_function_graph, op, vpp, true);
            boost::replace_all(vertex_print, "&", "&amp;");
            boost::replace_all(vertex_print, "|", "\\|");
            boost::replace_all(vertex_print, ">", "&gt;");
            boost::replace_all(vertex_print, "<", "&lt;");
            boost::replace_all(vertex_print, R"(\\")", "&#92;&quot;");
            boost::replace_all(vertex_print, "\\\"", "&quot;");
            boost::replace_all(vertex_print, ":", "&#58;");
            boost::replace_all(vertex_print, "\\n", "");
            boost::replace_all(vertex_print, "{", "\\{");
            boost::replace_all(vertex_print, "}", "\\}");
            os << vertex_print;
            if(critical ||
               std::find(state_data.executingOperations.begin(), state_data.executingOperations.end(), op) ==
                   state_data.executingOperations.end() ||
               op_node_info.node_type & TYPE_STORE)
            {
               os << " </font>";
            }
            os << "<br align=\"left\"/>";
         }
      }
      os << " } | ";
      os << "BB" << state_data.bbId;
      os << ">";
   };

   for(state_descriptor v = 0; v < states.size(); ++v)
   {
      const auto& sdata = states[v];
      dot << sdata.name << " [";
      if(v == entryNode || v == exitNode)
         dot << "color=blue,";
      dot << "label=";
      print_state_label(dot, v);
      dot << "];\n";
   }

   auto print_edge_info = [&](std::ostream& os, const edgeData& info) {
      if(info.edgeFSMType == noEdgeCondition)
      {
         ;
      }
      else if(info.edgeFSMType == elseifEdgeCondition)
      {
         if(detail_level == 0)
            os << op_function_graph.CGetNodeInfo(info.edgeOperations.front()).vertex_name;
         os << " (";
         const std::unique_ptr<var_pp_functor> std_vppf = std::make_unique<std_var_pp_functor>(BH);
         bool first = true;
         for(auto label : info.edgeConditions)
         {
            if(first)
            {
               os << BH->PrintNode(label, std_vppf);
               first = false;
            }
            else
            {
               os << "," << BH->PrintNode(label, std_vppf);
            }
         }
         if(info.isElseEdge)
         {
            if(first)
               os << "default\\n";
            else
               os << ",default\\n";
         }
         os << ")";
      }
      else if(info.edgeFSMType == doneVariableLatencyOpEdgeCondition)
      {
         bool first = true;
         for(auto op : info.edgeOperations)
         {
            if(first)
            {
               os << op_function_graph.CGetNodeInfo(op).vertex_name;
               first = false;
            }
            else
            {
               os << "," << op_function_graph.CGetNodeInfo(op).vertex_name;
            }
         }
         os << "(doneVariableLatencyOpEdgeCondition)\\n";
      }
      else if(info.edgeFSMType == runningVariableLatencyOpEdgeCondition)
      {
         bool first = true;
         for(auto op : info.edgeOperations)
         {
            if(first)
            {
               os << op_function_graph.CGetNodeInfo(op).vertex_name;
               first = false;
            }
            else
            {
               os << "," << op_function_graph.CGetNodeInfo(op).vertex_name;
            }
         }
         os << "(runningVariableLatencyOpEdgeCondition)\\n";
      }
      else
      {
         THROW_ERROR("transition type not yet supported");
      }
   };

   for(const auto& [src, targets] : edgeDataMap)
   {
      for(const auto& [dst, meta] : targets)
      {
         dot << states.at(src).name << " -> " << states.at(dst).name << " [label=\"";
         print_edge_info(dot, meta);
         dot << "\"];\n";
      }
   }

   dot << "}\n";
}

unsigned int FSMInfo::getNumberOfStates(bool is_function_pipelined) const
{
   const auto sz = states.size();
   return sz >= 2 ? static_cast<unsigned int>(sz - 2 + (is_function_pipelined ? 1 : 0)) : 0U;
}

FSMInfo::state_descriptor FSMInfo::createState(const std::list<operation_descriptor>& exec_op,
                                               const std::list<operation_descriptor>& start_op,
                                               const std::list<operation_descriptor>& end_op, unsigned int BB_id,
                                               const std::map<operation_descriptor, unsigned>& step_in,
                                               const std::map<operation_descriptor, unsigned>& step_out,
                                               unsigned vertex_LP_II, unsigned max_steps, bool is_last_state,
                                               bool isPipelined, const OpGraph& ASSERT_PARAMETER(data),
                                               const char* custom_name)
{
   const state_descriptor state = nextStateId++;
   THROW_ASSERT(state == states.size(), "Duplicated FSM state identifier");

   states.emplace_back();
   stateData& state_data = states.back();
   if(custom_name)
   {
      state_data.name = custom_name;
   }
   else
   {
      state_data.name = std::string(stateNamePrefix) + std::to_string(nextStateNameIndex++);
   }

   state_data.executingOperations = exec_op;
   for(auto ex : exec_op)
   {
#if HAVE_ASSERTS
      const auto& op_info = data.CGetNodeInfo(ex);
      THROW_ASSERT((op_info.node_type & TYPE_VPHI) == 0, "unexpected condition");
      THROW_ASSERT(!state_data.isDummy || (op_info.node_type & TYPE_PHI) == 0, "unexpected condition");
#endif
      operationExecutingStates[ex].insert(state);
   }
   state_data.startingOperations = start_op;
   state_data.endingOperations = end_op;
   for(auto en : end_op)
   {
      operationEndingStates[en].insert(state);
   }
   state_data.bbId = BB_id;
   state_data.stepIn = step_in;
   for(const auto& p : step_out)
   {
      THROW_ASSERT(opStepOut.find(p.first) == opStepOut.end() || opStepOut.find(p.first)->second == p.second,
                   "unexpected case");
      opStepOut.emplace(p.first, p.second);
   }
   state_data.lpII = vertex_LP_II;
   state_data.maxStep = max_steps;
   state_data.isLastState = is_last_state;
   state_data.isDummy = false;
   state_data.isPipelinedState = isPipelined;
   state_data.isPrologue.clear();
   THROW_ASSERT(BB2MaxStep.find(BB_id) == BB2MaxStep.end() || BB2MaxStep.find(BB_id)->second == max_steps,
                "unexpected case");
   BB2MaxStep.emplace(BB_id, max_steps);
   return state;
}

void FSMInfo::addMultiUnboundedObj(state_descriptor s, const std::vector<operation_descriptor>& ops)
{
   if(multiUnboundedTable.find(s) == multiUnboundedTable.end())
   {
      auto ordered_ops = ops;
      std::sort(ordered_ops.begin(), ordered_ops.end());
      ordered_ops.erase(std::unique(ordered_ops.begin(), ordered_ops.end()), ordered_ops.end());
      multiUnboundedTable[s] =
          generic_objRef(new multi_unbounded_obj(s, ordered_ops, std::string("mu_") + getState(s).name));
   }
}

void FSMInfo::specialiseMu(structural_objectRef& mu_mod, generic_objRef mu, bool is_function_pipelined) const
{
   structural_objectRef inOps = mu_mod->find_member("ops", port_vector_o_K, mu_mod);
   auto* port = GetPointer<port_o>(inOps);
   auto mut = GetPointer<multi_unbounded_obj>(mu);
   THROW_ASSERT(mut, "unexpected condition");
   auto n_in_ports = static_cast<unsigned int>(mut->get_ops().size());
   port->add_n_ports(n_in_ports, inOps);
   if(is_function_pipelined)
   {
      structural_objectRef inStarts = mu_mod->find_member("starts", port_vector_o_K, mu_mod);
      GetPointer<port_o>(inStarts)->add_n_ports(n_in_ports, inStarts);
   }
}

void FSMInfo::addToSM(structural_objectRef clock_port, structural_objectRef reset_port, hlsRef HLS,
                      bool is_function_pipelined) const
{
   const auto& SM = HLS->datapath;
   const auto& circuit = SM->get_circ();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, HLS->debug_level, "-->Adding :multi-unbounded controllers");
   for(const auto& state2mu : getMuCtrls())
   {
      auto mu = state2mu.second;
      std::string name = mu->get_string();
      structural_objectRef mu_mod;
      if(is_function_pipelined)
      {
         std::string library = HLS->HLS_D->get_technology_manager()->get_library(COMPLEXJOIN_STD);
         mu_mod = SM->add_module_from_technology_library(name, COMPLEXJOIN_STD, library, circuit,
                                                         HLS->HLS_D->get_technology_manager());
      }
      else
      {
         std::string library = HLS->HLS_D->get_technology_manager()->get_library(SIMPLEJOIN_STD);
         mu_mod = SM->add_module_from_technology_library(name, SIMPLEJOIN_STD, library, circuit,
                                                         HLS->HLS_D->get_technology_manager());
      }
      specialiseMu(mu_mod, mu, is_function_pipelined);

      structural_objectRef port_ck = mu_mod->find_member(CLOCK_PORT_NAME, port_o_K, mu_mod);
      SM->add_connection(clock_port, port_ck);
      structural_objectRef port_rst = mu_mod->find_member(RESET_PORT_NAME, port_o_K, mu_mod);
      SM->add_connection(reset_port, port_rst);
      mu->set_structural_obj(mu_mod);
      auto p_obj = mu_mod->find_member("sop", port_o_K, mu_mod);
      mu->set_out_sign(p_obj);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, HLS->debug_level, "<--Adding :multi-unbounded controllers");
}

unsigned FSMInfo::getStateId(state_descriptor s) const
{
   const auto state_name = getState(s).name;
   THROW_ASSERT(state_name.compare(0, std::string(stateNamePrefix).size(), stateNamePrefix) == 0,
                "unexpected state name format");
   const auto numberPart = state_name.substr(std::string(stateNamePrefix).size());
   size_t pos = 0;
   const auto state_id = static_cast<unsigned>(std::stoi(numberPart, &pos));
   THROW_ASSERT(pos == numberPart.size(), "state name format unexpected");
   return state_id;
}

const CustomOrderedSet<FSMInfo::state_descriptor>&
FSMInfo::getVariableSourceStates(FSMInfo::state_descriptor state, FSMInfo::operation_descriptor operation,
                                 unsigned int variable) const
{
   const auto stateIt = variableSourceStates.find(state);
   if(stateIt == variableSourceStates.end())
   {
      return emptyStateSet;
   }

   const auto operationIt = stateIt->second.find(operation);
   if(operationIt == stateIt->second.end())
   {
      return emptyStateSet;
   }

   const auto variableIt = operationIt->second.find(variable);
   if(variableIt == operationIt->second.end())
   {
      return emptyStateSet;
   }

   return variableIt->second;
}

void FSMInfo::addVariableSourceState(FSMInfo::state_descriptor state, FSMInfo::operation_descriptor operation,
                                     unsigned int variable, FSMInfo::state_descriptor sourceState)
{
   variableSourceStates[state][operation][variable].insert(sourceState);
}

void FSMInfo::finalizeFSMInfo(const OpGraph& data, const HLS_managerRef HLSMgr)
{
   for(const auto& st : verticesWithData())
   {
      const auto& state_info = st.second;
      for(const auto& op : state_info.executingOperations)
      {
         const auto& op_info = data.CGetNodeInfo(op);
         if((op_info.node_type & TYPE_NOP) != 0)
         {
            continue;
         }
         if((op_info.node_type & TYPE_PHI) != 0)
         {
            const auto phi_node = HLSMgr->get_ir_manager()->GetIRNode(data.CGetNodeInfo(op).GetNodeId());
            for(const auto& def_edge : GetPointer<const phi_stmt>(phi_node)->CGetDefEdgesList())
            {
               unsigned int ir_var = def_edge.first->index;
               unsigned int bb_index = def_edge.second;
               /// now we look for the last state with operations belonging to basic block bb_index
#if HAVE_ASSERTS
               bool found_state = false;
#endif

               for(const auto& src_state : predecessors(st.first))
               {
                  const auto& source_state_info = getState(src_state);
                  auto same_bb = source_state_info.bbId == bb_index;
                  if(((!source_state_info.isPipelinedState || !state_info.isPipelinedState) && same_bb) ||
                     ((state_info.isPipelinedState && source_state_info.isPipelinedState && same_bb &&
                       !source_state_info.isPrologue.count(op)) ||
                      (state_info.isPipelinedState && source_state_info.isPipelinedState && !same_bb &&
                       source_state_info.isPrologue.count(op))))
                  {
                     THROW_ASSERT(src_state != entryNode,
                                  "Source state for phi " + STR(data.CGetNodeInfo(op).GetNodeId()) + " not found");
                     addVariableSourceState(st.first, op, ir_var, src_state);
#if HAVE_ASSERTS
                     found_state = true;
#endif
                  }
               }
               THROW_ASSERT(found_state || state_info.isPipelinedState,
                            "Not found source for phi " + op_info.vertex_name + " in state " + state_info.name +
                                " coming from BB" + STR(bb_index));
            }
         }
         else
         {
            const auto& scalar_use = getVariablesScalarUse(data, op);
            for(const auto tgt_state : successors(st.first))
            {
               for(const auto use : scalar_use)
               {
                  THROW_ASSERT(HLSMgr->is_register_compatible(use) || HLSMgr->Rmem->has_base_address(use) ||
                                   ir_helper::IsParameter(HLSMgr->get_ir_manager()->GetIRNode(use)),
                               "unexpected condition " + STR(use));
                  addVariableSourceState(st.first, op, use, tgt_state);
               }
            }
         }
      }
   }
}

unsigned FSMInfo::satStep(unsigned BB_index, unsigned step) const
{
   return std::min(BB2MaxStep.at(BB_index) + 1, step);
}

unsigned FSMInfo::getStepInternal(const OpGraph& data, state_descriptor v, operation_descriptor op, unsigned int var,
                                  bool in) const
{
   unsigned int step = 0;
   if(in)
   {
      THROW_ASSERT(!(data.CGetNodeInfo(op).node_type & TYPE_PHI), "unexpected condition");
      const auto& state_data = getState(v);
      const auto step_it = state_data.stepIn.find(op);
      if(step_it != state_data.stepIn.end())
      {
         step = step_it->second;
      }
      else
      {
         auto def_op = getDefOp(data, var);
         THROW_ASSERT(opStepOut.count(def_op), "unexpected condition");
         step = opStepOut.at(def_op);
      }
   }
   else
   {
      THROW_ASSERT(opStepOut.count(op), "unexpected condition");
      step = opStepOut.at(op);
   }
   return step;
}

unsigned FSMInfo::GetStep(const OpGraph& data, state_descriptor v, operation_descriptor op, unsigned int var, bool in,
                          bool var_register_compatible) const
{
   if(!var_register_compatible)
   {
      return 0;
   }

   const auto& bb2max = BB2MaxStep;
   auto def_op = getDefOp(data, var);
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      auto op_BB_index = data.CGetNodeInfo(op).bb_index;
      if(def_op_BB_index == op_BB_index)
      {
         auto step = getStepInternal(data, v, op, var, in);
         if(getState(v).isDummy && in)
         {
            ++step;
         }
         return satStep(def_op_BB_index, step);
      }
      else
      {
         return bb2max.at(def_op_BB_index) + 1;
      }
   }
   else
   {
      return 0;
   }
}

unsigned FSMInfo::GetStepPhiIn(const OpGraph& data, operation_descriptor op, unsigned int var, unsigned int BB_src,
                               unsigned int BB_src_state, state_descriptor src_state, const ScheduleRef& schedule) const
{
   THROW_ASSERT(schedule, "unexpected condition");
   const auto& bb2max = BB2MaxStep;
   auto def_op = getDefOp(data, var);
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      auto op_BB_index = data.CGetNodeInfo(op).bb_index;
      if(def_op_BB_index == op_BB_index)
      {
         THROW_ASSERT(opStepOut.count(def_op), "unexpected condition");
         THROW_ASSERT(opStepOut.count(op), "unexpected condition");
         const auto II = schedule->GetLoopPipeliningII(op_BB_index);
         THROW_ASSERT(II, "unexpected condition");
         auto step = opStepOut.at(def_op);
         auto ostep = opStepOut.at(op);
         THROW_ASSERT((ostep % II == 0 ? II : ostep % II) >= (step % II), "unexpected condition");
         auto offset = (ostep % II == 0 ? II : ostep % II) - (step % II);
         THROW_ASSERT(step + offset >= 1,
                      "unexpected condition ostep=" + STR(ostep) + " II=" + STR(II) + " step=" + STR(step));
         return step + (offset > 0 ? offset - 1 : 0) + (getState(src_state).isDummy ? 1 : 0);
      }
      else
      {
         return bb2max.at(def_op_BB_index) + (BB_src != BB_src_state || def_op_BB_index != BB_src_state ? 1 : 0);
      }
   }
   return 0;
}

unsigned FSMInfo::GetStepPhiOut(const OpGraph& data, operation_descriptor op, unsigned int var,
                                const ScheduleRef& schedule) const
{
   THROW_ASSERT(schedule, "unexpected condition");
   const auto& bb2max = BB2MaxStep;
   auto def_op = getDefOp(data, var);
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      auto op_BB_index = data.CGetNodeInfo(op).bb_index;
      if(def_op_BB_index == op_BB_index)
      {
         THROW_ASSERT(opStepOut.count(def_op), "unexpected condition");
         THROW_ASSERT(opStepOut.count(op), "unexpected condition");
         const auto II = schedule->GetLoopPipeliningII(op_BB_index);
         THROW_ASSERT(II, "unexpected condition");
         auto step = opStepOut.at(def_op);
         auto ostep = opStepOut.at(op);
         THROW_ASSERT((ostep % II == 0 ? II : ostep % II) >= (step % II),
                      "unexpected condition: ostep=" + STR(ostep) + " II=" + STR(II) + " step=" + STR(step));
         auto offset = (ostep % II == 0 ? II : ostep % II) - (step % II);
         return step + offset;
      }
      else
      {
         return bb2max.at(def_op_BB_index) + 1;
      }
   }
   return 0;
}

unsigned FSMInfo::GetStepWrite(const OpGraph& data, operation_descriptor def_op) const
{
   const auto& bb2max = BB2MaxStep;
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      THROW_ASSERT(opStepOut.count(def_op), "unexpected condition");
      return satStep(def_op_BB_index, 1 + opStepOut.at(def_op));
   }
   return 0;
}

unsigned FSMInfo::GetStepIn(const OpGraph& data, unsigned int ASSERT_PARAMETER(BB_index), unsigned int var) const
{
   const auto& bb2max = BB2MaxStep;
   auto def_op = getDefOp(data, var);
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      THROW_ASSERT(BB_index != def_op_BB_index, "unexpected condition");
      return bb2max.at(def_op_BB_index) + 1;
   }
   return 0;
}

unsigned FSMInfo::GetStepOut(const OpGraph& data, unsigned int var) const
{
   const auto& bb2max = BB2MaxStep;
   auto def_op = getDefOp(data, var);
   auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
   if(bb2max.at(def_op_BB_index))
   {
      return bb2max.at(def_op_BB_index) + 1;
   }
   return 0;
}

std::pair<bool, unsigned> FSMInfo::GetPrevStep(const OpGraph& data, unsigned int BB_index, unsigned int var,
                                               unsigned curr_step, unsigned offset, bool var_register_compatible) const
{
   const auto& bb2max = BB2MaxStep;
   if(var_register_compatible)
   {
      auto def_op = getDefOp(data, var);
      auto def_op_BB_index = data.CGetNodeInfo(def_op).bb_index;
      if(bb2max.at(def_op_BB_index))
      {
         if(BB_index != def_op_BB_index)
         {
            return std::make_pair(true, bb2max.at(def_op_BB_index) + 1);
         }
         if(curr_step)
         {
            auto def_step = opStepOut.at(def_op);
            auto step = curr_step - offset;
            if(def_step < step)
            {
               return std::make_pair(true, step);
            }
            return std::make_pair(false, curr_step);
         }
         return std::make_pair(false, curr_step);
      }
      return std::make_pair(true, curr_step);
   }
   return std::make_pair(curr_step > 1, curr_step > 1 ? curr_step - 1 : 0);
}

unsigned FSMInfo::GetStepOp(const OpGraph& data, state_descriptor v, operation_descriptor exec_op) const
{
   const auto& bb2max = BB2MaxStep;
   auto op_BB_index = data.CGetNodeInfo(exec_op).bb_index;
   if(bb2max.at(op_BB_index))
   {
      const auto& state_data = getState(v);
      THROW_ASSERT(state_data.stepIn.count(exec_op), "unexpected condition");
      return state_data.stepIn.at(exec_op);
   }
   return 0;
}
