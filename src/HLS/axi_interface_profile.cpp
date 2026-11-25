/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file axi_interface_profile.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "axi_interface_profile.hpp"

#include "exceptions.hpp"
#include "hls_manager.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cstdint>

AXIReadBurstProfile ParseAXIReadBurstProfile(const FunctionArchitecture::iface_attrs& iface_attrs,
                                             const std::string& bundle_name, AXIBurstProfileOrigins* origins)
{
   const auto parse_bounded_decimal = [&](const FunctionArchitecture::iface_attr attr, const char* attr_name,
                                          const std::uint32_t default_value, const std::uint32_t maximum) {
      const auto it = iface_attrs.find(attr);
      if(it == iface_attrs.end())
      {
         return default_value;
      }
      const auto& text = it->second;
      const auto fail = [&]() -> void {
         THROW_ERROR_USAGE("Invalid AXI interface attribute '" + std::string(attr_name) + "' for bundle '" +
                           bundle_name + "': expected an ASCII decimal integer in [1," + STR(maximum) + "].");
      };
      std::uint32_t value = 0;
      if(panda::utility::parse_axi_profile_decimal(text, 1U, maximum, value) !=
         panda::utility::bounded_decimal_result::valid)
      {
         fail();
      }
      return static_cast<unsigned int>(value);
   };

   AXIReadBurstProfile profile;
   const auto max_burst = iface_attrs.find(FunctionArchitecture::iface_max_read_burst_length);
   const bool outstanding_hint =
       iface_attrs.find(FunctionArchitecture::iface_num_read_outstanding) != iface_attrs.end();
   const bool fifo_hint = iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth) != iface_attrs.end();
   const bool burst_default = max_burst == iface_attrs.end();
   if(origins)
   {
      origins->read_burst_default = burst_default;
      origins->read_outstanding_default = !outstanding_hint;
      origins->read_fifo_default = !fifo_hint;
      origins->read_burst_reason = !burst_default ? AXIBurstLengthDefaultReason::explicit_value :
                                   outstanding_hint && fifo_hint ?
                                                      AXIBurstLengthDefaultReason::outstanding_and_fifo_hints :
                                   outstanding_hint ? AXIBurstLengthDefaultReason::outstanding_hint :
                                   fifo_hint        ? AXIBurstLengthDefaultReason::fifo_hint :
                                                      AXIBurstLengthDefaultReason::default_value;
   }
   profile.max_outstanding =
       parse_bounded_decimal(FunctionArchitecture::iface_num_read_outstanding, "num_read_outstanding",
                             panda::utility::axi_outstanding_default, panda::utility::axi_outstanding_max);
   profile.max_burst_length =
       max_burst == iface_attrs.end() ?
           (outstanding_hint || fifo_hint ? panda::utility::axi_burst_length_min :
                                            panda::utility::axi_burst_length_default) :
           parse_bounded_decimal(FunctionArchitecture::iface_max_read_burst_length, "max_read_burst_length",
                                 panda::utility::axi_burst_length_default, panda::utility::axi_burst_length_max);
   if(iface_attrs.find(FunctionArchitecture::iface_read_fifo_depth) == iface_attrs.end())
   {
      // No explicit depth: size the FIFO so the credit never shortens a burst.
      // A burst keeps its credit until the kernel consumes the words, not when
      // they arrive, so issuing and draining only overlap if there is room for
      // a second full burst. Below this the engine issues shorter bursts and
      // pays another bus round trip: measured at 64-cycle memory latency,
      // depth B costs 6% cycles over depth 2B for one outstanding burst.
      profile.fifo_depth =
          panda::utility::derive_axi_read_fifo_depth(profile.max_burst_length, profile.max_outstanding);
   }
   else
   {
      profile.fifo_depth =
          parse_bounded_decimal(FunctionArchitecture::iface_read_fifo_depth, "read_fifo_depth",
                                panda::utility::axi_read_fifo_depth_default, panda::utility::axi_fifo_depth_max);
   }
   if(!panda::utility::is_axi_fifo_depth_valid(profile.fifo_depth))
   {
      THROW_ERROR_USAGE("Invalid AXI interface attribute 'read_fifo_depth' for bundle '" + bundle_name +
                        "': expected a power-of-two decimal integer in [1,4096] beats.");
   }
   return profile;
}

