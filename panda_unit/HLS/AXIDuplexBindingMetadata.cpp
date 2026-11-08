// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/**
 * @file AXIDuplexBindingMetadata.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "HLS/binding/module/AXIDuplexBinding.hpp"

#include <boost/test/unit_test.hpp>

namespace
{
   std::map<std::string, std::string> valid_metadata()
   {
      return {{"bambu_axi_duplex_role", "read_endpoint"},
              {"bambu_axi_burst_mode", "read"},
              {"bambu_axi_bundle_name", "memory"},
              {"bambu_axi_duplex_aggregator", "DuplexAXI"},
              {"bambu_axi_address_bitsize", "64"},
              {"bambu_axi_data_bitsize", "32"},
              {"bambu_axi_latency", "0"},
              {"bambu_max_read_burst_length", "16"},
              {"bambu_num_read_outstanding", "4"},
              {"bambu_read_fifo_depth", "128"},
              {"bambu_max_write_burst_length", "8"},
              {"bambu_write_burst_outstanding", "2"},
              {"bambu_write_fifo_depth", "16"}};
   }
} // namespace

BOOST_AUTO_TEST_CASE(axi_duplex_metadata_is_typed_and_validated)
{
   const auto parsed = AXIDuplexBinding::parse_metadata(valid_metadata());
   BOOST_CHECK(parsed.role == AXIDuplexBinding::endpoint_role::read);
   BOOST_CHECK_EQUAL(parsed.bundle, "memory");
   BOOST_CHECK_EQUAL(parsed.aggregator, "DuplexAXI");
   BOOST_CHECK_EQUAL(parsed.address_bitsize, 64U);
   BOOST_CHECK_EQUAL(parsed.data_bitsize, 32U);
   BOOST_CHECK_EQUAL(parsed.latency, 0U);
   BOOST_CHECK_EQUAL(parsed.read_burst_length, 16U);
   BOOST_CHECK_EQUAL(parsed.read_outstanding, 4U);
   BOOST_CHECK_EQUAL(parsed.read_fifo_depth, 128U);
   BOOST_CHECK_EQUAL(parsed.write_burst_length, 8U);
   BOOST_CHECK_EQUAL(parsed.write_outstanding, 2U);
   BOOST_CHECK_EQUAL(parsed.write_fifo_depth, 16U);
}

BOOST_AUTO_TEST_CASE(axi_duplex_metadata_rejects_invalid_contracts)
{
   auto malformed = valid_metadata();
   malformed["bambu_axi_burst_mode"] = "write";
   BOOST_CHECK_THROW(AXIDuplexBinding::parse_metadata(malformed), std::string);

   malformed = valid_metadata();
   malformed["bambu_max_read_burst_length"] = "257";
   BOOST_CHECK_THROW(AXIDuplexBinding::parse_metadata(malformed), std::string);

   malformed = valid_metadata();
   malformed["bambu_write_fifo_depth"] = "4";
   BOOST_CHECK_THROW(AXIDuplexBinding::parse_metadata(malformed), std::string);

   malformed = valid_metadata();
   malformed["bambu_axi_address_bitsize"] = "48";
   BOOST_CHECK_THROW(AXIDuplexBinding::parse_metadata(malformed), std::string);

   malformed = valid_metadata();
   malformed.erase("bambu_axi_duplex_aggregator");
   BOOST_CHECK_THROW(AXIDuplexBinding::parse_metadata(malformed), std::string);
}
