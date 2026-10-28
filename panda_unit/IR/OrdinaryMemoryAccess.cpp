// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "BambuParameter.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_node_dup.hpp"
#include "parse_ir.hpp"
#include "token_interface.hpp"

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace
{
ParameterConstRef make_parameter()
{
   static char program_name[] = "libbambu_unit";
   static char* argv[] = {program_name};
   auto parameter = std::make_shared<BambuParameter>(program_name, 1, argv);
   parameter->setOption(OPT_cc_m_env, std::string("-m64"));
   parameter->setOption(OPT_input_file, std::string("ordinary_memory_access.cpp"));
   return parameter;
}

ir_managerRef make_manager(const ParameterConstRef& parameter)
{
   return std::make_shared<ir_manager>(parameter);
}

std::filesystem::path write_raw_ir(const ir_manager& manager)
{
   std::ostringstream raw;
   manager.print(raw);
   const auto path = std::filesystem::temp_directory_path() /
                     ("ordinary_memory_access_" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".raw");
   std::ofstream output(path);
   output << "COMPILER_VERSION: \"unit\"\nPLUGIN_VERSION: \"unit\"\n" << raw.str();
   output.close();
   return path;
}
} // namespace

BOOST_AUTO_TEST_CASE(ordinary_memory_access_defaults_and_factory)
{
   auto parameter = make_parameter();
   auto manager = make_manager(parameter);

   const auto ordinary = std::make_shared<mem_access_node>(manager->new_ir_node_id());
   const auto unaligned = std::make_shared<unaligned_mem_access_node>(manager->new_ir_node_id());
   BOOST_CHECK(!ordinary->ordinary_memory_access);
   BOOST_CHECK(!unaligned->ordinary_memory_access);

   auto type = std::make_shared<integer_ty_node>(manager->new_ir_node_id());
   type->bitsize = 32;
   manager->AddIRNode(type);
   const auto schema = [&](const char* value) {
      return ir_manager::IRSchema{{TOK(TOK_TYPE), std::to_string(type->index)},
                                  {TOK(TOK_IR_LOCINFO), "unit:0:0"},
                                  {TOK(TOK_ORDINARY_MEMORY_ACCESS), value}};
   };
   const auto ordinary_true = GetPointerS<mem_access_node>(manager->create_ir_node(
       mem_access_node_K, schema("1")));
   const auto ordinary_false = GetPointerS<mem_access_node>(manager->create_ir_node(
       mem_access_node_K, schema("0")));
   const auto unaligned_true = GetPointerS<unaligned_mem_access_node>(manager->create_ir_node(
       unaligned_mem_access_node_K, schema("1")));
   const auto unaligned_false = GetPointerS<unaligned_mem_access_node>(manager->create_ir_node(
       unaligned_mem_access_node_K, schema("0")));
   BOOST_REQUIRE(ordinary_true);
   BOOST_REQUIRE(ordinary_false);
   BOOST_REQUIRE(unaligned_true);
   BOOST_REQUIRE(unaligned_false);
   BOOST_CHECK(ordinary_true->ordinary_memory_access);
   BOOST_CHECK(!ordinary_false->ordinary_memory_access);
   BOOST_CHECK(unaligned_true->ordinary_memory_access);
   BOOST_CHECK(!unaligned_false->ordinary_memory_access);
}

BOOST_AUTO_TEST_CASE(ordinary_memory_access_raw_round_trip)
{
   auto parameter = make_parameter();
   auto manager = make_manager(parameter);
   auto ordinary = std::make_shared<mem_access_node>(manager->new_ir_node_id());
   ordinary->ordinary_memory_access = true;
   manager->AddIRNode(ordinary);
   auto unaligned = std::make_shared<unaligned_mem_access_node>(manager->new_ir_node_id());
   manager->AddIRNode(unaligned);

   const auto raw_path = write_raw_ir(*manager);
   const auto parsed = ParseIRFile(parameter, raw_path.string());
   std::error_code ignored;
   std::filesystem::remove(raw_path, ignored);

   BOOST_REQUIRE(parsed);
   const auto parsed_ordinary = GetPointerS<const mem_access_node>(parsed->GetIRNode(ordinary->index));
   const auto parsed_unaligned = GetPointerS<const unaligned_mem_access_node>(parsed->GetIRNode(unaligned->index));
   BOOST_REQUIRE(parsed_ordinary);
   BOOST_REQUIRE(parsed_unaligned);
   BOOST_CHECK(parsed_ordinary->ordinary_memory_access);
   BOOST_CHECK(!parsed_unaligned->ordinary_memory_access);
}

BOOST_AUTO_TEST_CASE(ordinary_memory_access_duplication_and_ir_manager_merge)
{
   auto parameter = make_parameter();
   const auto application = std::make_shared<application_manager>(false, parameter);
   const auto manager = application->get_ir_manager();
   auto original = std::make_shared<mem_access_node>(manager->new_ir_node_id());
   original->ordinary_memory_access = true;
   manager->AddIRNode(original);

   CustomUnorderedMapStable<unsigned int, unsigned int> duplicate_remap;
   ir_node_dup duplicator(duplicate_remap, application);
   const auto duplicate_id = duplicator.create_ir_node(original);
   const auto duplicate = GetPointerS<const mem_access_node>(manager->GetIRNode(duplicate_id));
   BOOST_REQUIRE(duplicate);
   BOOST_CHECK(duplicate->ordinary_memory_access);

   const auto source_application = std::make_shared<application_manager>(false, parameter);
   const auto source = source_application->get_ir_manager();
   const auto function_name = source->create_ir_node(
       identifier_node_K, {{TOK(TOK_STRG), "ordinary_memory_access_merge_test"}});
   auto function = std::make_shared<function_val_node>(source->new_ir_node_id());
   function->name = function_name;
   source->AddIRNode(function);
   auto access = std::make_shared<mem_access_node>(source->new_ir_node_id());
   access->ordinary_memory_access = true;
   source->AddIRNode(access);
   auto assignment = std::make_shared<assign_stmt>(source->new_ir_node_id());
   assignment->op1 = access;
   source->AddIRNode(assignment);
   auto body = std::make_shared<statement_list_node>(source->new_ir_node_id());
   auto block = std::make_shared<bloc>(1);
   block->PushBack(assignment, source_application);
   body->add_bloc(block);
   source->AddIRNode(body);
   function->body = body;

   manager->merge_ir_managers(source);
   bool found_merged_access = false;
   for(unsigned int index = 1; index < manager->get_next_available_ir_node_id(); ++index)
   {
      const auto node = manager->GetIRNode(index);
      if(node && node->get_kind() == mem_access_node_K)
      {
         found_merged_access = true;
         BOOST_CHECK(GetPointerS<const mem_access_node>(node)->ordinary_memory_access);
      }
   }
   BOOST_REQUIRE(found_merged_access);
}
