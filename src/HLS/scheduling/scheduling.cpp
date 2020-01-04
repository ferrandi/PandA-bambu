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
 * @file scheduling.cpp
 * @brief Base class for all scheduling algorithms.
 *
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "scheduling.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "Parameter.hpp"

#include "allocation.hpp"
#include "fu_binding.hpp"
#include "hls_constraints.hpp"
#include "parallel_memory_fu_binding.hpp"
#include "refcount.hpp"

/// implemented algorithms
#include "parametric_list_based.hpp"
#if HAVE_ILP_BUILT && HAVE_EXPERIMENTAL
#include "ilp_loop_pipelining.hpp"
#include "ilp_scheduling.hpp"
#include "ilp_scheduling_new.hpp"
#include "meilp_solver.hpp"
#include "silp_scheduling.hpp"
#endif
#include "fixed_scheduling.hpp"

#include "polixml.hpp"

#include "dbgPrintHelper.hpp"
#include "op_graph.hpp"
#include "utility.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

Scheduling::Scheduling(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                       const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization), speculation(_Param->getOption<bool>(OPT_speculative))
{
}

Scheduling::~Scheduling() = default;

void Scheduling::Initialize()
{
   HLSFunctionStep::Initialize();
   if(HLS->Rsch)
   {
      HLS->Rsch->Initialize();
   }
   else
   {
      const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
      HLS->Rsch = ScheduleRef(new Schedule(HLSMgr, funId, FB->CGetOpGraph(FunctionBehavior::FLSAODG), parameters));
   }
   if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
   {
      HLS->Rfu = fu_bindingRef(new ParallelMemoryFuBinding(HLSMgr, funId, parameters));
   }
   else
   {
      HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
   }
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> Scheduling::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

unsigned int Scheduling::compute_b_tag_size(const OpGraphConstRef cdg, vertex controlling_vertex) const
{
   if(GET_TYPE(cdg.get(), controlling_vertex) & TYPE_ENTRY)
      return 1;
   else if(GET_TYPE(cdg.get(), controlling_vertex) & TYPE_IF)
      return 2;
   else if(GET_TYPE(cdg.get(), controlling_vertex) & TYPE_SWITCH)
   {
      THROW_ASSERT(switch_map_size.find(controlling_vertex) != switch_map_size.end(), "missing a controlling_vertex from the switch_map_size");
      return switch_map_size.find(controlling_vertex)->second;
   }
   else
      THROW_ERROR("Not yet supported conditional vertex");
   return 0;
}

unsigned int Scheduling::compute_b_tag(const EdgeDescriptor& e, const OpGraphConstRef cdg, CustomOrderedSet<unsigned int>::const_iterator& switch_it, CustomOrderedSet<unsigned int>::const_iterator& switch_it_end) const
{
   vertex controlling_vertex = boost::source(e, *cdg);
   if(GET_TYPE(cdg.get(), controlling_vertex) & TYPE_ENTRY)
      return 0;
   else if(GET_TYPE(cdg.get(), controlling_vertex) & TYPE_IF)
   {
      if(Cget_edge_info<OpEdgeInfo>(e, *cdg) && CDG_TRUE_CHECK(cdg.get(), e))
         return 0;
      else
         return 1;
   }
   else if(GET_TYPE(cdg, controlling_vertex) & TYPE_SWITCH)
   {
      const CustomOrderedSet<unsigned int>& switch_set = EDGE_GET_NODEID(cdg.get(), e, CDG_SELECTOR);
      switch_it = switch_set.begin();
      switch_it_end = switch_set.end();
      return 2;
   }
   else
      THROW_ERROR("Not yet supported conditional vertex");
   return 0;
}

unsigned int Scheduling::b_tag_normalize(vertex controlling_vertex, unsigned int b_tag_not_normalized) const
{
   auto snp_it = switch_normalizing_map.find(controlling_vertex);
   THROW_ASSERT(snp_it != switch_normalizing_map.end(), "this controlling vertex is not of switch type");
   auto snp_el_it = snp_it->second.find(b_tag_not_normalized);
   THROW_ASSERT(snp_el_it != snp_it->second.end(), "switch_normalizing_map not correctly initialized");
   return snp_el_it->second;
}

void Scheduling::init_switch_maps(vertex controlling_vertex, const OpGraphConstRef cdg)
{
   unsigned int curr_b_tag = 0;
   switch_normalizing_map.insert(std::pair<vertex, CustomUnorderedMapUnstable<unsigned int, unsigned int>>(controlling_vertex, CustomUnorderedMapUnstable<unsigned int, unsigned int>()));
   auto snp_it = switch_normalizing_map.find(controlling_vertex);
   OutEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::out_edges(controlling_vertex, *cdg); eo != eo_end; eo++)
   {
      const CustomOrderedSet<unsigned int>& switch_set = EDGE_GET_NODEID(cdg, *eo, CDG_SELECTOR);
      const CustomOrderedSet<unsigned int>::const_iterator switch_it_end = switch_set.end();
      for(auto switch_it = switch_set.begin(); switch_it != switch_it_end; ++switch_it)
         if(snp_it->second.find(*switch_it) == snp_it->second.end())
            snp_it->second[*switch_it] = curr_b_tag++;
   }
   switch_map_size[controlling_vertex] = curr_b_tag;
}

ControlStep Scheduling::anticipate_operations(const OpGraphConstRef dependence_graph)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Scheduling::anticipate_operations - Begin");
   /// The clock cycle
   const double clock_cycle = HLS->HLS_C->get_clock_period();

   /// Last control step
   ControlStep last_cs = ControlStep(0u);

   std::list<vertex> operations;
   boost::topological_sort(*dependence_graph, std::front_inserter(operations));
   std::list<vertex>::const_iterator v, v_end = operations.end();
   /// Checking vertex in topological order
   for(v = operations.begin(); v != v_end; ++v)
   {
      /// Checking if operations is time zero
      double execution_time = HLS->allocation_information->get_execution_time(HLS->Rfu->get_assign(*v), *v, dependence_graph);
      if(execution_time == 0.0)
      {
         InEdgeIterator ei, ei_end;
         for(boost::tie(ei, ei_end) = boost::in_edges(*v, *dependence_graph); ei != ei_end; ei++)
         {
            vertex source = boost::source(*ei, *dependence_graph);
            const auto ending_time = HLS->Rsch->get_cstep(source).second + HLS->allocation_information->op_et_to_cycles(HLS->allocation_information->get_execution_time(HLS->Rfu->get_assign(source), source, dependence_graph), clock_cycle);
            /// Operation can not be anticipated
            if(ending_time > HLS->Rsch->get_cstep(*v).second)
               break;

            if(ending_time == HLS->Rsch->get_cstep(*v).second && (GET_TYPE(dependence_graph, source) & (TYPE_IF | TYPE_WHILE | TYPE_FOR | TYPE_SWITCH)))
               break;
         }
         /// Operation can be anticipated
         if(ei == ei_end && HLS->Rsch->get_cstep(*v).second > 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Anticipating " + GET_NAME(dependence_graph, *v));
            HLS->Rsch->set_execution(*v, HLS->Rsch->get_cstep(*v).second - 1);
         }
         if(HLS->Rsch->get_cstep(*v).second > last_cs)
            last_cs = HLS->Rsch->get_cstep(*v).second;
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Scheduling::anticipate_operations - End");
   return last_cs;
}
