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
 * @file ASLAP.cpp
 * @brief Class implementation for ASLAP class methods.
 *
 * This file implements some of the ASLAP member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "ASLAP.hpp"
#include "allocation.hpp"
#include "basic_block.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "op_graph.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#include "Vertex.hpp"
#include "fu_binding.hpp"
#include "schedule.hpp"

///. include
#include "Parameter.hpp"

/// boost include
#include <boost/graph/reverse_graph.hpp>

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// STD include
#include <cmath>

/// tree include
#include "behavioral_helper.hpp"

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

// in case asap and alap are computed with/without constraints on the resources available
#define WITH_CONSTRAINT 0

ASLAP::ASLAP(const HLS_managerConstRef _hls_manager, const hlsRef HLS, const bool _speculation, const OpVertexSet& operations, const ParameterConstRef _parameters, unsigned int _ctrl_step_multiplier)
    : ASAP(
          ScheduleRef(new Schedule(_hls_manager, _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->get_function_index(), _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetOpGraph(FunctionBehavior::FLSAODG), _parameters))),
      ALAP(
          ScheduleRef(new Schedule(_hls_manager, _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetBehavioralHelper()->get_function_index(), _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetOpGraph(FunctionBehavior::FLSAODG), _parameters))),
      min_tot_csteps(0u),
      max_tot_csteps(0u),
      has_branching_blocks(false),
      allocation_information(HLS->allocation_information),
      speculation(_speculation),
      clock_period(HLS->HLS_C->get_clock_period() * HLS->HLS_C->get_clock_period_resource_fraction()),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this))),
      ctrl_step_multiplier(_ctrl_step_multiplier)
{
   if(speculation)
      beh_graph = _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetOpGraph(FunctionBehavior::SG, operations);
   else
      beh_graph = _hls_manager->CGetFunctionBehavior(HLS->functionId)->CGetOpGraph(FunctionBehavior::FLSAODG, operations);
   VertexIterator it, end_it;
   for(boost::tie(it, end_it) = boost::vertices(*beh_graph); !has_branching_blocks && it != end_it; it++)
   {
      if(GET_TYPE(beh_graph, *it) & (TYPE_IF | TYPE_SWITCH | TYPE_WHILE | TYPE_FOR))
         has_branching_blocks = true;
   }
   const std::deque<vertex>& ls = _hls_manager->CGetFunctionBehavior(HLS->functionId)->get_levels();
   for(auto l : ls)
   {
      if(operations.find(l) != operations.end())
         levels.push_back(l);
   }
}

const OpGraphConstRef ASLAP::CGetOpGraph() const
{
   return beh_graph;
}

