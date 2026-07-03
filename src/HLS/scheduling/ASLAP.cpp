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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file ASLAP.cpp
 * @brief Class implementation for ASLAP class methods.
 *
 * This file implements some of the ASLAP member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ASLAP.hpp"

#include "Parameter.hpp"
#include "Vertex.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#include <boost/graph/reverse_graph.hpp>

#include <cmath>

// in case asap and alap are computed with/without constraints on the resources available
#define WITH_CONSTRAINT 0

ASLAP::ASLAP(const HLS_managerConstRef _hls_manager, const hlsRef HLS,
             const CustomUnorderedSet<OpGraph::vertex_descriptor>& _operations, const ParameterConstRef _parameters,
             unsigned int _ctrl_step_multiplier)
    : beh_graph(_hls_manager->CGetFunctionBehavior(HLS->functionId)->GetOpGraph(FunctionBehavior::FLSAODG)),
      ASAP(_hls_manager, HLS->functionId, _parameters),
      ALAP(_hls_manager, HLS->functionId, _parameters),
      min_tot_csteps(0u),
      max_tot_csteps(0u),
      has_branching_blocks(false),
      allocation_information(*HLS->allocation_information),
      clock_period(HLS->HLS_C->get_clock_period() * HLS->HLS_C->get_clock_period_resource_fraction()),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this))),
      ctrl_step_multiplier(_ctrl_step_multiplier),
      operations(_operations)
{
   for(const auto v : beh_graph.vertices())
   {
      if(beh_graph.CGetNodeInfo(v).node_type & TYPE_MULTIIF)
      {
         has_branching_blocks = true;
         break;
      }
   }
   const auto& ls = _hls_manager->CGetFunctionBehavior(HLS->functionId)->get_levels();
   for(auto l : ls)
   {
      if(_operations.find(l) != _operations.end())
      {
         levels.push_back(l);
      }
   }
}

void ASLAP::print(std::ostream& os) const
{
   if(ASAP.num_scheduled())
   {
      os << "ASAP\n";
      ASAP.print();
      os << std::endl;
   }
   if(ALAP.num_scheduled())
   {
      os << "ALAP\n";
      ALAP.print();
      os << std::endl;
   }
}

/**
 * Terminate function used during improve_ASAP_with_constraints visiting and updating of ASAP_p vector
 */
struct p_update_check : public boost::dfs_visitor<>
{
 private:
   /// vertex
   OpGraph::vertex_descriptor s;
   /// string that identifies operation name
   const std::string& op_name;
   /// asap values
   vertex2int<>& ASAP_p;
   /// behavioral specification in terms of graph
   const OpGraph& beh_graph;

 public:
   p_update_check(OpGraph::vertex_descriptor v, const std::string& name, vertex2int<>& A_p, const OpGraph& g)
       : s(v), op_name(name), ASAP_p(A_p), beh_graph(g)
   {
   }

   /**
    * Template function used to discover vertex
    */
   template <class Vertex, class Graph>
   void discover_vertex(Vertex v, const Graph&) const
   {
      if(v != s && beh_graph.CGetNodeInfo(v).GetOperation() == op_name)
      {
         ASAP_p[v]++;
      }
   }
};

