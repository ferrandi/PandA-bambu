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
 *              Copyright (C) 2014-2026 Politecnico di Milano
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
 * @file sdc_scheduling_base.hpp
 * @brief SDC scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SDC_SCHEDULING_BASE_HPP
#define SDC_SCHEDULING_BASE_HPP

#include "SchedulingStep.hpp"
#include "custom_map.hpp"
#include "op_graph.hpp"

CONSTREF_FORWARD_DECL(FunctionBehavior);

#include <list>
#include <utility>
#include <vector>

class SDCScheduling_base : public SchedulingStep
{
 public:
   /// Result of SPECULATIVE_LOOP: the list of movement to be performed (first element is the operation, second element
   /// is the old basic block, third element is the new basic block) Movements have to be performed in order
   std::list<std::vector<unsigned int>> movements_list;

   SDCScheduling_base(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                      const DesignFlowManager& _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                      const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization);

   virtual ~SDCScheduling_base() override = default;
};

/**
 * Class used to sort operation using ALAP in ascending order as primary key and ASAP ascending order as secondary key
 */
struct SDCSorter
{
 private:
   /// The function behavior
   const FunctionBehaviorConstRef function_behavior;

   /// The operation graph
   const OpGraph& op_graph;

   /// The index basic block map
   const CustomUnorderedMap<unsigned int, OpGraph::vertex_descriptor>& bb_index_map;

   using ComparisonKey = std::pair<OpGraph::vertex_descriptor, OpGraph::vertex_descriptor>;
   mutable CustomUnorderedMapUnstable<ComparisonKey, bool> comparison_cache;

 public:
   /**
    * Constructor
    * @param _function_behavior is the function behavior
    * @param _op_graph is the operation graph
    */
   explicit SDCSorter(const FunctionBehaviorConstRef _function_behavior, const OpGraph& _op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in topological sort, false otherwise
    */
   bool operator()(OpGraph::vertex_descriptor x, OpGraph::vertex_descriptor y) const;
};

#endif