AXIWriteBurstProfile ParseAXIWriteBurstProfile(const FunctionArchitecture::iface_attrs& iface_attrs,
                                               const std::string& bundle_name, AXIBurstProfileOrigins* origins)
{
   const auto fail = [&](const char* attr_name, const std::string& expectation) -> void {
      THROW_ERROR_USAGE("Invalid AXI interface attribute '" + std::string(attr_name) + "' for bundle '" + bundle_name +
                        "': " + expectation + ".");
   };
   const auto parse_bounded_decimal = [&](const FunctionArchitecture::iface_attr attr, const char* attr_name,
                                          const unsigned int minimum, const unsigned int maximum) {
      const auto it = iface_attrs.find(attr);
      if(it == iface_attrs.end())
      {
         fail(attr_name, "attribute is required");
      }
      const auto& text = it->second;
      std::uint32_t value = 0;
      if(panda::utility::parse_axi_profile_decimal(text, minimum, maximum, value) !=
         panda::utility::bounded_decimal_result::valid)
      {
         fail(attr_name, "expected an ASCII decimal integer in [" + STR(minimum) + "," + STR(maximum) + "]");
      }
      return static_cast<unsigned int>(value);
   };

   AXIWriteBurstProfile profile;
   const auto max_burst_length = iface_attrs.find(FunctionArchitecture::iface_max_write_burst_length);
   const auto outstanding = iface_attrs.find(FunctionArchitecture::iface_write_burst_outstanding);
   const auto fifo_depth = iface_attrs.find(FunctionArchitecture::iface_write_fifo_depth);
   const bool outstanding_hint = outstanding != iface_attrs.end();
   const bool fifo_hint = fifo_depth != iface_attrs.end();
   const bool burst_default = max_burst_length == iface_attrs.end();
   if(origins)
   {
      origins->write_burst_default = burst_default;
      origins->write_outstanding_default = !outstanding_hint;
      origins->write_fifo_default = !fifo_hint;
      origins->write_burst_reason = !burst_default ? AXIBurstLengthDefaultReason::explicit_value :
                                    outstanding_hint && fifo_hint ?
                                                       AXIBurstLengthDefaultReason::outstanding_and_fifo_hints :
                                    outstanding_hint ? AXIBurstLengthDefaultReason::outstanding_hint :
                                    fifo_hint        ? AXIBurstLengthDefaultReason::fifo_hint :
                                                       AXIBurstLengthDefaultReason::default_value;
   }
   profile.max_burst_length =
       max_burst_length == iface_attrs.end() ?
           (outstanding_hint || fifo_hint ? panda::utility::axi_burst_length_min :
                                            panda::utility::axi_burst_length_default) :
           parse_bounded_decimal(FunctionArchitecture::iface_max_write_burst_length, "max_write_burst_length",
                                 panda::utility::axi_burst_length_min, panda::utility::axi_burst_length_max);

   profile.max_outstanding =
       outstanding == iface_attrs.end() ?
           panda::utility::axi_outstanding_default :
           parse_bounded_decimal(FunctionArchitecture::iface_write_burst_outstanding, "write_burst_outstanding",
                                 panda::utility::axi_outstanding_min, panda::utility::axi_outstanding_max);

   if(fifo_depth == iface_attrs.end())
   {
      profile.fifo_depth = panda::utility::derive_axi_write_fifo_depth(profile.max_burst_length);
   }
   else
   {
      profile.fifo_depth =
          parse_bounded_decimal(FunctionArchitecture::iface_write_fifo_depth, "write_fifo_depth",
                                panda::utility::axi_fifo_depth_min, panda::utility::axi_fifo_depth_max);
   }
   if(!panda::utility::is_axi_fifo_depth_valid(profile.fifo_depth))
   {
      fail("write_fifo_depth", "expected a power-of-two decimal integer in [1,4096] beats");
   }
   if(profile.fifo_depth < profile.max_burst_length)
   {
      fail("write_fifo_depth", "must be at least max_write_burst_length");
   }
   return profile;
}

AXIBurstProfileResolutions MaterializeAXIBurstProfileDefaults(const ModuleArchitectureRef& module_arch,
                                                              const bool burst_feature_enabled)
{
   AXIBurstProfileResolutions resolutions;
   for(auto architecture_it = module_arch->begin(); architecture_it != module_arch->end(); ++architecture_it)
   {
      const auto& function_symbol = architecture_it->first;
      const auto& architecture = architecture_it->second;
      for(auto& iface : architecture->ifaces)
      {
         auto& attrs = iface.second;
         const auto mode = attrs.find(FunctionArchitecture::iface_mode);
         if(mode == attrs.end() || mode->second != "m_axi")
         {
            continue;
         }

         auto& resolution = resolutions[{function_symbol, iface.first}];
         resolution.enabled = burst_feature_enabled;
         if(!burst_feature_enabled)
         {
            continue;
         }

         resolution.read = ParseAXIReadBurstProfile(attrs, iface.first, &resolution.origins);
         resolution.write = ParseAXIWriteBurstProfile(attrs, iface.first, &resolution.origins);
         const auto bundle_has_bound_parameter =
             std::any_of(architecture->parms.begin(), architecture->parms.end(), [&](const auto& parameter) {
                const auto bundle = parameter.second.find(FunctionArchitecture::parm_bundle);
                return bundle != parameter.second.end() && bundle->second == iface.first;
             });
         if(!bundle_has_bound_parameter)
         {
            continue;
         }

         if(resolution.origins.read_burst_default)
         {
            attrs.emplace(FunctionArchitecture::iface_max_read_burst_length, STR(resolution.read.max_burst_length));
         }
         if(resolution.origins.write_burst_default)
         {
            attrs.emplace(FunctionArchitecture::iface_max_write_burst_length, STR(resolution.write.max_burst_length));
         }
         resolution.materialized = true;
      }
   }
   return resolutions;
}

std::uint32_t ParseAXIInterfaceLatency(const FunctionArchitecture::iface_attrs& iface_attrs,
                                       const std::string& bundle_name)
{
   const auto it = iface_attrs.find(FunctionArchitecture::iface_latency);
   if(it == iface_attrs.end())
   {
      return 0U;
   }
   const auto& text = it->second;
   const auto fail = [&]() -> void {
      THROW_ERROR_USAGE("Invalid AXI interface attribute 'latency' for bundle '" + bundle_name +
                        "': expected an ASCII decimal integer in [0,4294967295].");
   };
   std::uint32_t value = 0U;
   if(panda::utility::parse_axi_profile_decimal(text, 0U, panda::utility::axi_latency_max, value) !=
      panda::utility::bounded_decimal_result::valid)
   {
      fail();
   }
   return value;
}