void ASLAP::add_constraints_to_ASAP()
{
   unsigned int m_k;
   auto cur_et = 0u;

   /** ASAP_nip[i] contains the number of non immediate predecessor of node i with the same type of operation.*/
   vertex2int<> ASAP_nip;

   /** ASAP_p[i] contains the number of predecessor of node i with the same type of operation.*/
   vertex2int<> ASAP_p;

   ASAP_nip.resize(levels.begin(), levels.end(), 0);
   ASAP_p.resize(levels.begin(), levels.end(), 0);

   for(auto level : levels)
   {
      // Updating ASAP_p information
      p_update_check vis(level, beh_graph.CGetNodeInfo(level).GetOperation(), ASAP_p, beh_graph);
      beh_graph.DepthFirstVisit(level, vis);
      ASAP_nip[level] = ASAP_p[level];
      for(const auto& ei : beh_graph.in_edges(level))
      {
         auto v = beh_graph.source(ei);
         if(!operations.count(v))
         {
            continue;
         }
         if(beh_graph.CGetNodeInfo(v).GetOperation() == beh_graph.CGetNodeInfo(level).GetOperation())
         {
            ASAP_nip[level]--;
         }
      }
   }

   for(auto v : levels)
   {
      if(ASAP_p[v] == 0)
      {
         continue;
      }
      m_k = allocation_information.max_number_of_resources(v);
      cur_et = static_cast<unsigned int>(ceil(double(GetCycleLatency(v, Allocation_MinMax::MIN)) /
                                              static_cast<double>(ctrl_step_multiplier))) *
               std::max(static_cast<unsigned int>(ceil(static_cast<double>(ASAP_p[v]) / m_k)),
                        (1 + static_cast<unsigned int>(ceil(static_cast<double>(ASAP_nip[v]) / m_k))));
      if(cur_et > 0u)
      {
         --cur_et;
      }
      min_tot_csteps = min_tot_csteps < cur_et ? cur_et : min_tot_csteps;
      const auto schedule = ASAP.get_cstep(v).second < cur_et ? cur_et : ASAP.get_cstep(v).second;
      ASAP.set_execution(v, schedule);
   }
#ifndef NDEBUG
   PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_p: ");
   for(const auto& i : ASAP_p)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       beh_graph.CGetNodeInfo(i.first).vertex_name + " - " + std::to_string(i.second));
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

   PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_nip: ");
   for(const auto& i : ASAP_nip)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       beh_graph.CGetNodeInfo(i.first).vertex_name + " - " + std::to_string(i.second));
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");
#endif
}

void ASLAP::compute_ASAP(const Schedule* partial_schedule)
{
   // Store the current execution time
   double cur_start;
   vertex2float<> finish_time; //
   const auto [vI, vI_end] = boost::vertices(beh_graph);

   ASAP.clear();
   finish_time.resize(vI, vI_end, 0); //
   min_tot_csteps = 0u;
   if(partial_schedule)
   {
      for(auto level : levels)
      {
         if(partial_schedule && partial_schedule->is_scheduled(level))
         {
            ASAP.set_execution(level, partial_schedule->get_cstep(level).second);
         }
      }
   }
   if(WITH_CONSTRAINT && !has_branching_blocks) // When no IF statements are present this function returns 1.
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ASAP: add_constraints_to_ASAP");
      add_constraints_to_ASAP();
   }

   for(auto level : levels)
   {
      const auto op_cycles = GetCycleLatency(level, Allocation_MinMax::MIN);
      cur_start = 0.0;

      for(const auto& ei : beh_graph.in_edges(level))
      {
         const auto vi = beh_graph.source(ei);
         cur_start = finish_time.at(vi) < cur_start ? cur_start : finish_time.at(vi);
         //       PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, beh_graph.CGetNodeInfo(vi).vertex_name + " -> " +
         //       beh_graph.CGetNodeInfo(level).vertex_name + " cur_start " + std::to_string(cur_start));
      }

      finish_time.at(level) = cur_start + op_cycles;
      auto curr_asap = ASAP.is_scheduled(level) ? ASAP.get_cstep(level).second : 0u;
      curr_asap = static_cast<unsigned int>(cur_start / ctrl_step_multiplier) > curr_asap ?
                      static_cast<unsigned int>(cur_start / ctrl_step_multiplier) :
                      curr_asap;
      ASAP.set_execution(level, curr_asap);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    beh_graph.CGetNodeInfo(level).vertex_name + " cur_start " + std::to_string(cur_start) +
                        " finish_time[level] " + std::to_string(finish_time[level]));
      min_tot_csteps = min_tot_csteps < curr_asap ? curr_asap : min_tot_csteps;
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                    beh_graph.CGetNodeInfo(level).vertex_name + " - " + STR(ASAP.get_cstep(level).second));
   }
   ASAP.set_csteps(min_tot_csteps + 1u);
}

