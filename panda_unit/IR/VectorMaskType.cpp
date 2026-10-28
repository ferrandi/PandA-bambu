// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_node_finder.hpp"
#include "ir_nodes_merger.hpp"
#include "raw_writer.hpp"
#include "token_interface.hpp"
#include "var_pp_functor.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

extern ir_managerRef ir_parseY(const ParameterConstRef parameters, std::string filename);

namespace
{
   class VectorMaskParameters : public Parameter
   {
    public:
      VectorMaskParameters() : Parameter("vector_mask_type", 0, nullptr)
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
} // namespace

BOOST_AUTO_TEST_CASE(vector_mask_type_has_logical_lanes)
{
   auto element = std::make_shared<integer_ty_node>(1);
   element->bitsize = 1;
   element->bitsizealloc = 8;
   element->algn = 8;

   auto two_lanes = std::make_shared<vector_ty_node>(2);
   two_lanes->elts = element;
   two_lanes->nelements = 2;
   two_lanes->bitsizealloc = 8;

   auto eight_lanes = std::make_shared<vector_ty_node>(3);
   eight_lanes->elts = element;
   eight_lanes->nelements = 8;
   eight_lanes->bitsizealloc = 8;

   BOOST_CHECK_EQUAL(ir_helper::SizeAlloc(two_lanes), 8);
   BOOST_CHECK_EQUAL(ir_helper::SizeAlloc(eight_lanes), 8);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(two_lanes), 2);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(eight_lanes), 8);
   BOOST_CHECK_EQUAL(ir_helper::VectorElementBitsize(two_lanes), 1);
   BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(two_lanes), 2);
   BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(eight_lanes), 8);
   BOOST_CHECK(two_lanes->ToString().find("<2 x ") == 0);
   BOOST_CHECK(eight_lanes->ToString().find("<8 x ") == 0);

   auto sixteen_lanes = std::make_shared<vector_ty_node>(4);
   sixteen_lanes->elts = element;
   sixteen_lanes->nelements = 16;
   sixteen_lanes->bitsizealloc = 16;
   BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(sixteen_lanes), 16);
   BOOST_CHECK(sixteen_lanes->ToString().find("<16 x ") == 0);

   std::map<IRVocabularyTokenTypes_TokenEnum, std::string> schema;
   schema[TOK(TOK_NELEMENTS)] = "2";
   ir_node_finder finder(schema);
   BOOST_CHECK(finder.check(two_lanes));
   BOOST_CHECK(!finder.check(eight_lanes));
   BOOST_CHECK(!finder.check(sixteen_lanes));

   std::ostringstream stream;
   raw_writer writer(stream);
   two_lanes->visit(&writer);
   BOOST_CHECK(stream.str().find("nelements: 2") != std::string::npos);

   auto word = std::make_shared<integer_ty_node>(5);
   word->bitsize = 32;
   word->bitsizealloc = 32;
   auto padded = std::make_shared<vector_ty_node>(6);
   padded->elts = word;
   padded->nelements = 3;
   padded->bitsizealloc = 128;
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(padded), 3);
   BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(padded), 96);
   BOOST_CHECK_EQUAL(ir_helper::SizeAlloc(padded), 128);

   const auto parameters = std::make_shared<VectorMaskParameters>();
   const auto manager = std::make_shared<ir_manager>(parameters);
   manager->AddIRNode(element);
   ir_manager::IRSchema vector_schema;
   vector_schema[TOK(TOK_ELTS)] = "1";
   vector_schema[TOK(TOK_BITSIZEALLOC)] = "8";
   vector_schema[TOK(TOK_NELEMENTS)] = "2";
   const auto created_two = manager->create_ir_node(vector_ty_node_K, vector_schema);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(created_two), 2);
   vector_schema[TOK(TOK_NELEMENTS)] = "8";
   const auto created_eight = manager->create_ir_node(vector_ty_node_K, vector_schema);
   BOOST_CHECK_NE(created_two->index, created_eight->index);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(created_eight), 8);

   const auto copied_manager = std::make_shared<ir_manager>(parameters);
   copied_manager->AddIRNode(element);
   CustomUnorderedMapUnstable<unsigned int, unsigned int> remap;
   remap[element->index] = element->index;
   ir_node_index_factory copier(remap, copied_manager);
   copier.create_ir_node(10, created_two);
   copier.create_ir_node(11, created_eight);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(copied_manager->GetIRNode(10)), 2);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(copied_manager->GetIRNode(11)), 8);
}

