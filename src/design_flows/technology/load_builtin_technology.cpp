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
 * @file load_builtin_technology.hpp
 * @brief This class load builtin components in technology manager
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_KOALA_BUILT.hpp"

/// Header include
#include "load_builtin_technology.hpp"

/// circuit include
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// parser/polixml include
#include "xml_dom_parser.hpp"

/// polixml include
#include "xml_document.hpp"

/// technology includes
#include "technology_manager.hpp"
#include "technology_node.hpp"

/// utility include
#include "fileIO.hpp"

LoadBuiltinTechnology::LoadBuiltinTechnology(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY, _parameters)
{
}

LoadBuiltinTechnology::~LoadBuiltinTechnology() = default;

const CustomUnorderedSet<TechnologyFlowStep_Type> LoadBuiltinTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<TechnologyFlowStep_Type>();
}

DesignFlowStep_Status LoadBuiltinTechnology::Exec()
{
   std::string fu_name;
   structural_objectRef top;
   structural_managerRef CM;
   structural_type_descriptorRef b_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   structural_type_descriptorRef module_type;
   std::string NP_parameters;
   std::string Library;

#if HAVE_KOALA_BUILT
   // LUT
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = LUT_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port_vector("I", port_o::IN, port_vector_o::PARAMETRIC_PORT, top, b_type);
   CM->add_port("O", port_o::OUT, top, b_type);
   NP_parameters = fu_name + " in init";
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   TM->add_resource(FPGA_LIBRARY, fu_name, CM, true);

   // IBUF
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = IBUF_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("I", port_o::IN, top, b_type);
   CM->add_port("O", port_o::OUT, top, b_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   TM->add_resource(FPGA_LIBRARY, fu_name, CM, true);

   // OBUF
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = OBUF_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("I", port_o::IN, top, b_type);
   CM->add_port("O", port_o::OUT, top, b_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   TM->add_resource(FPGA_LIBRARY, fu_name, CM, true);
#endif

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

   // BUFF
   CM = structural_managerRef(new structural_manager(parameters));
   fu_name = BUFF_GATE_STD;
   module_type = structural_type_descriptorRef(new structural_type_descriptor(fu_name));
   CM->set_top_info(fu_name, module_type);
   top = CM->get_circ();
   CM->add_port("in1", port_o::IN, top, b_type);
   CM->add_port("out1", port_o::OUT, top, b_type);
   NP_parameters = fu_name;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   TM->add_resource(Library, fu_name, CM, true);
   return DesignFlowStep_Status::SUCCESS;
}