void ASLAP::print(std::ostream& os) const
{
   if(ASAP->num_scheduled())
   {
      os << "ASAP\n";
      ASAP->print();
      os << std::endl;
   }
   if(ALAP->num_scheduled())
   {
      os << "ALAP\n";
      ALAP->print();
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
   vertex s;
   /// string that identifies operation name
   const std::string& op_name;
   /// asap values
   vertex2int& ASAP_p;
   /// behavioral specification in terms of graph
   const OpGraphConstRef beh_graph;

 public:
   /**
    * Constructor
    */
   p_update_check(vertex v, const std::string& name, vertex2int& A_p, const OpGraphConstRef g) : s(v), op_name(name), ASAP_p(A_p), beh_graph(g)
   {
   }

   /**
    * Template function used to discover vertex
    */
   template <class Vertex, class Graph>
   void discover_vertex(Vertex v, const Graph&) const
   {
      if(v != s && beh_graph->CGetOpNodeInfo(v)->GetOperation() == op_name)
         ASAP_p[v]++;
   }
};

void ASLAP::add_constraints_to_ASAP()
{
   vertex v;
   InEdgeIterator ei, ei_end;
   unsigned int m_k;
   ControlStep cur_et = ControlStep(0u);

   /** ASAP_nip[i] contains the number of non immediate predecessor of node i with the same type of operation.*/
   vertex2int ASAP_nip;

   /** ASAP_p[i] contains the number of predecessor of node i with the same type of operation.*/
   vertex2int ASAP_p;

   ASAP_nip.resize(levels.begin(), levels.end(), 0);
   ASAP_p.resize(levels.begin(), levels.end(), 0);

   for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      // Updating ASAP_p information
      p_update_check vis(*i, beh_graph->CGetOpNodeInfo(*i)->GetOperation(), ASAP_p, beh_graph);
      std::vector<boost::default_color_type> color_vec(boost::num_vertices(*beh_graph));
      boost::depth_first_visit(*beh_graph, *i, vis, boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), *beh_graph), boost::white_color));
      ASAP_nip[*i] = ASAP_p[*i];
      for(boost::tie(ei, ei_end) = boost::in_edges(*i, *beh_graph); ei != ei_end; ei++)
      {
         v = boost::source(*ei, *beh_graph);
         if(beh_graph->CGetOpNodeInfo(v)->GetOperation() == beh_graph->CGetOpNodeInfo(*i)->GetOperation())
            ASAP_nip[*i]--;
      }
   }

   VertexIterator vi, vi_end;

   for(boost::tie(vi, vi_end) = boost::vertices(*beh_graph); vi != vi_end; vi++)
   {
      v = *vi;
      if(ASAP_p[v] == 0)
         continue;
      m_k = allocation_information->max_number_of_resources(v);
      cur_et = ControlStep(static_cast<unsigned int>(ceil(from_strongtype_cast<double>(GetCycleLatency(v, Allocation_MinMax::MIN)) / static_cast<double>(ctrl_step_multiplier))) *
                           std::max(static_cast<unsigned int>(ceil(static_cast<double>(ASAP_p[v]) / m_k)), (1 + static_cast<unsigned int>(ceil(static_cast<double>(ASAP_nip[v]) / m_k)))));
      if(cur_et > 0u)
         --cur_et;
      min_tot_csteps = min_tot_csteps < cur_et ? cur_et : min_tot_csteps;
      const auto schedule = ASAP->get_cstep(v).second < cur_et ? cur_et : ASAP->get_cstep(v).second;
      ASAP->set_execution(v, schedule);
   }
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_p: ");
      vertex2int::const_iterator i_end = ASAP_p.end();
      for(vertex2int::const_iterator i = ASAP_p.begin(); i != i_end; ++i)
      {
         if(!beh_graph->is_in_subset(i->first))
            continue;
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(beh_graph, i->first) + " - " + boost::lexical_cast<std::string>(i->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_nip: ");
      vertex2int::const_iterator ai_end = ASAP_nip.end();
      for(vertex2int::const_iterator ai = ASAP_nip.begin(); ai != ai_end; ++ai)
      {
         if(!beh_graph->is_in_subset(ai->first))
            continue;
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(beh_graph, ai->first) + " - " + boost::lexical_cast<std::string>(ai->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");
   }
}

void ASLAP::compute_ASAP(const ScheduleConstRef partial_schedule)
{
   vertex vi;
   InEdgeIterator ei, ei_end;
   // Store the current execution time
   double cur_start;
   vertex2float finish_time; //

   ASAP->clear();
   finish_time.clear();                                 //
   finish_time.resize(levels.begin(), levels.end(), 0); //
   min_tot_csteps = ControlStep(0u);
   if(partial_schedule)
   {
      for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
      {
         if(!beh_graph->is_in_subset(*i))
            continue;
         if(partial_schedule && partial_schedule->is_scheduled(*i))
         {
            ASAP->set_execution(*i, partial_schedule->get_cstep(*i).second);
         }
      }
   }

   if(WITH_CONSTRAINT && !has_branching_blocks) // When no IF statements are present this function returns 1.
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ASAP: add_constraints_to_ASAP");
      add_constraints_to_ASAP();
   }

   for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      const auto op_cycles = GetCycleLatency(*i, Allocation_MinMax::MIN);
      cur_start = 0.0;

      for(boost::tie(ei, ei_end) = boost::in_edges(*i, *beh_graph); ei != ei_end; ei++)
      {
         vi = boost::source(*ei, *beh_graph);
         cur_start = finish_time[vi] < cur_start ? cur_start : finish_time[vi];
         //       PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, GET_NAME(beh_graph, vi) + " -> " +GET_NAME(beh_graph, *i) + " cur_start " + boost::lexical_cast<std::string>(cur_start));
      }

      finish_time[*i] = cur_start + from_strongtype_cast<double>(op_cycles);
      ControlStep curr_asap = ASAP->is_scheduled(*i) ? ASAP->get_cstep(*i).second : ControlStep(0u);
      curr_asap = ControlStep(static_cast<unsigned int>(cur_start / ctrl_step_multiplier)) > curr_asap ? ControlStep(static_cast<unsigned int>(cur_start / ctrl_step_multiplier)) : curr_asap;
      ASAP->set_execution(*i, curr_asap);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, GET_NAME(beh_graph, *i) + " cur_start " + boost::lexical_cast<std::string>(cur_start) + " finish_time[*i] " + boost::lexical_cast<std::string>(finish_time[*i]));
      min_tot_csteps = min_tot_csteps < curr_asap ? curr_asap : min_tot_csteps;
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, GET_NAME(beh_graph, *i) + " - " + STR(ASAP->get_cstep(*i).second));
   }
   ASAP->set_csteps(min_tot_csteps + 1u);
}

