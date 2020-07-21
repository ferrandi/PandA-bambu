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
 * @file chordal_coloring_register.cpp
 * @brief Class implementation of register allocation algorithm based on chordal algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "chordal_coloring_register.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "liveness.hpp"
#include "reg_binding.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

#include <boost/graph/sequential_vertex_coloring.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "cpu_time.hpp"

chordal_coloring_register::chordal_coloring_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : conflict_based_register(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING)
{
}

chordal_coloring_register::~chordal_coloring_register() = default;

bool chordal_coloring_register::lex_compare_gt(const std::vector<unsigned int>& v1, const std::vector<unsigned int>& v2) const
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
      return false;
   else
   {
      size_t v2_size = v2.size();
      if(v2_size == 0)
         return true;
      else
      {
         for(unsigned int index = 0; index < v1_size && index < v2_size; ++index)
         {
            if(v1[index] > v2[index])
               return true;
            else if(v1[index] < v2[index])
               return false;
         }
         /// they are equal with respect to the short string
         if(v1_size > v2_size)
            return true;
         else
            return false;
      }
   }
}

DesignFlowStep_Status chordal_coloring_register::InternalExec()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   create_conflict_graph();
   unsigned int cg_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
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
      cg_vertex_descriptor vx = boost::vertex(vx_index, cg);
      vertex_order[i] = vx;
      // for each unnumbered vertex v adjacent to vx
      // label(v)=label(v) + i
      boost::graph_traits<conflict_graph>::adjacency_iterator adj_i, adj_e;
      for(boost::tie(adj_i, adj_e) = boost::adjacent_vertices(vx, cg); adj_i != adj_e; ++adj_i)
      {
         long unsigned int vindex = get(boost::vertex_index, cg, *adj_i);
         if(seq[vindex] == NO_ORDER)
         {
            bool add;
            add = true;
            std::vector<unsigned int>::const_iterator it_end = label[vindex].end();
            for(std::vector<unsigned int>::const_iterator it = label[vindex].begin(); it != it_end && add; ++it)
               if(*it == i)
                  add = false;
            if(add)
               label[vindex].push_back(i); // append the label
         }
      }
   }

   /// sequential vertex coloring based on left edge sorting
   cg_vertices_size_type num_colors = boost::sequential_vertex_coloring(cg, boost::make_iterator_property_map(vertex_order.begin(), boost::identity_property_map(), boost::graph_traits<conflict_graph>::null_vertex()), color);

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
         HLS->Rreg->bind(storage_value_index, static_cast<unsigned int>(color[boost::vertex(storage_value_index, cg)]));
      }
   }
   HLS->Rreg->set_used_regs(static_cast<unsigned int>(num_colors));
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Register binding information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  std::string("---Register allocation algorithm obtains ") + (num_colors == register_lower_bound ? "an optimal" : "a sub-optimal") + " result: " + STR(num_colors) + " registers" +
                      (num_colors == register_lower_bound ? "" : ("(LB:" + STR(register_lower_bound) + ")")));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      HLS->Rreg->print();
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform register binding: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
