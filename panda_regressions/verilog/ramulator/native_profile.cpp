// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ramulator/base/config.h"
#include "ramulator/bambu/legacy_config.h"
#include "ramulator/controller/controller_base.h"
#include "ramulator/frontend/i_frontend.h"
#include "ramulator/memory_system/i_memory_system.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
int main(int argc, char** argv)
{
   assert(argc == 3);
   auto yaml = YAML::LoadFile(argv[1]);
   const auto config = Ramulator::Bambu::factory_configuration(yaml);
   std::unique_ptr<Ramulator::IFrontEnd> frontend(Ramulator::Factory::create_frontend(config));
   std::unique_ptr<Ramulator::IMemorySystem> memory(Ramulator::Factory::create_memory_system(config));
   frontend->connect_memory_system(memory.get());
   memory->connect_frontend(frontend.get());
   const auto* controller = memory->get_ifce<Ramulator::ControllerBase>();
   assert(controller);
   const auto* dram = controller->m_device.m_spec;
   uint64_t capacity = memory->get_tx_bytes();
   for(auto count : dram->organization.level_sizes)
      capacity *= count;
   capacity /= dram->internal_prefetch_size;
   const auto channels = yaml["memory_system"]["controllers"].size();
   capacity *= channels;
   assert(capacity == std::stoull(argv[2]));
   const bool hbm = dram->standard_name == "HBM2";
   struct Vector
   {
      uint64_t address;
      const char* level;
      int value;
   };
   const std::vector<Vector> vectors = hbm ? std::vector<Vector>{{32, "Channel", 1},
                                                                 {uint64_t(1) << 14, "PseudoChannel", 1},
                                                                 {uint64_t(1) << 15, "BankGroup", 1},
                                                                 {uint64_t(1) << 17, "Bank", 1},
                                                                 {uint64_t(1) << 19, "Row", 1}}
                                           : std::vector<Vector>{{64, "Channel", 1},
                                                                 {uint64_t(1) << 8, "Column", 1},
                                                                 {uint64_t(1) << 15, "BankGroup", 1},
                                                                 {uint64_t(1) << 17, "Bank", 1},
                                                                 {uint64_t(1) << 19, "Row", 1}};
   for(const auto& vector : vectors)
   {
      bool completed = false;
      Ramulator::Request request(vector.address, Ramulator::Request::Type::Read);
      request.source_id = 0;
      request.size_bytes = memory->get_tx_bytes();
      request.callback = [&](Ramulator::Request& done)
      {
         assert(done.addr_vec[dram->get_level_id(vector.level)] == vector.value);
         completed = true;
      };
      assert(memory->send(request));
      for(int cycle = 0; cycle < 10000 && !completed; ++cycle)
         memory->tick();
      assert(completed);
   }
   // The previous fork guarded invalid mapper coordinates. Preserve that
   // contract for native pass-through requests instead of indexing invalid nodes.
   auto checked_yaml = YAML::Clone(yaml);
   auto prototype = YAML::Clone(checked_yaml["memory_system"]["controllers"][0]);
   prototype["addr_mapper"]["impl"] = "PassThroughAddrMapper";
   checked_yaml["memory_system"]["controllers"] = YAML::Node(YAML::NodeType::Sequence);
   checked_yaml["memory_system"]["controllers"].push_back(prototype);
   const auto checked_config = Ramulator::Bambu::factory_configuration(checked_yaml);
   std::unique_ptr<Ramulator::IMemorySystem> checked_memory(Ramulator::Factory::create_memory_system(checked_config));
   checked_memory->connect_frontend(frontend.get());
   auto* checked_controller = checked_memory->get_ifce<Ramulator::ControllerBase>();
   for(bool wrong_size : {false, true})
   {
      Ramulator::AddrVec_t vector(dram->level_count, 0);
      vector[dram->get_level_id("Bank")] = dram->get_level_size("Bank");
      if(wrong_size)
         vector.resize(1);
      Ramulator::Request request(vector, Ramulator::Request::Type::Read);
      request.addr = 0;
      request.source_id = 0;
      request.size_bytes = memory->get_tx_bytes();
      bool rejected = false;
      try
      {
         checked_controller->send(request);
      }
      catch(const std::exception&)
      {
         rejected = true;
      }
      assert(rejected);
   }
   std::cout << "PASS native profile capacity=" << capacity << ", mapping and real completions\n";
}
