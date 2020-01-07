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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file omp_allocation.cpp
 * @brief This package is used by all HLS packages to manage resource constraints and characteristics.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "omp_allocation.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// circuit include
#include "structural_manager.hpp"

/// HLS includes
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// technology/physical_library/models includes
#include "area_model.hpp"
#include "time_model.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

/// utility include
#include "utility.hpp"

OmpAllocation::OmpAllocation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : allocation(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::OMP_ALLOCATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

OmpAllocation::~OmpAllocation()
{
}

void OmpAllocation::IntegrateTechnologyLibraries()
{
   allocation::IntegrateTechnologyLibraries();
   const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef op_graph = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
   VertexIterator operation, operation_end;
   for(boost::tie(operation, operation_end) = boost::vertices(*op_graph); operation != operation_end; operation++)
   {
      const auto current_op = tree_helper::normalized_ID(op_graph->CGetOpNodeInfo(*operation)->GetOperation());
      if(current_op == "panda_pthread_mutex")
      {
         const auto tn = TM->get_fu("panda_pthread_mutex", OPENMP_LIBRARY);
         if(not tn)
         {
            AddPandaPthreadMutex();
         }
      }
   }
}

void OmpAllocation::AddPandaPthreadMutex()
{
   const auto TreeM = HLSMgr->get_tree_manager();
   structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
   std::string fu_name = "panda_pthread_mutex";
   std::string op_name = "panda_pthread_mutex";
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   const auto top = CM->get_circ();
   /// add description and license
   GetPointer<module>(top)->set_description("Implementation of panda_pthread_mutex");
   GetPointer<module>(top)->set_copyright("Copyright (C) 2012-2020 Politecnico di Milano");
   GetPointer<module>(top)->set_authors("Marco Lattuada marco.lattuada@polimi.it");
   GetPointer<module>(top)->set_license("PANDA_GPLv3");
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, "panda_pthread_mutex");
   CM->add_NP_functionality(top, NP_functionality::VERILOG_PROVIDED, "---");
   TM->add_resource(OPENMP_LIBRARY, fu_name, CM);
   TM->add_operation(OPENMP_LIBRARY, fu_name, op_name);
   const auto tn = TM->get_fu(fu_name, OPENMP_LIBRARY);
   auto* fu = GetPointer<functional_unit>(tn);
   auto op = GetPointer<operation>(fu->get_operation(op_name));
   op->time_m = time_model::create_model(TargetDevice_Type::FPGA, parameters);
   fu->area_m = area_model::create_model(TargetDevice_Type::FPGA, parameters);
   structural_type_descriptorRef boolean_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   CM->add_port(START_PORT_NAME, port_o::IN, top, boolean_type);
   const auto behavioral_helper = HLSMgr->CGetFunctionBehavior(TreeM->function_index("panda_pthread_mutex"))->CGetBehavioralHelper();
   size_t parameter_index = 0;
   const auto function_parameters = behavioral_helper->get_parameters();
   THROW_ASSERT(function_parameters.size() == 2, STR(function_parameters.size()));
   for(const auto function_parameter : function_parameters)
   {
      CM->add_port(parameter_index == 0 ? "mutex" : "locking", port_o::IN, top, structural_type_descriptorRef(new structural_type_descriptor(function_parameter, behavioral_helper)));
      parameter_index++;
   }
   parameter_index = 0;
   for(const auto function_parameter : function_parameters)
   {
      CM->add_port(parameter_index == 0 ? "out_mutex" : "out_locking", port_o::OUT, top, structural_type_descriptorRef(new structural_type_descriptor(function_parameter, behavioral_helper)));
      parameter_index++;
   }
   CM->add_port(DONE_PORT_NAME, port_o::OUT, top, boolean_type);
}
