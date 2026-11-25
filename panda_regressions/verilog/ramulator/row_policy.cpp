// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "base/config.h"
#include "dram_controller/controller.h"
#include "frontend/frontend.h"
#include "memory_system/memory_system.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
   assert(argc == 4);
   auto config = YAML::LoadFile(argv[1]);
   config["MemorySystem"]["DRAM"]["org"]["channel"] = 1;
   auto controller_config = config["MemorySystem"]["Controller"];
   const bool closed = std::string(argv[3]) == "ClosedRowPolicy";
   controller_config["RowPolicy"]["impl"] = argv[3];
   controller_config["RowPolicy"]["cap"] = 2;
   YAML::Node counter;
   counter["impl"] = "CommandCounter";
   counter["path"] = argv[2];
   counter["commands_to_count"] = std::vector<std::string>{"ACT", "PRE", "PREA", "RD", "REFab"};
   controller_config["plugins"] = YAML::Node(YAML::NodeType::Sequence);
   YAML::Node plugin;
   plugin["ControllerPlugin"] = counter;
   controller_config["plugins"].push_back(plugin);

   std::unique_ptr<Ramulator::IFrontEnd> frontend(Ramulator::Factory::create_frontend(config));
   std::unique_ptr<Ramulator::IMemorySystem> memory(Ramulator::Factory::create_memory_system(config));
   frontend->connect_memory_system(memory.get());
   memory->connect_frontend(frontend.get());
   auto* controller = memory->get_ifce<Ramulator::IDRAMController>();
   auto* dram = controller->m_dram;
   const bool grouped = dram->m_levels.contains("bankgroup");
   const int ranks = dram->get_level_size("rank");
   const int groups = grouped ? dram->get_level_size("bankgroup") : 1;
   const int banks = dram->get_level_size("bank");
   std::vector<Ramulator::AddrVec_t> addresses;
   for(int rank : ranks > 1 ? std::vector<int>{0, ranks - 1} : std::vector<int>{0})
   {
      for(int group : groups > 1 ? std::vector<int>{0, groups - 1} : std::vector<int>{0})
      {
         for(int bank : {0, banks - 1})
         {
            Ramulator::AddrVec_t address(dram->m_levels.size(), 0);
            address[dram->m_levels("rank")] = rank;
            address[dram->m_levels("bank")] = bank;
            if(grouped)
            {
               address[dram->m_levels("bankgroup")] = group;
            }
            addresses.push_back(address);
         }
      }
   }

   unsigned completed = 0;
   auto read = [&](const Ramulator::AddrVec_t& address)
   {
      bool done = false;
      Ramulator::Request request(address, Ramulator::Request::Type::Read);
      request.source_id = 0;
      request.callback = [&](Ramulator::Request&)
      {
         done = true;
         ++completed;
      };
      assert(controller->send(request));
      // Let the existing model finish the access and any policy-generated PRE.
      // This is a bounded functional check, not a DRAM timing calibration.
      for(int cycle = 0; cycle < 80; ++cycle)
      {
         memory->tick();
      }
      assert(done);
   };
   auto close_requests = [&]()
   {
      if(!closed)
      {
         return 0;
      }
      YAML::Emitter output;
      output << YAML::BeginMap;
      dynamic_cast<Ramulator::Implementation*>(controller->m_rowpolicy)->print_stats(output);
      output << YAML::EndMap;
      return YAML::Load(output.c_str())["RowPolicy"]["num_close_reqs"].as<int>();
   };

   // Three accesses at cap=2 must close and then reopen each selected bank.
   for(const auto& address : addresses)
   {
      for(int access = 0; access < 3; ++access)
      {
         read(address);
      }
   }
   assert(close_requests() == (closed ? static_cast<int>(addresses.size()) : 0));

   // A rank-wide refresh must reset all bank counters, including implicit groups.
   for(int rank = 0; rank < ranks; ++rank)
   {
      Ramulator::AddrVec_t address(dram->m_levels.size(), -1);
      address[0] = 0;
      address[dram->m_levels("rank")] = rank;
      Ramulator::Request request(address, dram->m_requests("all-bank-refresh"));
      assert(controller->priority_send(request));
   }
   for(int cycle = 0; cycle < 1000; ++cycle)
   {
      memory->tick();
   }
   const auto before = close_requests();
   for(const auto& address : addresses)
   {
      read(address);
   }
   assert(close_requests() == before);
   assert(completed == addresses.size() * 4);
   memory->finalize();

   std::ifstream input(argv[2]);
   assert(input);
   std::map<std::string, unsigned> commands;
   std::string command;
   unsigned count;
   while(std::getline(input, command, ',') && input >> count)
   {
      commands[command] = count;
      input.ignore(1, '\n');
   }
   assert(commands["RD"] == completed);
   assert(commands["PRE"] == (closed ? addresses.size() : 0));
   assert(commands["ACT"] == addresses.size() * (closed ? 3 : 2));
   assert(commands["REFab"] == static_cast<unsigned>(ranks));
   std::cout << "PASS " << argv[3] << ": bank/rank isolation, PRE, reopen and refresh counter reset\n";
}
