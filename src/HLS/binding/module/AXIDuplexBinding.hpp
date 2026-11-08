/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file AXIDuplexBinding.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef AXI_DUPLEX_BINDING_HPP
#define AXI_DUPLEX_BINDING_HPP

#include "structural_objects.hpp"

#include <map>
#include <string>
#include <vector>

REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(functional_unit);
CONSTREF_FORWARD_DECL(Parameter);

namespace AXIDuplexBinding
{
   enum class endpoint_role
   {
      read,
      write
   };

   struct metadata
   {
      endpoint_role role;
      std::string bundle;
      std::string aggregator;
      unsigned int address_bitsize;
      unsigned int data_bitsize;
      unsigned int latency;
      unsigned int read_burst_length;
      unsigned int read_outstanding;
      unsigned int read_fifo_depth;
      unsigned int write_burst_length;
      unsigned int write_outstanding;
      unsigned int write_fifo_depth;

      bool operator==(const metadata& other) const;
      std::map<std::string, std::string> technology_attributes() const;
   };

   struct endpoint
   {
      structural_objectRef module;
      metadata profile;
   };

   struct bundle_endpoints
   {
      std::vector<endpoint> readers;
      std::vector<endpoint> writers;
   };

   using bundles = std::map<std::string, bundle_endpoints>;

   metadata parse_metadata(const std::map<std::string, std::string>& values);
   bool collect_endpoint(const functional_unit* unit, const structural_objectRef& module, endpoint& result);
   void add_endpoint(bundles& collected, endpoint value);
   std::vector<structural_objectRef> connect(const HLS_managerRef HLSMgr, const hlsRef HLS,
                                             const structural_objectRef& clock_port,
                                             const structural_objectRef& reset_port, const bundles& endpoints,
                                             const ParameterConstRef parameters, unsigned int& unique_id);
} // namespace AXIDuplexBinding

#endif // AXI_DUPLEX_BINDING_HPP