void ASLAP::add_constraints_to_ALAP()
{
   //
   // ALAP is computed as ASAP: 0 is the last step and at this point of the implementation ALAP[*i] is the distance from last step
   vertex v;
   OutEdgeIterator ei, ei_end;
   unsigned int m_k;

   /** ALAP_nip[i] contains the number of non immediate predecessor of node i with the same type of operation.*/
   vertex2int ALAP_nip;
   /** ALAP_p[i] contains the number of predecessor of node i with the same type of operation.*/
   vertex2int ALAP_p;
   const boost::reverse_graph<graph> R(*beh_graph);

   ALAP_nip.resize(levels.begin(), levels.end(), 0);
   ALAP_p.resize(levels.begin(), levels.end(), 0);
   std::deque<vertex>::const_reverse_iterator iend = levels.rend();
   for(std::deque<vertex>::const_reverse_iterator i = levels.rbegin(); i != iend; ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      p_update_check vis(*i, beh_graph->CGetOpNodeInfo(*i)->GetOperation(), ALAP_p, beh_graph);
      std::vector<boost::default_color_type> color_vec(boost::num_vertices(R));
      boost::depth_first_visit(R, *i, vis, boost::make_iterator_property_map(color_vec.begin(), boost::get(boost::vertex_index_t(), R), boost::white_color));
      ALAP_nip[*i] = ALAP_p[*i];
      for(boost::tie(ei, ei_end) = boost::out_edges(*i, *beh_graph); ei != ei_end; ei++)
      {
         v = boost::target(*ei, *beh_graph);
         if(beh_graph->CGetOpNodeInfo(v)->GetOperation() == beh_graph->CGetOpNodeInfo(*i)->GetOperation())
            ALAP_nip[*i]--;
      }
   }
   ControlStep cur_et = ControlStep(0u);
   VertexIterator vi, vi_end;
   for(boost::tie(vi, vi_end) = boost::vertices(*beh_graph); vi != vi_end; vi++)
   {
      v = *vi;
      if(ALAP_p(v) == 0)
         continue;
      m_k = allocation_information->min_number_of_resources(v);
      cur_et = ControlStep(static_cast<unsigned int>((ceil(from_strongtype_cast<double>(GetCycleLatency(v, Allocation_MinMax::MIN)) / static_cast<double>(ctrl_step_multiplier))) *
                                                     std::max(static_cast<unsigned int>(ceil(static_cast<double>(ALAP_p[v]) / m_k)), (1 + static_cast<unsigned int>(ceil(static_cast<double>(ALAP_nip[v]) / m_k))))));
      max_tot_csteps = max_tot_csteps < cur_et ? cur_et : max_tot_csteps;

      const auto schedule = ALAP->get_cstep(v).second < cur_et ? cur_et : ALAP->get_cstep(v).second;
      ALAP->set_execution(v, schedule);
   }
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ALAP_p: ");
      vertex2int::const_iterator i_end = ALAP_p.end();
      for(vertex2int::const_iterator i = ALAP_p.begin(); i != i_end; ++i)
      {
         if(!beh_graph->is_in_subset(i->first))
            continue;
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(beh_graph, i->first) + " - " + boost::lexical_cast<std::string>(i->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

      PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "ASAP_nip: ");
      vertex2int::const_iterator ai_end = ALAP_nip.end();
      for(vertex2int::const_iterator ai = ALAP_nip.begin(); ai != ai_end; ++ai)
      {
         if(!beh_graph->is_in_subset(ai->first))
            continue;
         PRINT_DBG_STRING(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(beh_graph, ai->first) + " - " + boost::lexical_cast<std::string>(ai->second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Reverse ALAP:");
      for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
      {
         if(!beh_graph->is_in_subset(*i))
            continue;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_NAME(beh_graph, *i) + " - " + STR(ALAP->get_cstep(*i).second));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "end");
   }
}

void ASLAP::compute_ALAP(ALAP_method met, const ScheduleConstRef partial_schedule, bool* feasible, const ControlStep est_upper_bound)
{
   switch(met)
   {
      case ALAP_fast:
         max_tot_csteps = ControlStep(0u);
         THROW_ASSERT(!partial_schedule, "ASLAP::compute_ALAP - partial_schedule not expected");
         update_ALAP(ControlStep(0u), feasible);
         break;
      case ALAP_worst_case:
         // THROW_ASSERT(!part_sch, "ASLAP::compute_ALAP - !part_sch failed into ALAP_worst_case");
         ALAP->clear();
         max_tot_csteps = ControlStep(0u);
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
            *feasible = false;
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
   ALAP->set_csteps(max_tot_csteps + 1u);
}

void ASLAP::update_ALAP(const ControlStep maxc, bool* feasible, const ScheduleConstRef partial_schedule)
{
   ALAP->clear();
   max_tot_csteps = maxc;
   if(partial_schedule)
   {
      for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
      {
         if(!beh_graph->is_in_subset(*i))
            continue;
         if(partial_schedule && partial_schedule->is_scheduled(*i))
         {
            ALAP->set_execution(*i, max_tot_csteps - partial_schedule->get_cstep(*i).second);
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
      *feasible = false;
}

void ASLAP::compute_ALAP_fast(bool* feasible)
{
   // This function is used both in fast case and
   vertex vi;
   OutEdgeIterator ei, ei_end;
   double cur_rev_start;
   vertex2float Rev_finish_time;                            //
   Rev_finish_time.clear();                                 //
   Rev_finish_time.resize(levels.begin(), levels.end(), 0); //

   std::deque<vertex>::const_reverse_iterator i_end = levels.rend();
   for(std::deque<vertex>::const_reverse_iterator i = levels.rbegin(); i != i_end; ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      const auto op_cycles = GetCycleLatency(*i, Allocation_MinMax::MIN);
      cur_rev_start = 0.0;
      for(boost::tie(ei, ei_end) = boost::out_edges(*i, *beh_graph); ei != ei_end; ei++)
      {
         vi = boost::target(*ei, *beh_graph);
         cur_rev_start = Rev_finish_time[vi] < cur_rev_start ? cur_rev_start : Rev_finish_time[vi];
      }
      Rev_finish_time[*i] = cur_rev_start + from_strongtype_cast<double>(op_cycles);

      ControlStep rev_curr_alap = ALAP->is_scheduled(*i) ? ALAP->get_cstep(*i).second : ControlStep(0u);
      const auto rev_finish_time = ControlStep(static_cast<unsigned int>((Rev_finish_time[*i] - 1) / ctrl_step_multiplier));
      rev_curr_alap = rev_finish_time > rev_curr_alap ? rev_finish_time : rev_curr_alap;
      ALAP->set_execution(*i, rev_curr_alap);
      max_tot_csteps = max_tot_csteps < rev_curr_alap ? rev_curr_alap : max_tot_csteps;
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ALAP fast");
   for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      ALAP->set_execution(*i, max_tot_csteps - ALAP->get_cstep(*i).second);
      if(feasible && *feasible)
      {
         *feasible = ALAP->get_cstep(*i) >= ASAP->get_cstep(*i);
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, GET_NAME(beh_graph, *i) + " - " + STR(ALAP->get_cstep(*i).second));
   }
}

void ASLAP::compute_ALAP_worst_case()
{
   vertex vi;
   OutEdgeIterator ei, ei_end;
   // Store the max reverse level
   ControlStep max_rev_level = ControlStep(0u);
   // Store the current reverse level
   std::map<ControlStep, ControlStep> rev_levels_to_cycles;
   std::map<ControlStep, ControlStep> max_et;

   std::deque<vertex>::const_reverse_iterator i_end = levels.rend();
   for(std::deque<vertex>::const_reverse_iterator i = levels.rbegin(); i != i_end; ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      for(boost::tie(ei, ei_end) = boost::out_edges(*i, *beh_graph); ei != ei_end; ei++)
      {
         vi = boost::target(*ei, *beh_graph);
         const ControlStep cur_rev_level = ALAP->get_cstep(vi).second + 1u;
         max_rev_level = std::max(max_rev_level, cur_rev_level);
         const auto schedule = ALAP->get_cstep(*i).second < cur_rev_level ? cur_rev_level : ALAP->get_cstep(*i).second;
         ALAP->set_execution(*i, schedule);
      }
      if(rev_levels_to_cycles.find(ALAP->get_cstep(*i).second) == rev_levels_to_cycles.end())
      {
         rev_levels_to_cycles.insert(std::pair<ControlStep, ControlStep>(ALAP->get_cstep(*i).second, ControlStep(0u)));
         max_et.insert(std::pair<ControlStep, ControlStep>(ALAP->get_cstep(*i).second, ControlStep(0u)));
      }
      rev_levels_to_cycles.find(ALAP->get_cstep(*i).second)->second += static_cast<unsigned int>(allocation_information->get_attribute_of_fu_per_op(*i, beh_graph, Allocation_MinMax::MAX, AllocationInformation::initiation_time));
      max_et.insert(
          std::pair<ControlStep, ControlStep>(ALAP->get_cstep(*i).second, std::max(max_et.find(ALAP->get_cstep(*i).second)->second,
                                                                                   ControlStep(static_cast<unsigned int>(ceil(from_strongtype_cast<double>(GetCycleLatency(*i, Allocation_MinMax::MAX)) / static_cast<double>(ctrl_step_multiplier)))))));
   }
   auto level = max_rev_level - 1u;
   do
   {
      rev_levels_to_cycles.find(level)->second += std::max(rev_levels_to_cycles.find(level + 1u)->second, max_et.find(level + 1u)->second);
   } while(level != 0u);
   for(std::deque<vertex>::const_iterator i = levels.begin(); i != levels.end(); ++i)
   {
      if(!beh_graph->is_in_subset(*i))
         continue;
      ALAP->set_execution(*i, rev_levels_to_cycles.find(ALAP->get_cstep(*i).second)->second -
                                  ControlStep(static_cast<unsigned int>(allocation_information->get_attribute_of_fu_per_op(*i, beh_graph, Allocation_MinMax::MAX, AllocationInformation::initiation_time))));
      max_tot_csteps = std::max(max_tot_csteps, ALAP->get_cstep(*i).second);
   }
}

ControlStep ASLAP::GetCycleLatency(const vertex operation, Allocation_MinMax minmax) const
{
   const CustomOrderedSet<unsigned int>& fu_set = allocation_information->can_implement_set(operation);
   double execution_time = allocation_information->get_attribute_of_fu_per_op(operation, beh_graph, minmax, AllocationInformation::execution_time);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, GET_NAME(beh_graph, operation) + " - ex=" + STR(execution_time));
   if(execution_time > 0.0)
   {
      return allocation_information->op_et_to_cycles(execution_time, clock_period / ctrl_step_multiplier);
   }
   else
   {
      for(auto const fu_type : fu_set)
      {
         if(allocation_information->get_stage_period(fu_type, operation, beh_graph) != 0.0)
         {
            return ControlStep(ctrl_step_multiplier) * allocation_information->get_cycles(fu_type, operation, beh_graph);
         }
         if(not allocation_information->is_operation_bounded(beh_graph, operation, fu_type))
         {
            return ControlStep(ctrl_step_multiplier);
         }
      }
   }
   return ControlStep(0u);
}
