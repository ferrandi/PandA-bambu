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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
/// Header include
#include "generate_fu_list.hpp"

///. include
#include "Parameter.hpp"

/// circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// STD include
#include <string>

/// STL include
#include <vector>

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_VERY_PEDANTIC
#include "library_manager.hpp"
#include "technology_node.hpp"

/// utility include
#include "string_manipulation.hpp"

GenerateFuList::GenerateFuList(const target_managerRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters), ToDataFileStep(_design_flow_manager, ToDataFileStep_Type::GENERATE_FU_LIST, _parameters), FunctionalUnitStep(_target, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   if(parameters->getOption<std::string>(OPT_component_name) != "all")
   {
      std::string to_be_splitted(parameters->getOption<std::string>(OPT_component_name));
      const auto splitted = SplitString(to_be_splitted, ",");
      for(const auto& component_to_be_characterized : splitted)
      {
         components_to_be_characterized.insert(component_to_be_characterized);
      }
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
         if(components_to_be_characterized.empty() or components_to_be_characterized.find(component) != components_to_be_characterized.end())
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

void GenerateFuList::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         {
            const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
            const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         {
            const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step =
                technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::WRITE_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
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

const std::string GenerateFuList::GetSignature() const
{
   return ToDataFileStep::GetSignature();
}

const std::string GenerateFuList::GetName() const
{
   return ToDataFileStep::GetName();
}

const DesignFlowStepFactoryConstRef GenerateFuList::CGetDesignFlowStepFactory() const
{
   return ToDataFileStep::CGetDesignFlowStepFactory();
}

void GenerateFuList::AnalyzeCell(functional_unit* fu, const unsigned int, const std::vector<std::string>&, const size_t, const std::vector<std::string>&, const size_t, const unsigned int constPort, const bool is_commutative, size_t)
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
         cells.insert(current_list);
      current_list = component + "-" + fu->get_name();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + fu->get_name());
}
