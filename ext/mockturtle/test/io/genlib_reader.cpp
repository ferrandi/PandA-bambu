#include <catch.hpp>

#include <mockturtle/io/genlib_reader.hpp>
#include <lorina/genlib.hpp>

#include <sstream>
#include <string>

TEST_CASE( "read genlib file", "[genlib_reader]" )
{
  std::vector<mockturtle::gate> gates;

  std::string const file{
    "GATE zero 0 O=0;\n"
    "GATE one 0 O=1;\n"
    "GATE inverter 1 O=!a; PIN * INV 1 999 1.0 1.0 1.0 1.0\n"
    "GATE buffer 2 O=a; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
    "GATE and 5 O=(ab); PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
  };

  std::istringstream in( file );
  auto const result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  CHECK( gates.size() == 5u );
  CHECK( gates[0u].name == "zero" );
  CHECK( gates[0u].expression == "0" );
  CHECK( gates[0u].function._bits[0] == 0 );
  CHECK( gates[0u].num_vars == 0 );
  CHECK( gates[0u].area == 0.0 );
  CHECK( gates[0u].delay == 1.0 );

  CHECK( gates[1u].name == "one" );
  CHECK( gates[1u].expression == "1" );
  CHECK( gates[1u].function._bits[0] == 1 );
  CHECK( gates[1u].num_vars == 0 );
  CHECK( gates[1u].area == 0.0 );
  CHECK( gates[1u].delay == 1.0 );

  CHECK( gates[2u].name == "inverter" );
  CHECK( gates[2u].expression == "!a" );
  CHECK( gates[2u].function._bits[0] == 1 );
  CHECK( gates[2u].num_vars == 1 );
  CHECK( gates[2u].area == 1.0 );
  CHECK( gates[2u].delay == 1.0 );

  CHECK( gates[3u].name == "buffer" );
  CHECK( gates[3u].expression == "a" );
  CHECK( gates[3u].function._bits[0] == 2 );
  CHECK( gates[3u].num_vars == 1 );
  CHECK( gates[3u].area == 2.0 );
  CHECK( gates[3u].delay == 1.0 );

  CHECK( gates[4u].name == "and" );
  CHECK( gates[4u].expression == "(ab)" );
  CHECK( gates[4u].function._bits[0] == 8 );
  CHECK( gates[4u].num_vars == 2 );
  CHECK( gates[4u].area == 5.0 );
  CHECK( gates[4u].delay == 1.0 );
}
