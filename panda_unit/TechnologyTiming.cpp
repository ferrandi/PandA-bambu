// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Parameter.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "xml_element.hpp"

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>

namespace
{
   class TechnologyTimingParameters : public Parameter
   {
    public:
      TechnologyTimingParameters() : Parameter("technology_timing", 0, nullptr)
      {
         SetCommonDefaults();
         setOption(OPT_debug_level, 0);
         setOption(OPT_output_level, 0);
      }
      int Exec() override
      {
         return 0;
      }
      void CheckParameters() override
      {
      }

    protected:
      void SetDefaults() override
      {
      }
      void PrintProgramName(std::ostream&) const override
      {
      }
      void PrintHelp(std::ostream&) const override
      {
      }
   };

   void load_stallable_attribute(functional_unit& fu, const std::string& value_type = {}, const std::string& value = {},
                                bool set_value_type = true, bool add_value = true)
   {
      xml_element node("cell");
      auto* name = node.add_child_element("name");
      name->add_child_text("test_fu");
      if(!value_type.empty() || !value.empty() || !set_value_type || !add_value)
      {
         auto* stallable = node.add_child_element("attribute");
         stallable->set_attribute("name", "Stallable");
         if(set_value_type)
         {
            stallable->set_attribute("value_type", value_type);
         }
         if(add_value)
         {
            stallable->add_child_text(value);
         }
      }
      const auto parameters = std::make_shared<TechnologyTimingParameters>();
      fu.xload(&node, {}, parameters);
   }
} // namespace

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
   BOOST_CHECK_EXCEPTION(load_operation(invalid_derived_latency, 0, 1), std::string, invalid_derived_timing_model);

   operation derived_latency;
   BOOST_CHECK_NO_THROW(load_operation(derived_latency, 0, 0));
   BOOST_CHECK_EQUAL(derived_latency.time_m->get_cycles(), 0U);
   BOOST_CHECK_EQUAL(derived_latency.time_m->get_initiation_time(), 0U);
}

BOOST_AUTO_TEST_CASE(functional_unit_stallable_attribute)
{
   functional_unit absent;
   BOOST_CHECK_NO_THROW(load_stallable_attribute(absent));
   BOOST_CHECK(!absent.is_stallable());

   functional_unit enabled_template;
   BOOST_CHECK_NO_THROW(load_stallable_attribute(enabled_template, "boolean", "1"));
   BOOST_CHECK(enabled_template.is_stallable());

   functional_unit inherited;
   BOOST_CHECK_NO_THROW(load_stallable_attribute(inherited));
   BOOST_CHECK(inherited.is_stallable(&enabled_template));

   functional_unit disabled_override;
   BOOST_CHECK_NO_THROW(load_stallable_attribute(disabled_override, "boolean", "0"));
   BOOST_CHECK(!disabled_override.is_stallable(&enabled_template));

   functional_unit invalid_type;
   BOOST_CHECK_EXCEPTION(
       load_stallable_attribute(invalid_type, "string", "1"), std::string,
       [](const std::string& message) { return message.find("expected a boolean attribute") != std::string::npos; });

   functional_unit invalid_value;
   BOOST_CHECK_EXCEPTION(
       load_stallable_attribute(invalid_value, "boolean", "yes"), std::string,
       [](const std::string& message) { return message.find("expected true, false, 1, or 0") != std::string::npos; });

   functional_unit missing_value_type;
   BOOST_CHECK_EXCEPTION(
       load_stallable_attribute(missing_value_type, {}, "1", false), std::string,
       [](const std::string& message) {
          return message.find("Stallable property is missing its value_type") != std::string::npos;
       });

   functional_unit missing_value;
   BOOST_CHECK_EXCEPTION(
       load_stallable_attribute(missing_value, "boolean", {}, true, false), std::string,
       [](const std::string& message) {
          return message.find("Stallable property is missing its boolean value") != std::string::npos;
       });
}
