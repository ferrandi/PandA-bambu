/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file technology_flow_step.cpp
 * @brief Base class for technology flow steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "technology_flow_step.hpp"

#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step_factory.hpp"

TechnologyFlowStep::TechnologyFlowStep(const technology_managerRef _TM, const generic_deviceRef _target,
                                       const DesignFlowManager& _design_flow_manager,
                                       const TechnologyFlowStep_Type _technology_flow_step_type,
                                       const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_technology_flow_step_type), _design_flow_manager, _parameters),
      technology_flow_step_type(_technology_flow_step_type),
      TM(_TM),
      target(_target)
{
}

DesignFlowStep::signature_t
TechnologyFlowStep::ComputeSignature(const TechnologyFlowStep_Type technology_flow_step_type)
{
   return DesignFlowStep::ComputeSignature(TECHNOLOGY, static_cast<unsigned short>(technology_flow_step_type), 0);
}

static std::string EnumToName(const TechnologyFlowStep_Type technology_flow_step_type)
{
   switch(technology_flow_step_type)
   {
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

std::string TechnologyFlowStep::GetName() const
{
   return "Technology::" + EnumToName(technology_flow_step_type);
}

void TechnologyFlowStep::ComputeRelationships(DesignFlowStepSet& steps,
                                              const DesignFlowStep::RelationshipType relationship_type)
{
   const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
   const auto step_factory = GetPointer<const TechnologyFlowStepFactory>(CGetDesignFlowStepFactory());
   const auto step_types = ComputeTechnologyRelationships(relationship_type);
   for(const auto& step_type : step_types)
   {
      auto technology_flow_step = design_flow_manager.GetDesignFlowStep(ComputeSignature(step_type));
      const auto design_flow_step = technology_flow_step != DesignFlowGraph::null_vertex() ?
                                        design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                                        step_factory->CreateTechnologyFlowStep(step_type);
      steps.insert(design_flow_step);
   }
}

DesignFlowStepFactoryConstRef TechnologyFlowStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY);
}

bool TechnologyFlowStep::HasToBeExecuted() const
{
   return true;
}
