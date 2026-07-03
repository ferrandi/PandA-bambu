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
 * @file weighted_clique_register.cpp
 * @brief Weighted clique covering register allocation procedure
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "weighted_clique_register.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "cdfc_module_binding.hpp"
#include "clique_covering.hpp"
#include "cpu_time.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveVariables.hpp"
#include "reg_binding.hpp"
#include "storage_value_information.hpp"

#include <filesystem>

template <typename vertex_type>
struct register_filter_clique : public filter_clique<vertex_type>
{
   bool select_candidate_to_remove(const CustomOrderedSet<C_vertex>&, C_vertex&,
                                   const CustomUnorderedMap<C_vertex, vertex_type>&,
                                   const cc_compatibility_graph&) const override
   {
      return false;
   }
   size_t clique_cost(const CustomOrderedSet<C_vertex>& candidate_clique,
                      const CustomUnorderedMap<C_vertex, vertex_type>&) const override
   {
      return candidate_clique.size();
   }
   virtual size_t sharing_cost(C_vertex v1, const CustomUnorderedSet<C_vertex>& candidate_clique,
                               const CustomUnorderedMap<C_vertex, vertex_type>&) const override
   {
      size_t acc_cost = 0;
      auto max_weight = HLS->storage_value_information->get_max_weight();
      auto vv1 = static_cast<unsigned>(v1);
      for(auto v2 : candidate_clique)
      {
         auto vv2 = static_cast<unsigned>(v2);
         auto deltaCost = 1 + max_weight - HLS->storage_value_information->get_compatibility_weight(vv1, vv2);
         THROW_ASSERT(deltaCost > 0, "unexpected value");
         acc_cost += static_cast<size_t>(deltaCost);
      }
      return acc_cost;
   }
   bool is_filtering() const override
   {
      return false;
   }
   register_filter_clique(const hlsRef _HLS) : HLS(_HLS)
   {
   }

 private:
   /// HLS data structure of the function to be analyzed
   const hlsRef HLS;
};

WeightedCliqueRegisterBindingSpecialization::WeightedCliqueRegisterBindingSpecialization(
    const CliqueCovering_Algorithm _clique_covering_algorithm)
    : clique_covering_algorithm(_clique_covering_algorithm)
{
}

std::string WeightedCliqueRegisterBindingSpecialization::GetName() const
{
   return cliqueCoveringAlgorithmToString(clique_covering_algorithm);
}

HLSFlowStepSpecialization::context_t WeightedCliqueRegisterBindingSpecialization::GetSignatureContext() const
{
   return ComputeSignatureContext(WEIGHTED_CLIQUE_REGISTER, static_cast<unsigned char>(clique_covering_algorithm));
}

weighted_clique_register::weighted_clique_register(
    const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
    const DesignFlowManager& _design_flow_manager,
    const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : reg_binding_creator(
          _parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING,
          _hls_flow_step_specialization ?
              _hls_flow_step_specialization :
              HLSFlowStepSpecializationConstRef(new WeightedCliqueRegisterBindingSpecialization(
                  _parameters->getOption<CliqueCovering_Algorithm>(OPT_weighted_clique_register_algorithm))))
{
}

void weighted_clique_register::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rreg = reg_binding::create_reg_binding(HLS, HLSMgr);
}

