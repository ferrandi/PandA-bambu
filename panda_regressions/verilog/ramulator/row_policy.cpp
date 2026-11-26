// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ramulator/base/config.h"
#include "ramulator/bambu/legacy_config.h"
#include "ramulator/controller/controller_base.h"
#include "ramulator/frontend/i_frontend.h"
#include "ramulator/memory_system/i_memory_system.h"

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
   config = Ramulator::Bambu::native_configuration(config);
   auto native_controller = config["memory_system"]["controllers"][0];
   native_controller["addr_mapper"]["impl"] = "PassThroughAddrMapper";
   YAML::Node counter;
   counter["impl"] = "CommandCounter";
   counter["path"] = argv[2];
   counter["commands_to_count"] = std::vector<std::string>{"ACT", "PREpb", "PREab", "RD", "RDA", "REFab"};
   native_controller["controller_plugins"].push_back(counter);
   auto factory_config = Ramulator::Bambu::factory_configuration(config);

   std::unique_ptr<Ramulator::IFrontEnd> frontend(Ramulator::Factory::create_frontend(factory_config));
   std::unique_ptr<Ramulator::IMemorySystem> memory(Ramulator::Factory::create_memory_system(factory_config));
   frontend->connect_memory_system(memory.get());
   memory->connect_frontend(frontend.get());
   auto* controller = memory->get_ifce<Ramulator::ControllerBase>();
   auto* dram = controller->m_device.m_spec;
   const bool grouped = dram->has_level("BankGroup");
   const int ranks = dram->get_level_size("Rank");
   const int groups = grouped ? dram->get_level_size("BankGroup") : 1;
   const int banks = dram->get_level_size("Bank");
   std::vector<Ramulator::AddrVec_t> addresses;
   for(int rank : ranks > 1 ? std::vector<int>{0, ranks - 1} : std::vector<int>{0})
   {
      for(int group : groups > 1 ? std::vector<int>{0, groups - 1} : std::vector<int>{0})
      {
         for(int bank : {0, banks - 1})
         {
            Ramulator::AddrVec_t address(dram->level_count, 0);
            address[dram->get_level_id("Rank")] = rank;
            address[dram->get_level_id("Bank")] = bank;
            if(grouped)
            {
               address[dram->get_level_id("BankGroup")] = group;
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
      request.addr = completed * 64;
      request.size_bytes = 8;
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
   // Three accesses at cap=2 must close and then reopen each selected bank.
   for(const auto& address : addresses)
   {
      for(int access = 0; access < 3; ++access)
      {
         read(address);
      }
   }


   // A rank-wide refresh must reset all bank counters, including implicit groups.
   for(int rank = 0; rank < ranks; ++rank)
   {
      Ramulator::AddrVec_t address(dram->level_count, -1);
      address[0] = 0;
      address[dram->get_level_id("Rank")] = rank;
      Ramulator::Request request(address, Ramulator::Request::Cmd, dram->get_command_id("REFab"));
      assert(controller->priority_send(request));
   }
   for(int cycle = 0; cycle < 1000; ++cycle)
   {
      memory->tick();
   }

   for(const auto& address : addresses)
   {
      read(address);
   }

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
   assert(commands["RD"] + commands["RDA"] == completed);
   assert(commands["PREpb"] + commands["RDA"] == (closed ? addresses.size() : 0));
   assert(commands["ACT"] == addresses.size() * (closed ? 3 : 2));
   assert(commands["REFab"] == static_cast<unsigned>(ranks));
   std::cout << "PASS " << argv[3] << ": bank/rank isolation, PRE, reopen and refresh counter reset\n";
}
