// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "HLS/hls_manager.hpp"
#include "BambuParameter.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(axi_burst_profiles_use_vitis_defaults)
{
   const FunctionArchitecture::iface_attrs empty;
   const auto read = ParseAXIReadBurstProfile(empty, "read");
   BOOST_CHECK_EQUAL(read.max_outstanding, 16U);
   BOOST_CHECK_EQUAL(read.fifo_depth, 256U);

   const auto write = ParseAXIWriteBurstProfile(empty, "write");
   BOOST_CHECK_EQUAL(write.max_burst_length, 16U);
   BOOST_CHECK_EQUAL(write.max_outstanding, 16U);
   BOOST_CHECK_EQUAL(write.fifo_depth, 32U);
}

BOOST_AUTO_TEST_CASE(m_axi_burst_parameter_is_enabled_by_default_and_overridable)
{
   char program_name[] = "libbambu_unit";
   char* argv[] = {program_name};
   BambuParameter parameters(program_name, 1, argv);
   BOOST_CHECK_EQUAL(parameters.GetParameter<std::string>("m-axi-burst"), "1");
   BOOST_CHECK(!parameters.IsParameter("experimental-m-axi-burst"));

   parameters.SetPandaParameterFromCli("m-axi-burst", "0");
   BOOST_CHECK_EQUAL(parameters.GetParameter<std::string>("m-axi-burst"), "0");
}

BOOST_AUTO_TEST_CASE(axi_burst_profile_attributes_override_defaults)
{
   FunctionArchitecture::iface_attrs read_attrs;
   read_attrs[FunctionArchitecture::iface_num_read_outstanding] = "3";
   read_attrs[FunctionArchitecture::iface_max_read_burst_length] = "8";
   const auto read = ParseAXIReadBurstProfile(read_attrs, "read");
   BOOST_CHECK_EQUAL(read.max_outstanding, 3U);
   BOOST_CHECK_EQUAL(read.fifo_depth, 32U);

   FunctionArchitecture::iface_attrs write_attrs;
   write_attrs[FunctionArchitecture::iface_max_write_burst_length] = "8";
   write_attrs[FunctionArchitecture::iface_write_burst_outstanding] = "3";
   write_attrs[FunctionArchitecture::iface_write_fifo_depth] = "64";
   const auto write = ParseAXIWriteBurstProfile(write_attrs, "write");
   BOOST_CHECK_EQUAL(write.max_burst_length, 8U);
   BOOST_CHECK_EQUAL(write.max_outstanding, 3U);
   BOOST_CHECK_EQUAL(write.fifo_depth, 64U);
}

BOOST_AUTO_TEST_CASE(axi_burst_profiles_reject_invalid_explicit_overrides)
{
   FunctionArchitecture::iface_attrs read_attrs;
   read_attrs[FunctionArchitecture::iface_num_read_outstanding] = "0";
   BOOST_CHECK_THROW(ParseAXIReadBurstProfile(read_attrs, "read"), std::string);

   FunctionArchitecture::iface_attrs write_attrs;
   write_attrs[FunctionArchitecture::iface_max_write_burst_length] = "257";
   BOOST_CHECK_THROW(ParseAXIWriteBurstProfile(write_attrs, "write"), std::string);
}