BOOST_AUTO_TEST_CASE(vector_mask_constant_keeps_lane_bits)
{
   auto element = std::make_shared<integer_ty_node>(1);
   element->bitsize = 1;
   element->bitsizealloc = 8;
   element->algn = 8;
   element->unsigned_flag = true;
   auto type = std::make_shared<vector_ty_node>(2);
   type->elts = element;
   type->nelements = 4;
   // Older LLVM targets allocate four bytes for this four-bit vector.
   type->bitsizealloc = 32;
   auto one = std::make_shared<constant_int_val_node>(3);
   one->type = element;
   one->value = 1;
   auto zero = std::make_shared<constant_int_val_node>(4);
   zero->type = element;
   zero->value = 0;
   auto mask = std::make_shared<constant_vector_val_node>(5);
   mask->type = type;
   mask->list_of_valu = {one, one, zero, one};
   BOOST_CHECK(ir_helper::NodeRange(mask).getUnsignedMax() == 11);
   BOOST_CHECK(ir_helper::NodeRange(mask).getUnsignedMin() == 11);
   type->bitsizealloc = 8;
   BOOST_CHECK(ir_helper::NodeRange(mask).getUnsignedMax() == 11);
}

BOOST_AUTO_TEST_CASE(vector_mask_shuffle_pretty_print_keeps_result_lanes)
{
   struct Names : var_pp_functor
   {
      std::string operator()(unsigned int index) const override
      {
         return index == 5 ? "a" : index == 6 ? "b" : "mask";
      }
   };
   const auto parameters = std::make_shared<VectorMaskParameters>();
   parameters->setOption(OPT_input_file, "");
   parameters->setOption(OPT_addr_bus_bitsize, static_cast<unsigned int>(sizeof(void*) * 8U));
   const auto application = std::make_shared<application_manager>(false, parameters);
   const BehavioralHelper helper(application, 0, parameters);
   const std::unique_ptr<var_pp_functor> names = std::make_unique<Names>();
   auto element = std::make_shared<integer_ty_node>(1);
   element->bitsize = element->bitsizealloc = element->algn = 32;

   // A three-lane input has a four-lane C carrier. Its logical lane count
   // still defines the boundary between the two shuffle inputs.
   for(const auto& counts : {std::make_pair(3U, 4U), std::make_pair(4U, 2U), std::make_pair(3U, 3U)})
   {
      auto input = std::make_shared<vector_ty_node>(2);
      input->elts = element;
      input->nelements = counts.first;
      input->bitsizealloc = input->algn = 128;
      auto output = std::make_shared<vector_ty_node>(3);
      output->elts = element;
      output->nelements = counts.second;
      output->bitsizealloc = output->algn = counts.second == 2 ? 64 : 128;
      auto a = std::make_shared<ssa_node>(5);
      auto b = std::make_shared<ssa_node>(6);
      auto mask = std::make_shared<ssa_node>(7);
      a->type = b->type = input;
      mask->type = output;
      auto shuffle = std::make_shared<shufflevector_node>(8);
      shuffle->type = output;
      shuffle->op0 = a;
      shuffle->op1 = b;
      shuffle->op2 = mask;
      const auto printed = helper.PrintNode(shuffle, names);
      BOOST_TEST_CONTEXT("shuffle " << counts.first << " -> " << counts.second << ": " << printed)
      {
         const auto start = printed.find('{');
         BOOST_REQUIRE(start != std::string::npos);
         const auto initializer = printed.substr(start);
         BOOST_CHECK_EQUAL(std::count(initializer.begin(), initializer.end(), ','), counts.second - 1);
         BOOST_CHECK(printed.find("(mask)[" + std::to_string(counts.second - 1) + "]") != std::string::npos);
         BOOST_CHECK(printed.find("(mask)[" + std::to_string(counts.second) + "]") == std::string::npos);
         BOOST_CHECK(printed.find(")%" + std::to_string(2 * counts.first) + ")") != std::string::npos);
         BOOST_CHECK(printed.find(" < " + std::to_string(counts.first) + ")") != std::string::npos);
         BOOST_CHECK(printed.find(")-" + std::to_string(counts.first) + "]") != std::string::npos);
      }
   }
}

