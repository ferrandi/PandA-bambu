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
 * @file technology_flow_step.cpp
 * @brief Base class for technology flow steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "technology_flow_step.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology include
#include "string_manipulation.hpp"
#include "technology_flow_step_factory.hpp"

const std::string TechnologyFlowStep::EnumToName(const TechnologyFlowStep_Type technology_flow_step_type)
{
   switch(technology_flow_step_type)
   {
      case TechnologyFlowStep_Type::FIX_CHARACTERIZATION:
         return "FixCharacterization";
#if HAVE_CIRCUIT_BUILT
      case TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY:
         return "LoadBuiltinTechnology";
#endif
      case TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY:
         return "LoadDefaultTechnology";
      case TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY:
         return "LoadDeviceTechnology";
      case TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY:
         return "LoadFileTechnology";
      case TechnologyFlowStep_Type::LOAD_TECHNOLOGY:
         return "LoadTechnology";
      case TechnologyFlowStep_Type::WRITE_TECHNOLOGY:
         return "WriteTechnology";
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}

const std::string TechnologyFlowStep::ComputeSignature(const TechnologyFlowStep_Type technology_flow_step_type)
{
   return "Technology::" + STR(static_cast<int>(technology_flow_step_type));
}

const std::string TechnologyFlowStep::GetSignature() const
{
   return ComputeSignature(technology_flow_step_type);
}

const std::string TechnologyFlowStep::GetName() const
{
   return "Technology::" + EnumToName(technology_flow_step_type);
}

void TechnologyFlowStep::ComputeRelationships(DesignFlowStepSet& steps, const DesignFlowStep::RelationshipType relationship_type)
{
   const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const auto step_factory = GetPointer<const TechnologyFlowStepFactory>(CGetDesignFlowStepFactory());
   const auto step_types = ComputeTechnologyRelationships(relationship_type);
   for(const auto& step_type : step_types)
   {
      vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(ComputeSignature(step_type));
      const DesignFlowStepRef design_flow_step = technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : step_factory->CreateTechnologyFlowStep(step_type);
      steps.insert(design_flow_step);
   }
}

const DesignFlowStepFactoryConstRef TechnologyFlowStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology");
}

TechnologyFlowStep::TechnologyFlowStep(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const TechnologyFlowStep_Type _technology_flow_step_type, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters), technology_flow_step_type(_technology_flow_step_type), TM(_TM), target(_target)
{
}

bool TechnologyFlowStep::HasToBeExecuted() const
{
   return true;
}
