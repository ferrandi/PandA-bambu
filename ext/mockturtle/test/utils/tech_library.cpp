#include <catch.hpp>

#include <cstdint>
#include <vector>

#include <lorina/genlib.hpp>
#include <lorina/super.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/super_reader.hpp>
#include <mockturtle/utils/super_utils.hpp>
#include <mockturtle/utils/tech_library.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/static_truth_table.hpp>

using namespace mockturtle;

std::string const simple_test_library = "GATE   inv1    1 O=!a;     PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                        "GATE   inv2    2 O=!a;     PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                        "GATE   buf     2 O=a;      PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                        "GATE   nand2   2 O=!(a*b); PIN * INV 1 999 1.0 0.2 1.0 0.2\n";

std::string const simple_library = "GATE zero 0 O=0;\n"
                                   "GATE one 0 O=1;\n"
                                   "GATE inverter 1 O=!a; PIN * INV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE buffer 2 O=a; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE and 5 O=a*b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE or 5 O=a+b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n";

std::string const super_library = "simple.genlib\n"
                                  "3\n"
                                  "7\n"
                                  "13\n"
                                  "* and 1 0\n"
                                  "* and 2 3\n"
                                  "and 2 0\n"
                                  "* and 1 5\n"
                                  "or 2 1\n"
                                  "* and 0 7\n"
                                  "* or 1 0\n"
                                  "or 2 0\n"
                                  "* and 1 10\n"
                                  "* and 2 9\n"
                                  "\0";

std::string const test_library = "GATE   inv1    3 O=!a;               PIN * INV 3 999 1.1 0.09 1.1 0.09\n"
                                 "GATE   inv2    2 O=!a;               PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                 "GATE   inv3    1 O=!a;               PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                 "GATE   inv4    4 O=!a;               PIN * INV 4 999 1.2 0.07 1.2 0.07\n"
                                 "GATE   nand2   2 O=!(a*b);           PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
                                 "GATE   nand3   3 O=!(a*b*c);         PIN * INV 1 999 1.1 0.3 1.1 0.3\n"
                                 "GATE   nand4   4 O=!(a*b*c*d);       PIN * INV 1 999 1.4 0.4 1.4 0.4\n"
                                 "GATE   nor2    2 O=!(a+b);           PIN * INV 1 999 1.4 0.5 1.4 0.5\n"
                                 "GATE   nor3    3 O=!(a+b+c);         PIN * INV 1 999 2.4 0.7 2.4 0.7\n"
                                 "GATE   nor4    4 O=!(a+b+c+d);       PIN * INV 1 999 3.8 1.0 3.8 1.0\n"
                                 "GATE   and2    3 O=a*b;              PIN * NONINV 1 999 1.9 0.3 1.9 0.3\n"
                                 "GATE   or2     3 O=a+b;              PIN * NONINV 1 999 2.4 0.3 2.4 0.3\n"
                                 "GATE   xor2a   5 O=a*!b+!a*b;        PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "#GATE  xor2b   5 O=!(a*b+!a*!b);     PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "GATE   xnor2a  5 O=a*b+!a*!b;        PIN * UNKNOWN 2 999 2.1 0.5 2.1 0.5\n"
                                 "#GATE  xnor2b  5 O=!(a*!b+!a*b);     PIN * UNKNOWN 2 999 2.1 0.5 2.1 0.5\n"
                                 "GATE   aoi21   3 O=!(a*b+c);         PIN * INV 1 999 1.6 0.4 1.6 0.4\n"
                                 "GATE   aoi22   4 O=!(a*b+c*d);       PIN * INV 1 999 2.0 0.4 2.0 0.4\n"
                                 "GATE   oai21   3 O=!((a+b)*c);       PIN * INV 1 999 1.6 0.4 1.6 0.4\n"
                                 "GATE   oai22   4 O=!((a+b)*(c+d));   PIN * INV 1 999 2.0 0.4 2.0 0.4\n"
                                 "GATE   buf     2 O=a;                PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                 "GATE   zero    0 O=CONST0;\n"
                                 "GATE   one     0 O=CONST1;";

