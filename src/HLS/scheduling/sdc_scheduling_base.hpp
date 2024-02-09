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
 *              Copyright (C) 2014-2024 Politecnico di Milano
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
 * @file sdc_scheduling_base.hpp
 * @brief SDC scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SDC_SCHEDULING_BASE_HPP
#define SDC_SCHEDULING_BASE_HPP

#include "scheduling_base_step.hpp"

CONSTREF_FORWARD_DECL(FunctionBehavior);

#include <list>
#include <vector>

class SDCScheduling_base : public schedulingBaseStep
{
 public:
   /// Result of SPECULATIVE_LOOP: the list of movement to be performed (first element is the operation, second element
   /// is the old basic block, third element is the new basic block) Movements have to be performed in order
   std::list<std::vector<unsigned int>> movements_list;

   SDCScheduling_base(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                      const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                      const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
       : schedulingBaseStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                            _hls_flow_step_specialization)
   {
   }
};

/**
 * Class used to sort operation using ALAP in ascending order as primary key and ASAP ascending order as secondary key
 */
struct SDCSorter : std::binary_function<vertex, vertex, bool>
{
 private:
   /// The function behavior
   const FunctionBehaviorConstRef function_behavior;

   /// The operation graph
   const OpGraphConstRef op_graph;

   /// The index basic block map
   const CustomUnorderedMap<unsigned int, vertex>& bb_index_map;

 public:
   /**
    * Constructor
    * @param _asap is the asap information
    * @param _alap is the alap information
    * @param _function_behavior is the function behavior
    * @param _op_graph is the operation graph
    * @param _statements_list is the list of the statements of the basic block
    * @param _parameters is the set of input parameters
    */
   explicit SDCSorter(const FunctionBehaviorConstRef _function_behavior, const OpGraphConstRef _op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in topological sort, false otherwise
    */
   bool operator()(const vertex& x, const vertex& y) const;
};

#endif
