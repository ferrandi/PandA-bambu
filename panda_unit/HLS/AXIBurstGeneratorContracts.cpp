// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/**
 * @file AXIBurstGeneratorContracts.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "HLS/hdl_generators/ReadWriteMAXIHDLGenerator.hpp"

#include "Parameter.hpp"
#include "circuit/structural_manager.hpp"
#include "circuit/structural_objects.hpp"
#include "constant_strings.hpp"
#include "design_flows/backend/ToHDL/language_writer.hpp"
#include "technology/library_manager.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <sstream>

namespace
{
   class TestParameters : public Parameter
   {
    public:
      TestParameters() : Parameter("libbambu_unit", 0, nullptr)
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

   ParameterConstRef make_test_parameters()
   {
      return ParameterConstRef(new TestParameters());
   }

   std::vector<HDLGenerator::parameter> legacy_inputs()
   {
      std::vector<HDLGenerator::parameter> ports(22);
      for(std::size_t i = 0; i < ports.size(); ++i)
      {
         ports[i].name = "input_" + std::to_string(i);
         ports[i].type = "bool";
         ports[i].type_size = 1;
         ports[i].alignment = 1;
      }
      ports[7].name = CACHE_RESET_PORT_NAME;
      ports[3].type_size = 2;
      ports[4].type_size = 32;
      ports[5].type_size = 32;
      ports[6].type_size = 32;
      ports[10].type_size = 6;
      ports[11].type_size = 2;
      ports[15].type_size = 6;
      ports[16].type_size = 32;
      ports[17].type_size = 2;
      return ports;
   }

   std::vector<HDLGenerator::parameter> burst_inputs()
   {
      auto ports = legacy_inputs();
      HDLGenerator::parameter in5;
      in5.name = "in5";
      in5.type = "bool";
      in5.type_size = 32;
      in5.alignment = 1;
      // Workaround for a GCC 13 false positive -Warray-bounds on the inlined
      // std::vector::insert path: append the port and rotate it into place.
      ports.push_back(in5);
      std::rotate(ports.begin() + 7, ports.end() - 1, ports.end());
      return ports;
   }

   std::vector<HDLGenerator::parameter> legacy_outputs()
   {
      std::vector<HDLGenerator::parameter> ports(34);
      for(std::size_t i = 0; i < ports.size(); ++i)
      {
         ports[i].name = "output_" + std::to_string(i);
         ports[i].type = "bool";
         ports[i].type_size = 1;
         ports[i].alignment = 1;
      }
      ports[1].type_size = 32;
      ports[2].type_size = 6;
      ports[3].type_size = 32;
      ports[4].type_size = 8;
      ports[5].type_size = 3;
      ports[6].type_size = 2;
      ports[8].type_size = 4;
      ports[9].type_size = 3;
      ports[10].type_size = 4;
      ports[11].type_size = 4;
      ports[12].type_size = 1;
      ports[14].type_size = 32;
      ports[15].type_size = 4;
      ports[16].type_size = 1;
      ports[20].type_size = 6;
      ports[21].type_size = 32;
      ports[22].type_size = 8;
      ports[23].type_size = 3;
      ports[24].type_size = 2;
      ports[26].type_size = 4;
      ports[27].type_size = 3;
      ports[28].type_size = 4;
      ports[29].type_size = 4;
      ports[30].type_size = 1;
      return ports;
   }

   struct GeneratorFixture
   {
      ParameterConstRef parameters = make_test_parameters();
      structural_managerRef manager{new structural_manager(parameters)};
      structural_type_descriptorRef module_type{new structural_type_descriptor("test")};
      structural_objectRef module;
      ReadWriteMAXIHDLGenerator generator{HLS_managerRef()};

      explicit GeneratorFixture(const std::string& mode = "read")
      {
         manager->set_top_info("test", module_type);
         module = manager->get_circ();
         GetPointerS<module_o>(module)->add_attribute(
             "bambu_axi_burst_mode", attributeRef(new attribute(attribute::STRING, mode)));
         manager->add_port("in5", port_o::IN, module,
                           structural_type_descriptorRef(new structural_type_descriptor("bool", 32)));
      }

      void run(const std::vector<HDLGenerator::parameter>& inputs, const std::vector<HDLGenerator::parameter>& outputs)
      {
         std::ostringstream generated;
         generator.InternalExec(generated, module, 0, gc_vertex_descriptor{}, HDLWriter_Language::VERILOG, {}, inputs,
                                outputs, {});
      }
   };
} // namespace

BOOST_AUTO_TEST_CASE(axi_burst_generator_rejects_missing_fifth_operand)
{
   GeneratorFixture fixture;
   auto inputs = burst_inputs();
   inputs[7].name = "in4_again";
   BOOST_CHECK_THROW(fixture.run(inputs, legacy_outputs()), std::string);
}

BOOST_AUTO_TEST_CASE(axi_burst_generator_rejects_short_input_and_output_vectors)
{
   GeneratorFixture fixture;
   BOOST_CHECK_THROW(fixture.run({}, legacy_outputs()), std::string);

   auto inputs = burst_inputs();
   auto outputs = legacy_outputs();
   outputs.resize(32);
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);
}

BOOST_AUTO_TEST_CASE(axi_burst_generator_rejects_data_address_and_id_widths)
{
   GeneratorFixture fixture;
   auto inputs = burst_inputs();
   auto outputs = legacy_outputs();

   inputs[5].type_size = 16;
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);

   inputs = burst_inputs();
   inputs[6].type_size = 64;
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);

   inputs = burst_inputs();
   inputs[16].type_size = 5;
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);
}

BOOST_AUTO_TEST_CASE(axi_burst_generator_rejects_write_address_and_data_widths)
{
   GeneratorFixture fixture("write");
   auto inputs = burst_inputs();
   auto outputs = legacy_outputs();

   outputs[3].type_size = 64;
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);

   outputs = legacy_outputs();
   outputs[14].type_size = 16;
   BOOST_CHECK_THROW(fixture.run(inputs, outputs), std::string);
}
