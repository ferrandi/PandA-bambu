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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file generate_fu_list.cpp
 * @brief Class for generating the list of functional untis to be characterized
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "generate_fu_list.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "library_manager.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <string>
#include <vector>

GenerateFuList::GenerateFuList(const generic_deviceRef _device, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(ToDataFileStep_Type::GENERATE_FU_LIST), _design_flow_manager, _parameters),
      ToDataFileStep(_design_flow_manager, ToDataFileStep_Type::GENERATE_FU_LIST, _parameters),
      FunctionalUnitStep(_device)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   if(parameters->getOption<std::string>(OPT_component_name) != "all")
   {
      components_to_be_characterized = string_to_container<CustomOrderedSet<std::string>>(
          parameters->getOption<std::string>(OPT_component_name), ",");
   }
}

DesignFlowStep_Status GenerateFuList::Exec()
{
   const auto libraries = TM->get_library_list();
   for(const auto& library : libraries)
   {
      const auto LM = TM->get_library_manager(library);
      const auto fus = LM->get_library_fu();
      for(const auto& fu : fus)
      {
         component = fu.first;
         if(components_to_be_characterized.empty() or
            components_to_be_characterized.find(component) != components_to_be_characterized.end())
         {
            AnalyzeFu(fu.second);
         }
      }
   }
   if(current_list != "")
   {
      cells.insert(current_list);
   }
   std::ofstream file_out(parameters->getOption<std::string>(OPT_output_file).c_str(), std::ios::out);
   for(const auto& cell : cells)
   {
      file_out << cell << std::endl;
   }
   file_out.close();
   return DesignFlowStep_Status::SUCCESS;
}

void GenerateFuList::ComputeRelationships(DesignFlowStepSet& relationship,
                                          const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto DFM = design_flow_manager.lock();
         const auto design_flow_graph = DFM->CGetDesignFlowGraph();
         const auto technology_flow_step_factory =
             GetPointer<const TechnologyFlowStepFactory>(DFM->CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         {
            const auto technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
            const auto technology_flow_step = DFM->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                    technology_flow_step_factory->CreateTechnologyFlowStep(
                        TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         {
            const auto technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            const auto technology_flow_step = DFM->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                    technology_flow_step_factory->CreateTechnologyFlowStep(
                        TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            const auto technology_flow_signature =
                TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            const auto technology_flow_step = DFM->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step != DesignFlowGraph::null_vertex() ?
                    design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                    technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
}

bool GenerateFuList::HasToBeExecuted() const
{
   return ToDataFileStep::HasToBeExecuted();
}

DesignFlowStepFactoryConstRef GenerateFuList::CGetDesignFlowStepFactory() const
{
   return ToDataFileStep::CGetDesignFlowStepFactory();
}

void GenerateFuList::AnalyzeCell(functional_unit* fu, const unsigned int, const std::vector<std::string>&, const size_t,
                                 const std::vector<std::string>&, const size_t, const unsigned int constPort,
                                 const bool is_commutative, size_t)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + fu->get_name());
   const structural_objectRef obj = fu->CM->get_circ();
   unsigned int n_ports = GetPointer<module>(obj)->get_in_port_size();
   if(constPort < n_ports && is_commutative && constPort > has_first_synthesis_id)
   {
      current_list = current_list + "," + component + "-" + fu->get_name();
   }
   else
   {
      has_first_synthesis_id = constPort;
      if(current_list != "")
      {
         cells.insert(current_list);
      }
      current_list = component + "-" + fu->get_name();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + fu->get_name());
}
