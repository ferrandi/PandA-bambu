// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Standalone integration test for production XML/YAML resolution.
#include "ramulator_config.hpp"
#include <yaml-cpp/yaml.h>
#include <pugixml.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <functional>
#include <stdexcept>
namespace fs = std::filesystem;
int main(int argc, char** argv)
{
   assert(argc == 3);
   const fs::path library = argv[1], directory = argv[2];
   fs::create_directories(directory);
   unsigned checks = 0;
   auto write = [&](const std::string& name, const std::string& value)
   {
      auto path = directory / name;
      std::ofstream(path) << value;
      return path.string();
   };
   auto expect_error = [&](const std::function<void()>& action)
   {
      bool failed = false;
      try
      {
         action();
      }
      catch(const std::exception&)
      {
         failed = true;
      }
      assert(failed);
      ++checks;
   };
   RamulatorConfigInputs in;
   in.library = library;
   in.output = directory;
   in.profile = "alveo-u55c";
   const auto first = resolve_ramulator_config(in);
   assert((first.cycles == std::array<unsigned, 4>{{1, 1, 1, 1}}));
   ++checks;
   auto native = YAML::LoadFile(first.configuration.string());
   assert(native["memory_system"]["controllers"].size() == 16);
   ++checks;
   const auto template_before = YAML::Dump(YAML::LoadFile((library / "configs/boards/alveo-u55c.yaml").string()));
   in.clock_period_ns = 5;
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   in.clock_period_ns = 10;
   auto probe = write(
       "probe.xml",
       R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="30" write_forward="30.000001" read_return="0" write_return="11"/></ramulator_configuration>)");
   in.imports = {probe};
   assert((resolve_ramulator_config(in).cycles == std::array<unsigned, 4>{{3, 4, 0, 2}}));
   ++checks;
   in.imports = {write(
       "fractional-fs.xml",
       R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="30.000000000001" write_forward="0.0000001" read_return="10.1"/></ramulator_configuration>)")};
   assert((resolve_ramulator_config(in).cycles == std::array<unsigned, 4>{{4, 1, 2, 1}}));
   ++checks;
   auto discovery = write("discovery.yaml", "memory_system:\n  channel_mapper:\n    interleave_bits: 2\n");
   in.imports = {probe, discovery};
   auto both = resolve_ramulator_config(in);
   assert(both.cycles[0] == 3);
   ++checks;
   native = YAML::LoadFile(both.configuration.string());
   assert(native["memory_system"]["channel_mapper"]["interleave_bits"].as<int>() == 2);
   ++checks;
   in.imports = {discovery};
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   assert(template_before == YAML::Dump(YAML::LoadFile((library / "configs/boards/alveo-u55c.yaml").string())));
   ++checks;
   auto partial =
       write("partial.xml",
             R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="25"/></ramulator_configuration>)");
   in.imports = {partial};
   assert((resolve_ramulator_config(in).cycles == std::array<unsigned, 4>{{3, 1, 1, 1}}));
   ++checks;
   auto switch_file = write(
       "switch.xml",
       R"(<ramulator_configuration schema_version="1"><hbm_switch enabled="true" read_correction_ns="20" write_correction_ns="30"/></ramulator_configuration>)");
   in.imports = {switch_file};
   assert((resolve_ramulator_config(in).cycles == std::array<unsigned, 4>{{1, 1, 3, 4}}));
   ++checks;
   for(const auto& xml :
       {R"(<ramulator_configuration schema_version="2"/>)",
        R"(<ramulator_configuration schema_version="1"><unknown/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><delay_ns read_foward="5"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="-1"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="nan"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><delay_ns read_forward="1e100"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><hbm_switch enabled="true"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><identity board_model="another-board"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1" profile="another-board"/>)",
        R"(<ramulator_configuration schema_version="1" status="calibrated"/>)",
        R"(<ramulator_configuration schema_version="1"><clocks_hz accelerator="200000000"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><clocks_hz dram="500000000"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1"><ramulator><set path="/unknown" value="1"/></ramulator></ramulator_configuration>)"})
   {
      in.imports = {write("invalid.xml", xml)};
      expect_error([&] { resolve_ramulator_config(in); });
   }
   in.imports = {probe, partial};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write("cycle.yaml", "!include cycle.yaml\n")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write("alias-cycle.yaml", "memory_system: &loop {loop: *loop}\n")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write("duplicate.yaml", "memory_system: {clock_ratio: 1, clock_ratio: 2}\n")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write("metadata.yaml", "bambu: forbidden\n")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports.clear();
   in.profile = "missing-board";
   expect_error([&] { resolve_ramulator_config(in); });
   in.profile.clear();
   in.imports.clear();
   assert(resolve_ramulator_config(in).configuration.empty());
   ++checks;
   in.profile = "alveo-u55c";
   in.imports = {write(
       "override.xml",
       R"(<ramulator_configuration schema_version="1"><ramulator><set path="/memory_system/controllers/0/row_policy/impl" value="ClosedCAP"/></ramulator></ramulator_configuration>)")};
   native = YAML::LoadFile(resolve_ramulator_config(in).configuration.string());
   assert(native["memory_system"]["controllers"][0]["row_policy"]["impl"].as<std::string>() == "ClosedCAP");
   assert(native["memory_system"]["controllers"][1]["row_policy"]["impl"].as<std::string>() == "Open");
   ++checks;
   in.imports = {write(
       "invalid-disabled-switch.xml",
       R"(<ramulator_configuration schema_version="1"><hbm_switch enabled="false" read_correction_ns="-1"/></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   const std::string qualified =
       R"(<ramulator_configuration schema_version="1" status="calibrated" profile="alveo-u55c">
     <identity board_model="alveo-u55c" fpga_speed_grade="example-fpga" dram_model="example-part" dram_speed_grade="example-memory" configuration_id="measured"/>
     <clocks_hz accelerator="100000000" axi_frontend="100000000" dram="1000000000"/>
     <delay_ns read_forward="12.5" write_forward="20" read_return="30" write_return="40"/>
     <controller_configuration><mode b="2" a="1"/><policy value="open"/></controller_configuration>
   </ramulator_configuration>)";
   in.imports.clear();
   in.calibration = write("qualified.xml", qualified);
   assert((resolve_ramulator_config(in).cycles == std::array<unsigned, 4>{{2, 2, 3, 4}}));
   ++checks;
   pugi::xml_document qualified_report;
   qualified_report.load_file((directory / "memory-model/resolved-memory-model.xml").c_str());
   assert(qualified_report.document_element()
              .child("resolved")
              .child("selection_key")
              .child("identity")
              .attribute("fpga_speed_grade")
              .as_string() == std::string("example-fpga"));
   ++checks;
   const auto axi_key = [&]()
   {
      pugi::xml_document report;
      if(!report.load_file((directory / "memory-model/resolved-memory-model.xml").c_str()))
         throw std::runtime_error("Missing calibration report");
      return std::string(report.document_element().child("resolved").child("selection_key")
                             .child("clocks_hz").attribute("axi_frontend").value());
   };
   if(axi_key() != "100000000") throw std::runtime_error("Missing AXI frequency in calibration key");
   ++checks;
   in.imports = {write(
       "same-axi-clock.xml",
       R"(<ramulator_configuration schema_version="1"><clocks_hz axi_frontend="1e8"/></ramulator_configuration>)")};
   const auto same_axi = resolve_ramulator_config(in);
   if(same_axi.status != "calibrated" || axi_key() != "100000000")
      throw std::runtime_error("Equivalent AXI frequencies changed calibration key");
   ++checks;
   // Changing only the frequency must not reuse a previously calibrated delay.
   for(const auto& overlay :
       {R"(<ramulator_configuration schema_version="1"><clocks_hz axi_frontend="225000000"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1" status="calibrated"><clocks_hz axi_frontend="225000000"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1" status="calibrated"><clocks_hz accelerator="100000000" axi_frontend="225000000" dram="1000000000"/><delay_ns read_forward="10" write_forward="20" read_return="30"/></ramulator_configuration>)",
        R"(<ramulator_configuration schema_version="1" status="calibrated"><clocks_hz axi_frontend="225000000"/><delay_ns read_forward="10" write_forward="20" read_return="30" write_return="40"/></ramulator_configuration>)"})
   {
      in.imports = {write("changed-axi-clock.xml", overlay)};
      bool failed = false;
      try { resolve_ramulator_config(in); }
      catch(const std::runtime_error& error)
      {
         failed = std::string(error.what()).find("AXI frontend frequency") != std::string::npos;
      }
      if(!failed) throw std::runtime_error("Mismatched AXI frequency accepted without a complete new probe");
      ++checks;
   }
   auto requalified = qualified;
   requalified.replace(requalified.find(R"(axi_frontend="100000000")"),
                       std::string(R"(axi_frontend="100000000")").size(), R"(axi_frontend="225000000")");
   requalified.replace(requalified.find(R"(read_forward="12.5")"),
                       std::string(R"(read_forward="12.5")").size(), R"(read_forward="25")");
   in.imports = {write("new-axi-probe.xml", requalified)};
   const auto new_axi = resolve_ramulator_config(in);
   if(new_axi.status != "calibrated" || new_axi.cycles[0] != 3 || axi_key() != "225000000")
      throw std::runtime_error("Complete probe for new AXI frequency was not selected");
   ++checks;
   in.calibration = write("new-axi-calibration.xml", requalified);
   in.imports.clear();
   const auto selected_axi = resolve_ramulator_config(in);
   if(selected_axi.status != "calibrated" || axi_key() != "225000000")
      throw std::runtime_error("Explicit configuration did not select its AXI frequency");
   ++checks;
   in.calibration = write("qualified.xml", qualified);
   in.imports = {write(
       "reordered.xml",
       R"(<ramulator_configuration schema_version="1"><controller_configuration><policy value="open"/><mode a="1" b="2"/></controller_configuration><delay_ns read_return="31"/></ramulator_configuration>)")};
   assert(resolve_ramulator_config(in).cycles[2] == 4);
   ++checks;
   in.imports = {write(
       "changed.xml",
       R"(<ramulator_configuration schema_version="1"><controller_configuration><mode a="2"/></controller_configuration></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write(
       "identity-change.xml",
       R"(<ramulator_configuration schema_version="1"><identity fpga_speed_grade="other"/></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports.clear();
   in.calibration.clear();
   in.profile = "alveo-u250";
   in.imports = {write(
       "ddr-clock.xml",
       R"(<ramulator_configuration schema_version="1"><clocks_hz dram="1200000000"/></ramulator_configuration>)")};
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   in.imports.clear();
   in.clock_period_ns = 3.333333333333;
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   in.clock_period_ns = 10;
   in.profile.clear();
   in.calibration = write("qualified.xml", qualified);
   assert(resolve_ramulator_config(in).cycles[0] == 2);
   ++checks;
   in.calibration.clear();
   in.imports = {(library / "calibrations/examples/probe.xml").string()};
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   in.profile.clear();
   in.calibration.clear();
   in.imports = {write(
       "identity-only.xml",
       R"(<ramulator_configuration schema_version="1"><identity board_model="alveo-u55c"/></ramulator_configuration>)")};
   assert(resolve_ramulator_config(in).cycles[0] == 1);
   ++checks;
   in.calibration = write("qualified.xml", qualified);
   in.imports = {write("changed-discovery.yaml", "memory_system:\n  channel_mapper: {interleave_bits: 1}\n")};
   const auto changed = resolve_ramulator_config(in);
   assert(changed.status == "experimental" && changed.cycles[0] == 2);
   ++checks;
   auto empty_configuration = qualified;
   const auto raw_start = empty_configuration.find("<controller_configuration>");
   const auto raw_end = empty_configuration.find("</controller_configuration>", raw_start) +
                        std::string("</controller_configuration>").size();
   empty_configuration.replace(raw_start, raw_end - raw_start, "<controller_configuration/>");
   in.profile.clear();
   in.imports.clear();
   in.calibration = write("empty-calibrated.xml", empty_configuration);
   expect_error([&] { resolve_ramulator_config(in); });
   in.calibration.clear();
   in.profile = "alveo-u55c";
   in.imports = {write(
       "provenance.xml",
       R"(<ramulator_configuration schema_version="1"><provenance><note>Imported evidence text</note></provenance></ramulator_configuration>)")};
   resolve_ramulator_config(in);
   pugi::xml_document evidence;
   evidence.load_file((directory / "memory-model/resolved-memory-model.xml").c_str());
   assert(evidence.document_element().child("provenance").child("note").text().as_string() ==
          std::string("Imported evidence text"));
   ++checks;
   in.calibration = write("qualified.xml", qualified);
   in.imports = {write(
       "changed-policy.xml",
       R"(<ramulator_configuration schema_version="1"><ramulator><set path="/memory_system/controllers/0/row_policy/impl" value="ClosedCAP"/></ramulator></ramulator_configuration>)")};
   assert(resolve_ramulator_config(in).status == "experimental");
   ++checks;
   in.imports = {write(
       "unmeasured-policy.xml",
       R"(<ramulator_configuration schema_version="1" status="calibrated"><ramulator><set path="/memory_system/controllers/0/row_policy/impl" value="ClosedCAP"/></ramulator></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.calibration.clear();
   in.profile = "alveo-u55c";
   in.imports = {write(
       "clock-owner.xml",
       R"(<ramulator_configuration schema_version="1"><ramulator><set path="/memory_system/clock_ratio" value="3"/></ramulator></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write(
       "frontend-owner.xml",
       R"(<ramulator_configuration schema_version="1"><ramulator><set path="/frontend/impl" value="SimpleO3"/></ramulator></ramulator_configuration>)")};
   expect_error([&] { resolve_ramulator_config(in); });
   in.imports = {write(
       "controller-default.xml",
       R"(<ramulator_configuration schema_version="1"><ramulator><set path="/memory_system/controllers/0/read_buffer_size" value="8"/></ramulator></ramulator_configuration>)")};
   const auto defaults = YAML::LoadFile(resolve_ramulator_config(in).configuration.string());
   assert(defaults["memory_system"]["controllers"][0]["read_buffer_size"].as<int>() == 8);
   assert(defaults["memory_system"]["controllers"][1]["read_buffer_size"].as<int>() == 32);
   ++checks;
   // Keep alias isolation covered even when upstream serializes controllers separately.
   auto aliased = YAML::LoadFile((library / "configs/boards/alveo-u55c.yaml").string());
   aliased["memory_system"]["controllers"][1] = aliased["memory_system"]["controllers"][0];
   assert(aliased["memory_system"]["controllers"][1].is(aliased["memory_system"]["controllers"][0]));
   in.imports.insert(in.imports.begin(), write("aliased-controller.yaml", YAML::Dump(aliased)));
   const auto isolated = YAML::LoadFile(resolve_ramulator_config(in).configuration.string());
   assert(isolated["memory_system"]["controllers"][0]["read_buffer_size"].as<int>() == 8);
   assert(isolated["memory_system"]["controllers"][1]["read_buffer_size"].as<int>() == 32);
   ++checks;
   in.imports = {write("exact-decimal.xml",
                       "<ramulator_configuration schema_version=\"1\"><delay_ns read_forward=\"10." +
                           std::string(100, '0') + "1\"/></ramulator_configuration>")};
   assert(resolve_ramulator_config(in).cycles[0] == 2);
   ++checks;
   in.imports = {write(
       "exact-switch.xml",
       R"(<ramulator_configuration schema_version="1"><hbm_switch enabled="true" read_correction_ns="1e-100" write_correction_ns="0"/></ramulator_configuration>)")};
   assert(resolve_ramulator_config(in).cycles[2] == 2);
   ++checks;
   in.imports = {write("exact-switch-sum.xml",
                       "<ramulator_configuration schema_version=\"1\"><delay_ns read_return=\"9." +
                           std::string(100, '9') +
                           "\"/><hbm_switch enabled=\"true\" read_correction_ns=\"1e-100\" "
                           "write_correction_ns=\"0\"/></ramulator_configuration>")};
   assert(resolve_ramulator_config(in).cycles[2] == 1);
   ++checks;
   std::cout << "PASS Ramulator configuration contracts: " << checks << " checks\n";
}
