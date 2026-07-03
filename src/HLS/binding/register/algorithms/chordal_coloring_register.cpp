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
 * @file chordal_coloring_register.cpp
 * @brief Class implementation of register allocation algorithm based on chordal algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "chordal_coloring_register.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveVariables.hpp"
#include "reg_binding.hpp"
#include "storage_value_information.hpp"
#include "utility.hpp"

#include <boost/graph/sequential_vertex_coloring.hpp>

#include <vector>

chordal_coloring_register::chordal_coloring_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                                     unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : conflict_based_register(_Param, _HLSMgr, _funId, _design_flow_manager,
                              HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING)
{
}

bool chordal_coloring_register::lex_compare_gt(const std::vector<unsigned int>& v1,
                                               const std::vector<unsigned int>& v2) const
{
   /*
   std::cout << "v1 ";
   std::copy(v1.begin(), v1.end(), std::ostream_iterator<unsigned int>(std::cout, " "));
   std::cout << "\nv2 ";
   std::copy(v2.begin(), v2.end(), std::ostream_iterator<unsigned int>(std::cout, " "));
   std::cout << "\n";
   */
   size_t v1_size = v1.size();
   if(v1_size == 0)
   {
      return false;
   }
   else
   {
      size_t v2_size = v2.size();
      if(v2_size == 0)
      {
         return true;
      }
      else
      {
         for(unsigned int index = 0; index < v1_size && index < v2_size; ++index)
         {
            if(v1[index] > v2[index])
            {
               return true;
            }
            else if(v1[index] < v2[index])
            {
               return false;
            }
         }
         /// they are equal with respect to the short string
         if(v1_size > v2_size)
         {
            return true;
         }
         else
         {
            return false;
         }
      }
   }
}

DesignFlowStep_Status chordal_coloring_register::RegisterBinding()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   unsigned int cg_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
   auto cg_ptr = new conflict_graph(cg_num_vertices);
   conflict_graph& cg = *cg_ptr;
   create_conflict_graph(cg);
   const unsigned int NO_ORDER = std::numeric_limits<unsigned int>::max();
   std::vector<cg_vertex_descriptor> vertex_order(cg_num_vertices);

   std::vector<std::vector<unsigned int>> label(cg_num_vertices);
   std::vector<unsigned int> seq(cg_num_vertices, NO_ORDER);

   for(unsigned int irev = 0; irev < cg_num_vertices; ++irev)
   {
      unsigned int i = cg_num_vertices - irev - 1;
      /// search vertex vx with maximum label on unnumbered vertex
      unsigned int vx_index = 0;
      bool found;
      found = false;
      for(unsigned int vindex = 0; vindex < cg_num_vertices; ++vindex)
      {
         if(seq[vindex] == NO_ORDER)
         {
            if(!found)
            {
               vx_index = vindex;
               found = true;
            }
            else if(lex_compare_gt(label[vindex], label[vx_index]))
            {
               vx_index = vindex;
            }
         }
      }
      THROW_ASSERT(found, "maximal not found");
      seq[vx_index] = i;
      auto vx = boost::vertex(vx_index, cg);
      vertex_order[i] = vx;
      // for each unnumbered vertex v adjacent to vx
      // label(v)=label(v) + i
      for(const auto adj : boost::make_iterator_range(boost::adjacent_vertices(vx, cg)))
      {
         long unsigned int vindex = get(boost::vertex_index, cg, adj);
         if(seq[vindex] == NO_ORDER)
         {
            bool add = true;
            for(auto it : label[vindex])
            {
               if(it == i)
               {
                  add = false;
                  break;
               }
            }
            if(add)
            {
               label[vindex].push_back(i); // append the label
            }
         }
      }
   }

   /// sequential vertex coloring based on left edge sorting
   auto num_colors = boost::sequential_vertex_coloring(
       cg,
       boost::make_iterator_property_map(vertex_order.begin(), boost::identity_property_map(),
                                         boost::graph_traits<conflict_graph>::null_vertex()),
       color);

   /// finalize
   HLS->Rreg = reg_bindingRef(new reg_binding(HLS, HLSMgr));
   const auto states = HLS->fsm_info->vertices();

   for(const auto v : states)
   {
      const auto& live = HLS->Rliv->getLiveInFsmVariables(v);
      for(const auto& [var, stage] : live)
      {
         const auto storage_value_index = HLS->storage_value_information->get_storage_value_index(v, var, stage);
         HLS->Rreg->bind(storage_value_index, static_cast<unsigned int>(color[boost::vertex(storage_value_index, cg)]));
      }
   }
   HLS->Rreg->set_used_regs(static_cast<unsigned int>(num_colors));
   delete cg_ptr;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->Register binding information for function " +
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetFunctionName() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  std::string("---Register allocation algorithm obtains ") +
                      (num_colors == register_lower_bound ? "an optimal" : "a sub-optimal") +
                      " result: " + STR(num_colors) + " registers" +
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
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   return DesignFlowStep_Status::SUCCESS;
}
