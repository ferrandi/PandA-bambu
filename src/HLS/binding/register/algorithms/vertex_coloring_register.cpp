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
 * @file vertex_coloring_register.cpp
 * @brief Class implementation of a coloring based register allocation algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "vertex_coloring_register.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "dsatur2_coloring.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveVariables.hpp"
#include "reg_binding.hpp"
#include "storage_value_information.hpp"
#include "utility.hpp"

#include <vector>

vertex_coloring_register::vertex_coloring_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                                   unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : conflict_based_register(_Param, _HLSMgr, _funId, _design_flow_manager,
                              HLSFlowStep_Type::COLORING_REGISTER_BINDING)
{
}

DesignFlowStep_Status vertex_coloring_register::RegisterBinding()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   unsigned int cg_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
   auto cg_ptr = std::make_unique<conflict_graph>(cg_num_vertices);
   conflict_graph& cg = *cg_ptr;
   create_conflict_graph(cg);

   /// coloring based on DSATUR 2 heuristic
   const auto num_colors = dsatur2_coloring(cg, color);

   /// finalize
   HLS->Rreg = reg_bindingRef(new reg_binding(HLS, HLSMgr));
   auto number_of_storage_values = HLS->storage_value_information->get_number_of_storage_values();
   for(auto vi = 0U; vi < number_of_storage_values; ++vi)
   {
      HLS->Rreg->bind(vi, static_cast<unsigned int>(color[vi]));
   }
   HLS->Rreg->set_used_regs(static_cast<unsigned int>(num_colors));
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level,
                  "-->Register binding information for function " +
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetFunctionName() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level,
                  std::string("---Register allocation algorithm obtains ") +
                      (num_colors == register_lower_bound ? "an optimal" : "a sub-optimal") +
                      " result: " + std::to_string(num_colors) + " registers" +
                      (num_colors == register_lower_bound ? "" : ("(LB:" + STR(register_lower_bound) + ")")));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      HLS->Rreg->print();
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "Time to perform register binding: " + print_cpu_time(step_time) + " seconds");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "<--");
   if(output_level == OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "");
   }
   return DesignFlowStep_Status::SUCCESS;
}