TEST_CASE( "Simple test library generation 1", "[tech_library]" )
{
  std::vector<gate> gates;

  std::istringstream in( simple_test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  tech_library<2, classification_type::np_configurations> lib( gates );

  CHECK( lib.max_gate_size() == 2 );
  CHECK( lib.get_inverter_info() == std::make_tuple( 1.0f, 0.9f, 0u ) );
  CHECK( lib.get_buffer_info() == std::make_tuple( 2.0f, 1.0f, 2u ) );

  kitty::static_truth_table<2> tt;

  kitty::create_from_hex_string( tt, "5" );
  auto const inv = lib.get_supergates( tt );
  CHECK( inv != nullptr );
  CHECK( inv->size() == 2 );
  CHECK( ( *inv )[0].root->root->name == "inv1" );
  CHECK( ( *inv )[0].area == 1.0f );
  CHECK( ( *inv )[0].tdelay[0] == 0.9f );
  CHECK( ( *inv )[0].polarity == 0u );
  CHECK( ( *inv )[1].root->root->name == "inv2" );
  CHECK( ( *inv )[1].area == 2.0f );
  CHECK( ( *inv )[1].tdelay[0] == 1.0f );
  CHECK( ( *inv )[1].polarity == 0u );

  kitty::create_from_hex_string( tt, "7" );
  auto const nand_7 = lib.get_supergates( tt );
  CHECK( nand_7 != nullptr );
  CHECK( nand_7->size() == 1 );
  CHECK( ( *nand_7 )[0].root->root->name == "nand2" );
  CHECK( ( *nand_7 )[0].area == 2.0f );
  CHECK( ( *nand_7 )[0].tdelay[0] == 1.0f );
  CHECK( ( *nand_7 )[0].tdelay[1] == 1.0f );
  CHECK( ( *nand_7 )[0].polarity == 0u );

  kitty::create_from_hex_string( tt, "b" );
  auto const nand_b = lib.get_supergates( tt );
  CHECK( nand_b != nullptr );
  CHECK( nand_b->size() == 1 );
  CHECK( ( *nand_b )[0].root->root->name == "nand2" );
  CHECK( ( *nand_b )[0].area == 2.0f );
  CHECK( ( *nand_b )[0].tdelay[0] == 1.0f );
  CHECK( ( *nand_b )[0].tdelay[1] == 1.0f );
  CHECK( ( *nand_b )[0].polarity == 1u );

  kitty::create_from_hex_string( tt, "d" );
  auto const nand_d = lib.get_supergates( tt );
  CHECK( nand_d != nullptr );
  CHECK( nand_d->size() == 1 );
  CHECK( ( *nand_d )[0].root->root->name == "nand2" );
  CHECK( ( *nand_d )[0].area == 2.0f );
  CHECK( ( *nand_d )[0].tdelay[0] == 1.0f );
  CHECK( ( *nand_d )[0].tdelay[1] == 1.0f );
  CHECK( ( *nand_d )[0].polarity == 2u );

  kitty::create_from_hex_string( tt, "e" );
  auto const nand_e = lib.get_supergates( tt );
  CHECK( nand_e != nullptr );
  CHECK( nand_e->size() == 1 );
  CHECK( ( *nand_e )[0].root->root->name == "nand2" );
  CHECK( ( *nand_e )[0].area == 2.0f );
  CHECK( ( *nand_e )[0].tdelay[0] == 1.0f );
  CHECK( ( *nand_e )[0].tdelay[1] == 1.0f );
  CHECK( ( *nand_e )[0].polarity == 3u );
}

TEST_CASE( "Simple test library generation 2", "[tech_library]" )
{
  std::vector<gate> gates;

  std::istringstream in( simple_test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  tech_library<2, classification_type::p_configurations> lib( gates );

  CHECK( lib.max_gate_size() == 2 );
  CHECK( lib.get_inverter_info() == std::make_tuple( 1.0f, 0.9f, 0u ) );
  CHECK( lib.get_buffer_info() == std::make_tuple( 2.0f, 1.0f, 2u ) );

  kitty::static_truth_table<2> tt;

  kitty::create_from_hex_string( tt, "5" );
  auto const inv = lib.get_supergates( tt );
  CHECK( inv != nullptr );
  CHECK( inv->size() == 2 );
  CHECK( ( *inv )[0].root->root->name == "inv1" );
  CHECK( ( *inv )[0].area == 1.0f );
  CHECK( ( *inv )[0].tdelay[0] == 0.9f );
  CHECK( ( *inv )[0].polarity == 0u );
  CHECK( ( *inv )[1].root->root->name == "inv2" );
  CHECK( ( *inv )[1].area == 2.0f );
  CHECK( ( *inv )[1].tdelay[0] == 1.0f );
  CHECK( ( *inv )[1].polarity == 0u );

  kitty::create_from_hex_string( tt, "7" );
  auto const nand_7 = lib.get_supergates( tt );
  CHECK( nand_7 != nullptr );
  CHECK( nand_7->size() == 1 );
  CHECK( ( *nand_7 )[0].root->root->name == "nand2" );
  CHECK( ( *nand_7 )[0].area == 2.0f );
  CHECK( ( *nand_7 )[0].tdelay[0] == 1.0f );
  CHECK( ( *nand_7 )[0].tdelay[1] == 1.0f );
  CHECK( ( *nand_7 )[0].polarity == 0u );

  kitty::create_from_hex_string( tt, "b" );
  auto const nand_b = lib.get_supergates( tt );
  CHECK( nand_b == nullptr );

  kitty::create_from_hex_string( tt, "d" );
  auto const nand_d = lib.get_supergates( tt );
  CHECK( nand_d == nullptr );

  kitty::create_from_hex_string( tt, "e" );
  auto const nand_e = lib.get_supergates( tt );
  CHECK( nand_e == nullptr );
}

TEST_CASE( "Supergate library generation P", "[tech_library]" )
{
  std::vector<gate> gates;
  super_lib super_data;

  std::istringstream in_genlib( simple_library );
  auto result = lorina::read_genlib( in_genlib, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  std::istringstream in_super( super_library );
  result = lorina::read_super( in_super, mockturtle::super_reader( super_data ) );

  CHECK( result == lorina::return_code::success );

  tech_library_params ps;
  ps.verbose = true;
  ps.very_verbose = true;
  tech_library<3, classification_type::p_configurations> lib( gates, super_data );
  fflush( stdout );

  CHECK( lib.max_gate_size() == 3 );
  CHECK( lib.get_inverter_info() == std::make_tuple( 1.0f, 1.0f, 2u ) );
  CHECK( lib.get_buffer_info() == std::make_tuple( 2.0f, 1.0f, 3u ) );

  kitty::static_truth_table<3> tt;

  kitty::create_from_hex_string( tt, "55" );
  auto const inv = lib.get_supergates( tt );
  CHECK( inv != nullptr );
  CHECK( inv->size() == 1 );
  CHECK( ( *inv )[0].root->root->name == "inverter" );
  CHECK( ( *inv )[0].area == 1.0f );
  CHECK( ( *inv )[0].tdelay[0] == 1.0f );
  CHECK( ( *inv )[0].polarity == 0u );

  kitty::create_from_hex_string( tt, "11" );
  auto const and_1 = lib.get_supergates( tt );
  CHECK( and_1 != nullptr );
  CHECK( and_1->size() == 1 );
  CHECK( ( *and_1 )[0].root->root->name == "and" );
  CHECK( ( *and_1 )[0].area == 5.0f );
  CHECK( ( *and_1 )[0].tdelay[0] == 1.0f );
  CHECK( ( *and_1 )[0].tdelay[1] == 1.0f );
  CHECK( ( *and_1 )[0].polarity == 3u );

  kitty::create_from_hex_string( tt, "22" );
  auto const and_8 = lib.get_supergates( tt );
  CHECK( and_8 == nullptr );

  kitty::create_from_hex_string( tt, "44" );
  auto const nand_d = lib.get_supergates( tt );
  CHECK( nand_d == nullptr );

  kitty::create_from_hex_string( tt, "88" );
  auto const nand_e = lib.get_supergates( tt );
  CHECK( nand_e == nullptr );

  kitty::create_from_hex_string( tt, "07" );
  auto const andor_07 = lib.get_supergates( tt );
  CHECK( andor_07 != nullptr );
  CHECK( andor_07->size() == 1 );
  CHECK( ( *andor_07 )[0].root->root->name == "and" );
  CHECK( ( *andor_07 )[0].area == 10.0f );
  CHECK( ( *andor_07 )[0].tdelay[0] == 2.0f );
  CHECK( ( *andor_07 )[0].tdelay[1] == 2.0f );
  CHECK( ( *andor_07 )[0].tdelay[2] == 1.0f );
  CHECK( ( *andor_07 )[0].polarity == 7u );

  kitty::create_from_hex_string( tt, "01" );
  auto const and_01 = lib.get_supergates( tt );
  CHECK( and_01 != nullptr );
  CHECK( and_01->size() == 2 );
  CHECK( ( *and_01 )[0].root->root->name == "and" );
  CHECK( ( *and_01 )[0].area == 10.0f );
  CHECK( ( *and_01 )[0].tdelay[0] == 2.0f );
  CHECK( ( *and_01 )[0].tdelay[1] == 2.0f );
  CHECK( ( *and_01 )[0].tdelay[2] == 1.0f );
  CHECK( ( *and_01 )[0].polarity == 7u );
  CHECK( ( *and_01 )[1].root->root->name == "and" );
  CHECK( ( *and_01 )[1].area == 10.0f );
  CHECK( ( *and_01 )[1].tdelay[0] == 2.0f );
  CHECK( ( *and_01 )[1].tdelay[1] == 1.0f );
  CHECK( ( *and_01 )[1].tdelay[2] == 2.0f );
  CHECK( ( *and_01 )[1].polarity == 7u );
}

TEST_CASE( "Supergate library generation NP", "[tech_library]" )
{
  std::vector<gate> gates;
  super_lib super_data;

  std::istringstream in_genlib( simple_library );
  auto result = lorina::read_genlib( in_genlib, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  std::istringstream in_super( super_library );
  result = lorina::read_super( in_super, mockturtle::super_reader( super_data ) );

  CHECK( result == lorina::return_code::success );

  tech_library_params ps;
  ps.verbose = true;
  ps.very_verbose = true;
  tech_library<3, classification_type::np_configurations> lib( gates, super_data );
  fflush( stdout );

  CHECK( lib.max_gate_size() == 3 );
  CHECK( lib.get_inverter_info() == std::make_tuple( 1.0f, 1.0f, 2u ) );
  CHECK( lib.get_buffer_info() == std::make_tuple( 2.0f, 1.0f, 3u ) );

  kitty::static_truth_table<3> tt;

  kitty::create_from_hex_string( tt, "11" );
  auto const and_1 = lib.get_supergates( tt );
  CHECK( and_1 != nullptr );
  CHECK( and_1->size() == 1 );
  CHECK( ( *and_1 )[0].root->root->name == "and" );
  CHECK( ( *and_1 )[0].area == 5.0f );
  CHECK( ( *and_1 )[0].tdelay[0] == 1.0f );
  CHECK( ( *and_1 )[0].tdelay[1] == 1.0f );
  CHECK( ( *and_1 )[0].polarity == 3u );

  kitty::create_from_hex_string( tt, "22" );
  auto const and_2 = lib.get_supergates( tt );
  CHECK( and_2 != nullptr );
  CHECK( and_2->size() == 1 );
  CHECK( ( *and_2 )[0].root->root->name == "and" );
  CHECK( ( *and_2 )[0].area == 5.0f );
  CHECK( ( *and_2 )[0].tdelay[0] == 1.0f );
  CHECK( ( *and_2 )[0].tdelay[1] == 1.0f );
  CHECK( ( *and_2 )[0].polarity == 2u );

  kitty::create_from_hex_string( tt, "44" );
  auto const and_4 = lib.get_supergates( tt );
  CHECK( and_4 != nullptr );
  CHECK( and_4->size() == 1 );
  CHECK( ( *and_4 )[0].root->root->name == "and" );
  CHECK( ( *and_4 )[0].area == 5.0f );
  CHECK( ( *and_4 )[0].tdelay[0] == 1.0f );
  CHECK( ( *and_4 )[0].tdelay[1] == 1.0f );
  CHECK( ( *and_4 )[0].polarity == 1u );

  kitty::create_from_hex_string( tt, "88" );
  auto const and_8 = lib.get_supergates( tt );
  CHECK( and_8 != nullptr );
  CHECK( and_8->size() == 1 );
  CHECK( ( *and_8 )[0].root->root->name == "and" );
  CHECK( ( *and_8 )[0].area == 5.0f );
  CHECK( ( *and_8 )[0].tdelay[0] == 1.0f );
  CHECK( ( *and_8 )[0].tdelay[1] == 1.0f );
  CHECK( ( *and_8 )[0].polarity == 0u );

  kitty::create_from_hex_string( tt, "07" );
  auto const andor_07 = lib.get_supergates( tt );
  CHECK( andor_07 != nullptr );
  CHECK( andor_07->size() == 1 );
  CHECK( ( *andor_07 )[0].root->root->name == "and" );
  CHECK( ( *andor_07 )[0].area == 10.0f );
  CHECK( ( *andor_07 )[0].tdelay[0] == 2.0f );
  CHECK( ( *andor_07 )[0].tdelay[1] == 2.0f );
  CHECK( ( *andor_07 )[0].tdelay[2] == 1.0f );
  CHECK( ( *andor_07 )[0].polarity == 7u );

  kitty::create_from_hex_string( tt, "e0" );
  auto const andor_e0 = lib.get_supergates( tt );
  CHECK( andor_e0 != nullptr );
  CHECK( andor_e0->size() == 1 );
  CHECK( ( *andor_e0 )[0].root->root->name == "and" );
  CHECK( ( *andor_e0 )[0].area == 10.0f );
  CHECK( ( *andor_e0 )[0].tdelay[0] == 2.0f );
  CHECK( ( *andor_e0 )[0].tdelay[1] == 2.0f );
  CHECK( ( *andor_e0 )[0].tdelay[2] == 1.0f );
  CHECK( ( *andor_e0 )[0].polarity == 0u );
}

TEST_CASE( "Complete library generation", "[tech_library]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  tech_library<4, classification_type::np_configurations> lib( gates );

  CHECK( lib.max_gate_size() == 4 );
  CHECK( lib.get_inverter_info() == std::make_tuple( 1.0f, 0.9f, 2u ) );

  for ( auto const& gate : gates )
  {
    auto const tt = gate.function;

    const auto test_enumeration = [&]( auto const& tt, auto, auto ) {
      const auto static_tt = kitty::extend_to<4>( tt );

      auto const supergates = lib.get_supergates( static_tt );

      CHECK( supergates != nullptr );

      bool found = false;
      for ( auto const& supergate : *supergates )
      {
        if ( supergate.root->id == gate.id )
        {
          found = true;
          break;
        }
      }

      CHECK( found == true );
    };

    kitty::exact_np_enumeration( tt, test_enumeration );
  }
}