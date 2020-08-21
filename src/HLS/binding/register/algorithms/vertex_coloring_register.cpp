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
 * @file vertex_coloring_register.cpp
 * @brief Class implementation of a coloring based register allocation algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "vertex_coloring_register.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "liveness.hpp"
#include "reg_binding.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "dsatur2_coloring.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "cpu_time.hpp"

vertex_coloring_register::vertex_coloring_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : conflict_based_register(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::COLORING_REGISTER_BINDING)
{
}

vertex_coloring_register::~vertex_coloring_register() = default;

DesignFlowStep_Status vertex_coloring_register::InternalExec()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   create_conflict_graph();
   /// coloring based on DSATUR 2 heuristic
   cg_vertices_size_type num_colors = dsatur2_coloring(cg, color);

   /// finalize
   HLS->Rreg = reg_bindingRef(new reg_binding(HLS, HLSMgr));
   const std::list<vertex>& support = HLS->Rliv->get_support();

   const std::list<vertex>::const_iterator vEnd = support.end();
   for(auto vIt = support.begin(); vIt != vEnd; ++vIt)
   {
      const CustomOrderedSet<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
      auto k_end = live.end();
      for(auto k = live.begin(); k != k_end; ++k)
      {
         unsigned int storage_value_index = HLS->storage_value_information->get_storage_value_index(*vIt, *k);
         HLS->Rreg->bind(storage_value_index, static_cast<unsigned int>(color[storage_value_index]));
      }
   }
   HLS->Rreg->set_used_regs(static_cast<unsigned int>(num_colors));
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "-->Register binding information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level,
                  std::string("---Register allocation algorithm obtains ") + (num_colors == register_lower_bound ? "an optimal" : "a sub-optimal") + " result: " + std::to_string(num_colors) + " registers" +
                      (num_colors == register_lower_bound ? "" : ("(LB:" + STR(register_lower_bound) + ")")));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      HLS->Rreg->print();
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform register binding: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "<--");
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