void ASLAP::add_constraints_to_ALAP()
{
   //
   // ALAP is computed as ASAP: 0 is the last step and at this point of the implementation ALAP[*i] is the distance from
   // last step
   unsigned int m_k;

   /** ALAP_nip[i] contains the number of non immediate predecessor of node i with the same type of operation.*/
   vertex2int<> ALAP_nip;
   /** ALAP_p[i] contains the number of predecessor of node i with the same type of operation.*/
   vertex2int<> ALAP_p;
   const auto R = beh_graph.reverse_graph();

   ALAP_nip.resize(levels.begin(), levels.end(), 0);
   ALAP_p.resize(levels.begin(), levels.end(), 0);
   auto iend = levels.rend();
   for(auto i = levels.rbegin(); i != iend; ++i)
   {
      const auto& op_info = beh_graph.CGetNodeInfo(*i);
      p_update_check vis(*i, op_info.GetOperation(), ALAP_p, beh_graph);
      R.DepthFirstVisit(*i, vis);
      ALAP_nip[*i] = ALAP_p[*i];
      for(const auto& ei : beh_graph.out_edges(*i))
      {
         auto v = beh_graph.target(ei);
         if(!operations.count(v))
         {
            continue;
         }
         if(beh_graph.CGetNodeInfo(v).GetOperation() == op_info.GetOperation())
         {
            ALAP_nip[*i]--;
         }
      }
   }
   auto cur_et = 0u;
   for(auto v : levels)
   {
      if(ALAP_p(v) == 0)
      {
         continue;
      }
      m_k = allocation_information.min_number_of_resources(v);
      cur_et = static_cast<unsigned int>(
          (ceil(double(GetCycleLatency(v, Allocation_MinMax::MIN)) / static_cast<double>(ctrl_step_multiplier))) *
          std::max(static_cast<unsigned int>(ceil(static_cast<double>(ALAP_p[v]) / m_k)),
                   (1 + static_cast<unsigned int>(ceil(static_cast<double>(ALAP_nip[v]) / m_k)))));
      max_tot_csteps = max_tot_csteps < cur_et ? cur_et : max_tot_csteps;

      const auto schedule = ALAP.get_cstep(v).second < cur_et ? cur_et : ALAP.get_cstep(v).second;
      ALAP.set_execution(v, schedule);
   }
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ALAP_p: ");
      vertex2int<>::const_iterator i_end = ALAP_p.end();
      for(vertex2int<>::const_iterator i = ALAP_p.begin(); i != i_end; ++i)
      {
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          beh_graph.CGetNodeInfo(i->first).vertex_name + " - " + std::to_string(i->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_nip: ");
      vertex2int<>::const_iterator ai_end = ALAP_nip.end();
      for(vertex2int<>::const_iterator ai = ALAP_nip.begin(); ai != ai_end; ++ai)
      {
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          beh_graph.CGetNodeInfo(ai->first).vertex_name + " - " + std::to_string(ai->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Reverse ALAP:");
#ifndef NDEBUG
      for(auto level : levels)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       beh_graph.CGetNodeInfo(level).vertex_name + " - " + STR(ALAP.get_cstep(level).second));
      }
#endif
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "end");
   }
}

