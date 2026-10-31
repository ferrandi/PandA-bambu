// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "technology_node.hpp"
#include "time_info.hpp"
#include "xml_element.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_CASE(technology_operation_rejects_initiation_interval_above_latency)
{
   const auto make_fu = [] {
      technology_nodeRef fu(new functional_unit);
      GetPointer<functional_unit>(fu)->functional_unit_name = "test_fu";
      return fu;
   };
   const auto load_operation = [&make_fu](operation& op, unsigned int cycles, unsigned int initiation_time) {
      xml_element node("operation");
      node.set_attribute("operation_name", "test_op");
      node.set_attribute("cycles", STR(cycles));
      node.set_attribute("initiation_time", STR(initiation_time));
      op.xload(&node, make_fu(), {});
   };
   const auto invalid_timing_model = [](const std::string& message) {
      return message.find("initiation_time (3) exceeds latency (cycles=2)") != std::string::npos;
   };
   const auto invalid_derived_timing_model = [](const std::string& message) {
      return message.find("cycles=0 is reserved for non-pipelineable units") != std::string::npos;
   };

   operation invalid;
   BOOST_CHECK_EXCEPTION(load_operation(invalid, 2, 3), std::string, invalid_timing_model);

   operation equal;
   BOOST_CHECK_NO_THROW(load_operation(equal, 2, 2));
   BOOST_CHECK_EQUAL(equal.time_m->get_cycles(), 2U);
   BOOST_CHECK_EQUAL(equal.time_m->get_initiation_time(), 2U);

   operation invalid_derived_latency;
   BOOST_CHECK_EXCEPTION(load_operation(invalid_derived_latency, 0, 1), std::string,
                         invalid_derived_timing_model);

   operation derived_latency;
   BOOST_CHECK_NO_THROW(load_operation(derived_latency, 0, 0));
   BOOST_CHECK_EQUAL(derived_latency.time_m->get_cycles(), 0U);
   BOOST_CHECK_EQUAL(derived_latency.time_m->get_initiation_time(), 0U);
}
