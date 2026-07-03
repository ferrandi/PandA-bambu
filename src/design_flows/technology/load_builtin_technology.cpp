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
 * @file load_builtin_technology.hpp
 * @brief This class load builtin components in technology manager
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "load_builtin_technology.hpp"

#include "fileIO.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

LoadBuiltinTechnology::LoadBuiltinTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                                             const DesignFlowManager& _design_flow_manager,
                                             const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY,
                         _parameters)
{
}

CustomUnorderedSet<TechnologyFlowStep_Type>
LoadBuiltinTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<TechnologyFlowStep_Type>();
}

DesignFlowStep_Status LoadBuiltinTechnology::Exec()
{
   std::string fu_name;
   structural_objectRef top;
   structural_managerRef CM;
   structural_type_descriptorRef b_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   structural_type_descriptorRef bvec_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
   structural_type_descriptorRef module_type;
   std::string NP_parameters;
   std::string Library;

   Library = LIBRARY_STD;

   // AND
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = AND_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=[*]");
   TM->add_resource(Library, fu_name, CM, true);

   // NAND
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = NAND_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=![*]");
   TM->add_resource(Library, fu_name, CM, true);

   // OR
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = OR_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=[+]");
   TM->add_resource(Library, fu_name, CM, true);

   // NOR
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = NOR_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=![+]");
   TM->add_resource(Library, fu_name, CM, true);

   // XOR
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = XOR_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=[^]");
   TM->add_resource(Library, fu_name, CM, true);

   // XNOR
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = XNOR_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("in", port_o::IN, port_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=![^]");
   TM->add_resource(Library, fu_name, CM, true);

   // NOT
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = NOT_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("in1", port_o::IN, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=!in1");
   TM->add_resource(Library, fu_name, CM, true);

   // DFF
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = DFF_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("in1", port_o::IN, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   TM->add_resource(Library, fu_name, CM, true);

   // ASSIGN
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = ASSIGN_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("in1", port_o::IN, top, bvec_type);
   CM->add_port("out1", port_o::OUT, top, bvec_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   CM->add_NP_functionality(top, NP_functionality::EQUATION, "out1=in1");
   TM->add_resource(Library, fu_name, CM, true);

   return DesignFlowStep_Status::SUCCESS;
}