void ASLAP::compute_ALAP(ALAP_method met, const Schedule* partial_schedule, bool* feasible,
                         const unsigned int est_upper_bound)
{
   switch(met)
   {
      case ALAP_fast:
         max_tot_csteps = 0u;
         THROW_ASSERT(!partial_schedule, "ASLAP::compute_ALAP - partial_schedule not expected");
         update_ALAP(0u, feasible);
         break;
      case ALAP_worst_case:
         // THROW_ASSERT(!part_sch, "ASLAP::compute_ALAP - !part_sch failed into ALAP_worst_case");
         ALAP.clear();
         max_tot_csteps = 0u;
         compute_ALAP_worst_case();
         break;
      case ALAP_with_upper:
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computing alap starting from list based");
         THROW_ASSERT(partial_schedule, "ASLAP::compute_ALAP - partial_schedule expected");
         max_tot_csteps = partial_schedule->get_csteps() - 1u;
         update_ALAP(max_tot_csteps);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computed alap");
         break;
      }
      case ALAP_with_upper_minus_one:
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computing alap starting from list based minus one");
         THROW_ASSERT(partial_schedule, "ASLAP::compute_ALAP - partial_schedule expected");
         if(partial_schedule->get_csteps() < 2u)
         {
            *feasible = false;
         }
         else
         {
            max_tot_csteps = partial_schedule->get_csteps() - 2u;
            update_ALAP(max_tot_csteps, feasible);
         }
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computed alap");
         break;
      }
      case ALAP_with_partial_scheduling:
      {
         THROW_ASSERT(partial_schedule, "ASLAP::compute_ALAP - partial_schedule expected");
         update_ALAP(est_upper_bound - 1u, feasible, partial_schedule);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computed alap");
         break;
      }
      default:
         THROW_ERROR("InconsistentDataStructure");
         break;
   }
   ALAP.set_csteps(max_tot_csteps + 1u);
}

void ASLAP::update_ALAP(const unsigned int maxc, bool* feasible, const Schedule* partial_schedule)
{
   ALAP.clear();
   max_tot_csteps = maxc;
   if(partial_schedule)
   {
      for(auto level : levels)
      {
         if(partial_schedule && partial_schedule->is_scheduled(level))
         {
            ALAP.set_execution(level, max_tot_csteps - partial_schedule->get_cstep(level).second);
         }
      }
   }
   if(WITH_CONSTRAINT && !has_branching_blocks) // When no IF statements are present this function returns 1.
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ALAP: add_constraints_to_ALAP");
      add_constraints_to_ALAP();
   }
   compute_ALAP_fast(feasible);
   if(feasible && *feasible && max_tot_csteps > maxc)
   {
      *feasible = false;
   }
}

