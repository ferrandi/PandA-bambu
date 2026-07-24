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
 * @file load_device_technology.cpp
 * @brief This class loads device dependent technology information
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "load_device_technology.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"

#include "config_PANDA_LIB_INSTALLDIR.hpp"

LoadDeviceTechnology::LoadDeviceTechnology(const technology_managerRef _TM, const generic_deviceRef _target,
                                           const DesignFlowManager& _design_flow_manager,
                                           const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY,
                         _parameters)
{
}

CustomUnorderedSet<TechnologyFlowStep_Type>
LoadDeviceTechnology::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<TechnologyFlowStep_Type> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_CIRCUIT_BUILT
         relationships.insert(TechnologyFlowStep_Type::LOAD_BUILTIN_TECHNOLOGY);
#endif
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY);
         relationships.insert(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status LoadDeviceTechnology::Exec()
{
   /// load specific device information

   if(output_level >= OUTPUT_LEVEL_MINIMUM)
   {
      std::cout << "Available devices:\n";
      for(const auto& device_data :
          std::filesystem::directory_iterator(relocate_install_path(PANDA_LIB_INSTALLDIR "/libtech/targets")))
      {
         std::cout << " - " + device_data.path().stem().string() << "\n";
      }
   }

   target->load_devices();
   return DesignFlowStep_Status::SUCCESS;
}
