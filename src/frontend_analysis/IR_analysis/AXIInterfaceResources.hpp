/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file AXIInterfaceResources.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef AXI_INTERFACE_RESOURCES_HPP
#define AXI_INTERFACE_RESOURCES_HPP

#include "AXIBurstAnalysis.hpp"
#include "technology_manager.hpp"

#include <cstdint>
#include <set>
#include <string>

namespace InterfaceInferInternal
{
   struct AXIResourceRequest
   {
      std::string argument_id;
      std::string interface_name;
      std::uint64_t bitwidth;
   };

   void RegisterFrozenDuplexResources(
       const technology_managerRef& TechMan, const ParameterConstRef& parameters,
       const std::map<std::string, FrozenDuplexContribution>& frozen_by_bundle,
       std::map<std::pair<std::string, std::string>, std::pair<unsigned int, unsigned int>>& global_constraints);

   void CreateMAXIResource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                           const AXIResourceRequest& request, FunctionArchitectureRef func_arch, bool unused_port,
                           unsigned int root_id, const application_managerRef& AppM,
                           const ParameterConstRef& parameters, int debug_level);

   std::string getHDLGeneratorNameToken(const std::string& interface_name);
} // namespace InterfaceInferInternal

#endif // AXI_INTERFACE_RESOURCES_HPP