void ASLAP::compute_ALAP_fast(bool* feasible)
{
   // This function is used both in fast case and
   double cur_rev_start;
   vertex2float<> Rev_finish_time;
   const auto [vI, vI_end] = boost::vertices(beh_graph);
   Rev_finish_time.resize(vI, vI_end, 0);

   auto i_end = levels.rend();
   for(auto i = levels.rbegin(); i != i_end; ++i)
   {
      const auto op_cycles = GetCycleLatency(*i, Allocation_MinMax::MIN);
      cur_rev_start = 0.0;
      for(const auto& ei : beh_graph.in_edges(*i))
      {
         const auto vi = beh_graph.target(ei);
         cur_rev_start = Rev_finish_time.at(vi) < cur_rev_start ? cur_rev_start : Rev_finish_time.at(vi);
      }
      Rev_finish_time.at(*i) = cur_rev_start + op_cycles;

      unsigned int rev_curr_alap = ALAP.is_scheduled(*i) ? ALAP.get_cstep(*i).second : 0u;
      const auto rev_finish_time = static_cast<unsigned int>((Rev_finish_time.at(*i) - 1) / ctrl_step_multiplier);
      rev_curr_alap = rev_finish_time > rev_curr_alap ? rev_finish_time : rev_curr_alap;
      ALAP.set_execution(*i, rev_curr_alap);
      max_tot_csteps = max_tot_csteps < rev_curr_alap ? rev_curr_alap : max_tot_csteps;
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ALAP fast");
   for(auto level : levels)
   {
      ALAP.set_execution(level, max_tot_csteps - ALAP.get_cstep(level).second);
      if(feasible && *feasible)
      {
         *feasible = ALAP.get_cstep(level) >= ASAP.get_cstep(level);
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                    beh_graph.CGetNodeInfo(level).vertex_name + " - " + STR(ALAP.get_cstep(level).second));
   }
}

void ASLAP::compute_ALAP_worst_case()
{
   // Store the max reverse level
   auto max_rev_level = 0u;
   // Store the current reverse level
   std::map<unsigned int, unsigned int> rev_levels_to_cycles;
   std::map<unsigned int, unsigned int> max_et;

   auto i_end = levels.rend();
   for(auto i = levels.rbegin(); i != i_end; ++i)
   {
      for(const auto& ei : beh_graph.in_edges(*i))
      {
         const auto vi = beh_graph.target(ei);
         if(!operations.count(vi))
         {
            continue;
         }
         const auto cur_rev_level = ALAP.get_cstep(vi).second + 1u;
         max_rev_level = std::max(max_rev_level, cur_rev_level);
         const auto schedule = ALAP.get_cstep(*i).second < cur_rev_level ? cur_rev_level : ALAP.get_cstep(*i).second;
         ALAP.set_execution(*i, schedule);
      }
      if(rev_levels_to_cycles.find(ALAP.get_cstep(*i).second) == rev_levels_to_cycles.end())
      {
         rev_levels_to_cycles.emplace(ALAP.get_cstep(*i).second, 0u);
         max_et.emplace(ALAP.get_cstep(*i).second, 0u);
      }
      rev_levels_to_cycles.find(ALAP.get_cstep(*i).second)->second +=
          static_cast<unsigned int>(allocation_information.get_attribute_of_fu_per_op(
              *i, beh_graph, Allocation_MinMax::MAX, AllocationInformation::initiation_time));
      max_et.emplace(ALAP.get_cstep(*i).second,
                     std::max(max_et.find(ALAP.get_cstep(*i).second)->second,
                              static_cast<unsigned int>(ceil(double(GetCycleLatency(*i, Allocation_MinMax::MAX)) /
                                                             static_cast<double>(ctrl_step_multiplier)))));
   }
   auto levelr = max_rev_level - 1u;
   do
   {
      rev_levels_to_cycles.find(levelr)->second +=
          std::max(rev_levels_to_cycles.find(levelr + 1u)->second, max_et.find(levelr + 1u)->second);
   } while(levelr != 0u);
   for(auto level : levels)
   {
      ALAP.set_execution(level,
                         rev_levels_to_cycles.find(ALAP.get_cstep(level).second)->second -
                             static_cast<unsigned int>(allocation_information.get_attribute_of_fu_per_op(
                                 level, beh_graph, Allocation_MinMax::MAX, AllocationInformation::initiation_time)));
      max_tot_csteps = std::max(max_tot_csteps, ALAP.get_cstep(level).second);
   }
}

unsigned int ASLAP::GetCycleLatency(OpGraph::vertex_descriptor operation, Allocation_MinMax minmax) const
{
   const CustomOrderedSet<unsigned int>& fu_set = allocation_information.can_implement_set(operation);
   double execution_time = allocation_information.get_attribute_of_fu_per_op(operation, beh_graph, minmax,
                                                                             AllocationInformation::execution_time);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                 beh_graph.CGetNodeInfo(operation).vertex_name + " - ex=" + STR(execution_time));
   if(execution_time > 0.0)
   {
      return allocation_information.op_et_to_cycles(execution_time, clock_period / ctrl_step_multiplier);
   }
   else
   {
      for(auto const fu_type : fu_set)
      {
         if(allocation_information.get_stage_period(fu_type, operation, beh_graph) != 0.0)
         {
            return ctrl_step_multiplier * allocation_information.get_cycles(fu_type, operation, beh_graph);
         }
         if(!allocation_information.is_operation_bounded(beh_graph, operation, fu_type))
         {
            return ctrl_step_multiplier;
         }
      }
   }
   return 0u;
}