DesignFlowStep_Status weighted_clique_register::RegisterBinding()
{
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   auto number_of_storage_values = HLS->storage_value_information->get_number_of_storage_values();
   const auto clique_covering_algorithm =
       GetPointer<const WeightedCliqueRegisterBindingSpecialization>(hls_flow_step_specialization)
           ->clique_covering_algorithm;
   const auto register_clique =
       clique_covering<unsigned>::create_solver(clique_covering_algorithm, number_of_storage_values);
   HLS->storage_value_information->Initialize();
   for(auto vi = 0U; vi < number_of_storage_values; ++vi)
   {
      register_clique->add_vertex(vi, STR(vi));
   }
   unsigned int num_registers = 0;
   if(number_of_storage_values > 0)
   {
      if(clique_covering_algorithm == CliqueCovering_Algorithm::BIPARTITE_MATCHING)
      {
         const auto states = HLS->fsm_info->vertices();
         unsigned current_partition = 0;
         for(auto vState : states)
         {
            const auto& live = HLS->Rliv->getLiveInFsmVariables(vState);
            for(auto l : live)
            {
               unsigned int sv = HLS->storage_value_information->get_storage_value_index(vState, l.first, l.second);
               register_clique->add_subpartitions(current_partition, sv);
            }
            ++current_partition;
         }
      }
      else
      {
         interferenceGraphClass interferenceGraph;
         const auto states = HLS->fsm_info->vertices();
         /// create the interference graph
         for(const auto state : states)
         {
            const auto& live = HLS->Rliv->getLiveInFsmVariables(state);
            register_lower_bound = std::max(static_cast<unsigned int>(live.size()), register_lower_bound);
            const auto k_end = live.cend();
            for(auto k = live.cbegin(); k != k_end; ++k)
            {
               auto k_inner = k;
               ++k_inner;
               const auto v1 = HLS->storage_value_information->get_storage_value_index(state, k->first, k->second);
               THROW_ASSERT(v1 < number_of_storage_values, "wrong compatibility graph index");
               while(k_inner != k_end)
               {
                  const auto v2 =
                      HLS->storage_value_information->get_storage_value_index(state, k_inner->first, k_inner->second);
                  THROW_ASSERT(v2 < number_of_storage_values, "wrong compatibility graph index");
                  interferenceGraph.add_edge(v1, v2);
                  ++k_inner;
               }
            }
         }

         for(auto v2 = 1U; v2 < number_of_storage_values; ++v2)
         {
            // std::cerr << "v2 " << v2 << " tot " << number_of_storage_values << "\n";
            for(auto v1 = 0U; v1 < v2; ++v1)

               if(!interferenceGraph(v1, v2) && HLS->storage_value_information->are_storage_value_compatible(v1, v2))
               {
                  const auto edge_weight = HLS->storage_value_information->get_compatibility_weight(v1, v2);
                  /// we consider only valuable sharing between registers
                  if(edge_weight > 1)
                  {
                     register_clique->add_edge(v1, v2, edge_weight);
                  }
               }
         }
      }
      if(parameters->getOption<bool>(OPT_print_dot))
      {
         register_clique->writeDot(FB->GetDotPath() / "HLS_RegisterBinding.dot");
      }
      /// performing clique covering
      register_filter_clique<unsigned> fc(HLS);
      register_clique->exec(fc);
      /// vertex to clique map
      CustomUnorderedMap<unsigned, unsigned int> v2c;
      /// retrieve the solution
      num_registers = static_cast<unsigned int>(register_clique->num_vertices());
      for(unsigned int i = 0; i < num_registers; ++i)
      {
         for(const auto v : register_clique->get_clique(i))
         {
            v2c[v] = i;
         }
      }
      /// finalize
      for(auto vi = 0U; vi < number_of_storage_values; ++vi)
      {
         HLS->Rreg->bind(vi, v2c.at(vi));
      }
   }
   HLS->Rreg->set_used_regs(num_registers);
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->Register binding information for function " + FB->CGetBehavioralHelper()->GetFunctionName() +
                      ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  std::string("---Register allocation algorithm obtains ") +
                      (num_registers == register_lower_bound ? "an optimal" : "a sub-optimal") +
                      " result: " + STR(num_registers) + " registers" +
                      (num_registers == register_lower_bound ? "" : ("(LB:" + STR(register_lower_bound) + ")")));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      THROW_ASSERT(HLS->Rreg, "unexpected condition");
      HLS->Rreg->print();
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
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
