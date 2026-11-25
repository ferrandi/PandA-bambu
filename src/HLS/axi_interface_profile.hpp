/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file axi_interface_profile.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef HLS_AXI_INTERFACE_PROFILE_HPP
#define HLS_AXI_INTERFACE_PROFILE_HPP

#include "function_architecture.hpp"
#include "utility/axi_interface_profile.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

REF_FORWARD_DECL(ModuleArchitecture);

using AXIReadBurstProfile = panda::utility::axi_read_profile;
using AXIWriteBurstProfile = panda::utility::axi_write_profile;

enum class AXIBurstLengthDefaultReason
{
   explicit_value,
   default_value,
   outstanding_hint,
   fifo_hint,
   outstanding_and_fifo_hints
};

struct AXIBurstProfileOrigins
{
   bool read_burst_default = false;
   bool read_outstanding_default = false;
   bool read_fifo_default = false;
   bool write_burst_default = false;
   bool write_outstanding_default = false;
   bool write_fifo_default = false;
   AXIBurstLengthDefaultReason read_burst_reason = AXIBurstLengthDefaultReason::explicit_value;
   AXIBurstLengthDefaultReason write_burst_reason = AXIBurstLengthDefaultReason::explicit_value;
};

struct AXIBurstProfileResolution
{
   AXIReadBurstProfile read;
   AXIWriteBurstProfile write;
   AXIBurstProfileOrigins origins;
   bool enabled = false;
   bool materialized = false;
};

using AXIBurstProfileResolutions = std::map<std::pair<std::string, std::string>, AXIBurstProfileResolution>;

AXIReadBurstProfile ParseAXIReadBurstProfile(const FunctionArchitecture::iface_attrs& iface_attrs,
                                             const std::string& bundle_name, AXIBurstProfileOrigins* origins = nullptr);
AXIWriteBurstProfile ParseAXIWriteBurstProfile(const FunctionArchitecture::iface_attrs& iface_attrs,
                                               const std::string& bundle_name,
                                               AXIBurstProfileOrigins* origins = nullptr);
std::uint32_t ParseAXIInterfaceLatency(const FunctionArchitecture::iface_attrs& iface_attrs,
                                       const std::string& bundle_name);

AXIBurstProfileResolutions MaterializeAXIBurstProfileDefaults(const ModuleArchitectureRef& module_arch,
                                                              bool burst_feature_enabled);

#endif // HLS_AXI_INTERFACE_PROFILE_HPP
