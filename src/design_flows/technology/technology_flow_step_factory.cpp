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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file technology_flow_step_factory.cpp
 * @brief Factory for technology flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "technology_flow_step_factory.hpp"

#include "exceptions.hpp"
#include "load_builtin_technology.hpp"
#include "load_default_technology.hpp"
#include "load_device_technology.hpp"
#include "load_file_technology.hpp"
#include "load_technology.hpp"
#include "write_technology.hpp"

TechnologyFlowStepFactory::TechnologyFlowStepFactory(const technology_managerRef _TM, const generic_deviceRef _target,
                                                     const DesignFlowManager& _design_flow_manager,
                                                     const ParameterConstRef _parameters)
    : DesignFlowStepFactory(DesignFlowStep::TECHNOLOGY, _design_flow_manager, _parameters), TM(_TM), target(_target)
{
}

DesignFlowStepRef
TechnologyFlowStepFactory::CreateTechnologyFlowStep(const TechnologyFlowStep_Type technology_flow_step_type) const
{
   switch(technology_flow_step_type)
   {
#if HAVE_CIRCUIT_BUILT
      case TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadBuiltinTechnology(TM, target, design_flow_manager, parameters));
      }
#endif
      case TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadDefaultTechnology(TM, target, design_flow_manager, parameters));
      }
      case TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadDeviceTechnology(TM, target, design_flow_manager, parameters));
      }
      case TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadFileTechnology(TM, target, design_flow_manager, parameters));
      }
      case TechnologyFlowStep_Type::LOAD_TECHNOLOGY:
      {
         return DesignFlowStepRef(new LoadTechnology(TM, target, design_flow_manager, parameters));
      }
      case TechnologyFlowStep_Type::WRITE_TECHNOLOGY:
      {
         return DesignFlowStepRef(new WriteTechnology(TM, target, design_flow_manager, parameters));
      }
      default:
         break;
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