BOOST_AUTO_TEST_CASE(vector_mask_parser_round_trip)
{
   const auto parameters = std::make_shared<VectorMaskParameters>();
   const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
   const auto directory =
       root / "documentation" / "tmp" /
       ("vector-mask-unit-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
   std::filesystem::create_directories(directory);
   struct Cleanup
   {
      std::filesystem::path directory;
      ~Cleanup()
      {
         std::error_code error;
         std::filesystem::remove_all(directory, error);
      }
   } cleanup{directory};
   const auto input = directory / "input.bambuir";
   {
      std::ofstream stream(input);
      stream << "COMPILER_VERSION: \"unit\"\nPLUGIN_VERSION: \"unit\"\nNODE_COUNT: 9\n"
                "@1 integer_ty_node bitsizealloc: 8 algn: 8 bitsize: 1 unsigned\n"
                "@2 vector_ty_node bitsizealloc: 8 algn: 8 elts: @1 nelements: 2\n"
                "@3 vector_ty_node bitsizealloc: 8 algn: 8 elts: @1 nelements: 8\n"
                "@4 vector_ty_node bitsizealloc: 16 algn: 16 elts: @1 nelements: 16\n"
                "@5 constant_int_val_node type: @1 value: 1\n"
                "@6 constant_int_val_node type: @1 value: 0\n"
                "@7 constant_vector_val_node type: @2 valu: @5 valu: @6\n"
                "@8 integer_ty_node bitsizealloc: 32 algn: 32 bitsize: 32 unsigned\n"
                "@9 vector_ty_node bitsizealloc: 128 algn: 128 elts: @8 nelements: 3\n";
   }
   const auto parsed = ir_parseY(parameters, input.string());
   const auto output = directory / "output.bambuir";
   {
      std::ofstream stream(output);
      parsed->print(stream);
   }
   const auto reparsed = ir_parseY(parameters, output.string());
   for(const auto& manager : {parsed, reparsed})
   {
      BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(manager->GetIRNode(2)), 2);
      BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(manager->GetIRNode(3)), 8);
      BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(manager->GetIRNode(4)), 16);
      BOOST_CHECK(ir_helper::NodeRange(manager->GetIRNode(7)).getUnsignedMax() == 1);
      BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(manager->GetIRNode(9)), 3);
      BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(manager->GetIRNode(9)), 96);
      BOOST_CHECK_EQUAL(ir_helper::SizeAlloc(manager->GetIRNode(9)), 128);
   }
   const auto missing_count = [](const std::string& message) {
      return message.find("Vector type without nelements") != std::string::npos;
   };
   for(const std::string suffix : {"", " nelements: 0"})
   {
      const auto invalid = directory / "invalid.bambuir";
      {
         std::ofstream stream(invalid);
         stream << "COMPILER_VERSION: \"unit\"\nPLUGIN_VERSION: \"unit\"\nNODE_COUNT: 2\n"
                   "@1 integer_ty_node bitsizealloc: 8 algn: 8 bitsize: 1 unsigned\n"
                   "@2 vector_ty_node bitsizealloc: 8 algn: 8 elts: @1"
                << suffix << '\n';
      }
      BOOST_CHECK_EXCEPTION(ir_parseY(parameters, invalid.string()), std::string, missing_count);
   }
}

BOOST_AUTO_TEST_CASE(vector_mask_factory_rejects_missing_count)
{
   const auto parameters = std::make_shared<VectorMaskParameters>();
   const auto invalid_count = [](const std::string& message) {
      return message.find("Vector type without nelements") != std::string::npos;
   };
   for(const bool include_zero : {false, true})
   {
      const auto manager = std::make_shared<ir_manager>(parameters);
      ir_manager::IRSchema schema;
      if(include_zero)
      {
         schema[TOK(TOK_NELEMENTS)] = "0";
      }
      BOOST_CHECK_EXCEPTION(manager->create_ir_node(vector_ty_node_K, schema), std::string, invalid_count);
   }
}
