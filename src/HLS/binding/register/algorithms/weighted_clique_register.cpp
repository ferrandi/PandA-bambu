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
 * @file weighted_clique_register.cpp
 * @brief Weighted clique covering register allocation procedure
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "weighted_clique_register.hpp"

#include <boost/filesystem/operations.hpp>

#include "check_clique.hpp"
#include "clique_covering.hpp"
#include "filter_clique.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "reg_binding.hpp"

///. include
#include "Parameter.hpp"

/// HLS/binding/module include
#include "cdfc_module_binding.hpp"

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "cpu_time.hpp"

WeightedCliqueRegisterBindingSpecialization::WeightedCliqueRegisterBindingSpecialization(const CliqueCovering_Algorithm _clique_covering_algorithm) : clique_covering_algorithm(_clique_covering_algorithm)
{
}

const std::string WeightedCliqueRegisterBindingSpecialization::GetKindText() const
{
   return CliqueCovering_AlgorithmToString(clique_covering_algorithm);
}

const std::string WeightedCliqueRegisterBindingSpecialization::GetSignature() const
{
   return STR(static_cast<unsigned int>(clique_covering_algorithm));
}

weighted_clique_register::weighted_clique_register(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                                   const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : compatibility_based_register(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING,
                                   _hls_flow_step_specialization ? _hls_flow_step_specialization :
                                                                   HLSFlowStepSpecializationConstRef(new WeightedCliqueRegisterBindingSpecialization(_parameters->getOption<CliqueCovering_Algorithm>(OPT_weighted_clique_register_algorithm))))
{
}

weighted_clique_register::~weighted_clique_register() = default;

void weighted_clique_register::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rreg = reg_binding::create_reg_binding(HLS, HLSMgr);
}

DesignFlowStep_Status weighted_clique_register::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   const CliqueCovering_Algorithm clique_covering_algorithm = GetPointer<const WeightedCliqueRegisterBindingSpecialization>(hls_flow_step_specialization)->clique_covering_algorithm;
   refcount<clique_covering<CG_vertex_descriptor>> register_clique = clique_covering<CG_vertex_descriptor>::create_solver(clique_covering_algorithm);
   create_compatibility_graph();

   std::vector<CG_vertex_descriptor>::const_iterator v_it_end = verts.end();
   unsigned int vertex_index = 0;
   unsigned int num_registers = 0;
   for(std::vector<CG_vertex_descriptor>::const_iterator v_it = verts.begin(); v_it != v_it_end; ++v_it, ++vertex_index)
      register_clique->add_vertex(*v_it, STR(vertex_index));
   if(vertex_index > 0)
   {
      HLS->Rreg->set_used_regs(num_registers);
      boost::graph_traits<compatibility_graph>::edge_iterator cg_ei, cg_ei_end;
      for(boost::tie(cg_ei, cg_ei_end) = boost::edges(CG); cg_ei != cg_ei_end; ++cg_ei)
      {
         CG_vertex_descriptor src = boost::source(*cg_ei, CG);
         CG_vertex_descriptor tgt = boost::target(*cg_ei, CG);
         register_clique->add_edge(src, tgt, CG[*cg_ei].weight);
      }
      if(parameters->getOption<bool>(OPT_print_dot))
      {
         const auto functionName = FB->CGetBehavioralHelper()->get_function_name();
         const auto output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "/" + functionName + "/";
         if(!boost::filesystem::exists(output_directory))
            boost::filesystem::create_directories(output_directory);
         const auto file_name = output_directory + "HLS_RegisterBinding.dot";
         register_clique->writeDot(file_name);
      }
      /// performing clique covering
      no_check_clique<CG_vertex_descriptor> cq;
      register_clique->exec(no_filter_clique<CG_vertex_descriptor>(), cq);
      /// vertex to clique map
      std::map<CG_vertex_descriptor, unsigned int> v2c;
      /// retrieve the solution
      num_registers = static_cast<unsigned int>(register_clique->num_vertices());
      for(unsigned int i = 0; i < num_registers; ++i)
      {
         CustomOrderedSet<CG_vertex_descriptor> clique = register_clique->get_clique(i);
         auto v_end = clique.end();
         for(auto v = clique.begin(); v != v_end; ++v)
         {
            v2c[*v] = i;
         }
      }
      /// finalize
      HLS->Rreg = reg_binding::create_reg_binding(HLS, HLSMgr);
      const std::list<vertex>& support = HLS->Rliv->get_support();

      const std::list<vertex>::const_iterator vEnd = support.end();
      for(auto vIt = support.begin(); vIt != vEnd; ++vIt)
      {
         const CustomOrderedSet<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
         auto k_end = live.end();
         for(auto k = live.begin(); k != k_end; ++k)
         {
            unsigned int storage_value_index = HLS->storage_value_information->get_storage_value_index(*vIt, *k);
            HLS->Rreg->bind(storage_value_index, v2c[verts[storage_value_index]]);
         }
      }
   }
   else
   {
      HLS->Rreg = reg_binding::create_reg_binding(HLS, HLSMgr);
      num_registers = 0;
   }
   HLS->Rreg->set_used_regs(num_registers);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Register binding information for function " + FB->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  std::string("---Register allocation algorithm obtains ") + (num_registers == register_lower_bound ? "an optimal" : "a sub-optimal") + " result: " + STR(num_registers) + " registers" +
                      (num_registers == register_lower_bound ? "" : ("(LB:" + STR(register_lower_bound) + ")")));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      HLS->Rreg->print();
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform register binding: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}
