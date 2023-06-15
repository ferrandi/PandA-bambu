#include <catch.hpp>

#include <cstdint>
#include <vector>

#include <lorina/genlib.hpp>
#include <lorina/super.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/utils/super_utils.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/static_truth_table.hpp>

using namespace mockturtle;

std::string const genlib_library = "GATE zero 0 O=0;\n"
                                   "GATE one 0 O=1;\n"
                                   "GATE inverter 1 O=!a; PIN * INV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE buffer 2 O=a; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE and 5 O=a*b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE or 5 O=a+b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n";

std::string const super_library = "test.genlib\n"
                                  "3\n"
                                  "8\n"
                                  "14\n"
                                  "* and 1 0\n"
                                  "* and 2 3\n"
                                  "and 2 0\n"
                                  "* and 1 5\n"
                                  "or 2 1\n"
                                  "* and 0 7\n"
                                  "* or 1 0\n"
                                  "* and 0 9\n"
                                  "or 2 0\n"
                                  "* and 1 11\n"
                                  "* and 2 9\n"
                                  "\0";

TEST_CASE( "Standard gates super library", "[super_utils]" )
{
  std::vector<gate> gates;

  std::istringstream in_genlib( genlib_library );
  auto result = lorina::read_genlib( in_genlib, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  super_utils<3> super( gates );

  auto const& lib = super.get_super_library();
  CHECK( lib.size() == 6 );

  CHECK( lib[0].id == 0 );
  CHECK( lib[0].is_super == false );
  CHECK( lib[0].root == &gates[0] );
  CHECK( lib[0].num_vars == 0 );
  CHECK( lib[0].function == gates[0].function );
  CHECK( lib[0].tdelay[0] == 0 );
  CHECK( lib[0].fanin.size() == 0 );

  CHECK( lib[1].id == 1 );
  CHECK( lib[1].is_super == false );
  CHECK( lib[1].root == &gates[1] );
  CHECK( lib[1].num_vars == 0 );
  CHECK( lib[1].function == gates[1].function );
  CHECK( lib[1].tdelay[0] == 0 );
  CHECK( lib[1].fanin.size() == 0 );

  CHECK( lib[2].id == 2 );
  CHECK( lib[2].is_super == false );
  CHECK( lib[2].root == &gates[2] );
  CHECK( lib[2].num_vars == 1 );
  CHECK( lib[2].function == gates[2].function );
  CHECK( lib[2].tdelay[0] == 1 );
  CHECK( lib[3].tdelay[1] == 0 );
  CHECK( lib[2].fanin.size() == 0 );

  CHECK( lib[3].id == 3 );
  CHECK( lib[3].is_super == false );
  CHECK( lib[3].root == &gates[3] );
  CHECK( lib[3].num_vars == 1 );
  CHECK( lib[3].function == gates[3].function );
  CHECK( lib[3].tdelay[0] == 1 );
  CHECK( lib[3].tdelay[1] == 0 );
  CHECK( lib[3].fanin.size() == 0 );

  CHECK( lib[4].id == 4 );
  CHECK( lib[4].is_super == false );
  CHECK( lib[4].root == &gates[4] );
  CHECK( lib[4].num_vars == 2 );
  CHECK( lib[4].function == gates[4].function );
  CHECK( lib[4].tdelay[0] == 1 );
  CHECK( lib[4].tdelay[1] == 1 );
  CHECK( lib[4].fanin.size() == 0 );

  CHECK( lib[5].id == 5 );
  CHECK( lib[5].is_super == false );
  CHECK( lib[5].root == &gates[5] );
  CHECK( lib[5].num_vars == 2 );
  CHECK( lib[5].function == gates[5].function );
  CHECK( lib[5].tdelay[0] == 1 );
  CHECK( lib[5].tdelay[1] == 1 );
  CHECK( lib[5].fanin.size() == 0 );
}

TEST_CASE( "Supergates super library", "[super_utils]" )
{
  std::vector<gate> gates;
  super_lib super_data;

  std::istringstream in_genlib( genlib_library );
  auto result = lorina::read_genlib( in_genlib, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  std::istringstream in_super( super_library );
  result = lorina::read_super( in_super, mockturtle::super_reader( super_data ) );

  CHECK( result == lorina::return_code::success );

  super_utils<3> super( gates, super_data );

  auto const& lib = super.get_super_library();
  CHECK( lib.size() == 20 );

  CHECK( lib[0].id == 0 );
  CHECK( lib[0].is_super == false );
  CHECK( lib[0].root == nullptr );
  CHECK( lib[0].num_vars == 0 );
  CHECK( lib[0].function._bits[0] == 0xAA );
  CHECK( lib[0].tdelay[0] == 0 );
  CHECK( lib[0].fanin.size() == 0 );

  CHECK( lib[1].id == 1 );
  CHECK( lib[1].is_super == false );
  CHECK( lib[1].root == nullptr );
  CHECK( lib[1].num_vars == 0 );
  CHECK( lib[1].function._bits[0] == 0xCC );
  CHECK( lib[1].tdelay[0] == 0 );
  CHECK( lib[1].fanin.size() == 0 );

  CHECK( lib[2].id == 2 );
  CHECK( lib[2].is_super == false );
  CHECK( lib[2].root == nullptr );
  CHECK( lib[2].num_vars == 0 );
  CHECK( lib[2].function._bits[0] == 0xF0 );
  CHECK( lib[2].tdelay[0] == 0 );
  CHECK( lib[2].fanin.size() == 0 );

  CHECK( lib[3].id == 3 );
  CHECK( lib[3].is_super == false );
  CHECK( lib[3].root == &gates[0] );
  CHECK( lib[3].num_vars == 0 );
  CHECK( lib[3].function == gates[0].function );
  CHECK( lib[3].tdelay[0] == 0 );
  CHECK( lib[3].fanin.size() == 0 );

  CHECK( lib[4].id == 4 );
  CHECK( lib[4].is_super == false );
  CHECK( lib[4].root == &gates[1] );
  CHECK( lib[4].num_vars == 0 );
  CHECK( lib[4].function == gates[1].function );
  CHECK( lib[4].tdelay[0] == 0 );
  CHECK( lib[4].fanin.size() == 0 );

  CHECK( lib[6].id == 6 );
  CHECK( lib[6].is_super == false );
  CHECK( lib[6].root == &gates[3] );
  CHECK( lib[6].num_vars == 1 );
  CHECK( lib[6].function == gates[3].function );
  CHECK( lib[6].tdelay[0] == 1 );
  CHECK( lib[6].fanin.size() == 0 );

  CHECK( lib[7].id == 7 );
  CHECK( lib[7].is_super == false );
  CHECK( lib[7].root == &gates[4] );
  CHECK( lib[7].num_vars == 2 );
  CHECK( lib[7].function == gates[4].function );
  CHECK( lib[7].tdelay[0] == 1 );
  CHECK( lib[7].tdelay[1] == 1 );
  CHECK( lib[7].fanin.size() == 0 );

  CHECK( lib[9].id == 9 );
  CHECK( lib[9].is_super == false );
  CHECK( lib[9].root == &gates[4] );
  CHECK( lib[9].num_vars == 2 );
  CHECK( lib[9].function._bits[0] == 0x88 );
  CHECK( lib[9].tdelay[0] == 1 );
  CHECK( lib[9].tdelay[1] == 1 );
  CHECK( lib[9].fanin.size() == 2 );
  CHECK( lib[9].fanin[0] == &lib[1] );
  CHECK( lib[9].fanin[1] == &lib[0] );

  CHECK( lib[10].id == 10 );
  CHECK( lib[10].is_super == true );
  CHECK( lib[10].root == &gates[4] );
  CHECK( lib[10].num_vars == 3 );
  CHECK( lib[10].function._bits[0] == 0x80 );
  CHECK( lib[10].tdelay[0] == 2 );
  CHECK( lib[10].tdelay[1] == 2 );
  CHECK( lib[10].tdelay[2] == 1 );
  CHECK( lib[10].fanin.size() == 2 );
  CHECK( lib[10].fanin[0] == &lib[2] );
  CHECK( lib[10].fanin[1] == &lib[9] );

  CHECK( lib[13].id == 13 );
  CHECK( lib[13].is_super == false );
  CHECK( lib[13].root == &gates[5] );
  CHECK( lib[13].num_vars == 2 );
  CHECK( lib[13].function._bits[0] == 0xFC );
  CHECK( lib[13].tdelay[0] == 0 );
  CHECK( lib[13].tdelay[1] == 1 );
  CHECK( lib[13].tdelay[2] == 1 );
  CHECK( lib[13].fanin.size() == 2 );
  CHECK( lib[13].fanin[0] == &lib[2] );
  CHECK( lib[13].fanin[1] == &lib[1] );

  CHECK( lib[14].id == 14 );
  CHECK( lib[14].is_super == true );
  CHECK( lib[14].root == &gates[4] );
  CHECK( lib[14].num_vars == 3 );
  CHECK( lib[14].function._bits[0] == 0xA8 );
  CHECK( lib[14].tdelay[0] == 1 );
  CHECK( lib[14].tdelay[1] == 2 );
  CHECK( lib[14].tdelay[2] == 2 );
  CHECK( lib[14].fanin.size() == 2 );
  CHECK( lib[14].fanin[0] == &lib[0] );
  CHECK( lib[14].fanin[1] == &lib[13] );
}
