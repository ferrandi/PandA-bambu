#include <catch.hpp>

#include <lorina/genlib.hpp>
#include <mockturtle/io/genlib_reader.hpp>

#include <sstream>
#include <string>

TEST_CASE( "read genlib file", "[genlib_reader]" )
{
  using mockturtle::phase_type;

  std::vector<mockturtle::gate> gates;

  std::string const file{
      "GATE zero 0 O=CONST0;\n"
      "GATE one 0 O=CONST1;\n"
      "GATE inverter 1 O=!a; PIN * INV 1 999 1.0 1.0 1.0 1.0\n"
      "GATE buffer 2 O=a; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
      "GATE and 5 Y=a*b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
      "GATE or 5 Y=n1+n2; PIN n1 NONINV 1 999 1.0 1.0 1.0 1.0; PIN n2 NONINV 1 999 0.98 1.0 0.98 1.0\n" };

  std::istringstream in( file );
  auto const result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  CHECK( gates.size() == 6u );
  CHECK( gates[0u].id == 0u );
  CHECK( gates[0u].name == "zero" );
  CHECK( gates[0u].expression == "CONST0" );
  CHECK( gates[0u].function._bits[0] == 0 );
  CHECK( gates[0u].num_vars == 0 );
  CHECK( gates[0u].area == 0.0 );
  CHECK( gates[0u].pins.empty() );
  CHECK( gates[0u].output_name == "O" );

  CHECK( gates[1u].id == 1u );
  CHECK( gates[1u].name == "one" );
  CHECK( gates[1u].expression == "CONST1" );
  CHECK( gates[1u].function._bits[0] == 1 );
  CHECK( gates[1u].num_vars == 0 );
  CHECK( gates[1u].area == 0.0 );
  CHECK( gates[1u].pins.empty() );
  CHECK( gates[1u].output_name == "O" );

  CHECK( gates[2u].id == 2u );
  CHECK( gates[2u].name == "inverter" );
  CHECK( gates[2u].expression == "!a" );
  CHECK( gates[2u].function._bits[0] == 1 );
  CHECK( gates[2u].num_vars == 1 );
  CHECK( gates[2u].area == 1.0 );
  CHECK( gates[2u].pins.size() == 1 );
  CHECK( gates[2u].pins[0u].name == "a" );
  CHECK( gates[2u].pins[0u].phase == phase_type::INV );
  CHECK( gates[2u].pins[0u].input_load == 1.0 );
  CHECK( gates[2u].pins[0u].max_load == 999.0 );
  CHECK( gates[2u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[2u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[2u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[2u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[2u].output_name == "O" );

  CHECK( gates[3u].id == 3u );
  CHECK( gates[3u].name == "buffer" );
  CHECK( gates[3u].expression == "a" );
  CHECK( gates[3u].function._bits[0] == 2 );
  CHECK( gates[3u].num_vars == 1 );
  CHECK( gates[3u].area == 2.0 );
  CHECK( gates[3u].pins.size() == 1 );
  CHECK( gates[3u].pins[0u].name == "a" );
  CHECK( gates[3u].pins[0u].phase == phase_type::NONINV );
  CHECK( gates[3u].pins[0u].input_load == 1.0 );
  CHECK( gates[3u].pins[0u].max_load == 999.0 );
  CHECK( gates[3u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[3u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[3u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[3u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[3u].output_name == "O" );

  CHECK( gates[4u].id == 4u );
  CHECK( gates[4u].name == "and" );
  CHECK( gates[4u].expression == "a*b" );
  CHECK( gates[4u].function._bits[0] == 8 );
  CHECK( gates[4u].num_vars == 2 );
  CHECK( gates[4u].area == 5.0 );
  CHECK( gates[4u].pins.size() == 2 );
  CHECK( gates[4u].pins[0u].name == "a" );
  CHECK( gates[4u].pins[0u].phase == phase_type::NONINV );
  CHECK( gates[4u].pins[0u].input_load == 1.0 );
  CHECK( gates[4u].pins[0u].max_load == 999.0 );
  CHECK( gates[4u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[4u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[4u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[4u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[4u].pins[1u].name == "b" );
  CHECK( gates[4u].pins[1u].phase == phase_type::NONINV );
  CHECK( gates[4u].pins[1u].input_load == 1.0 );
  CHECK( gates[4u].pins[1u].max_load == 999.0 );
  CHECK( gates[4u].pins[1u].rise_block_delay == 1.0 );
  CHECK( gates[4u].pins[1u].rise_fanout_delay == 1.0 );
  CHECK( gates[4u].pins[1u].rise_block_delay == 1.0 );
  CHECK( gates[4u].pins[1u].rise_fanout_delay == 1.0 );
  CHECK( gates[4u].output_name == "Y" );

  CHECK( gates[5u].id == 5u );
  CHECK( gates[5u].name == "or" );
  CHECK( gates[5u].expression == "n1+n2" );
  CHECK( gates[5u].function._bits[0] == 0xe );
  CHECK( gates[5u].num_vars == 2 );
  CHECK( gates[5u].area == 5.0 );
  CHECK( gates[5u].pins.size() == 2 );
  CHECK( gates[5u].pins[0u].name == "n1" );
  CHECK( gates[5u].pins[0u].phase == phase_type::NONINV );
  CHECK( gates[5u].pins[0u].input_load == 1.0 );
  CHECK( gates[5u].pins[0u].max_load == 999.0 );
  CHECK( gates[5u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[5u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[5u].pins[0u].rise_block_delay == 1.0 );
  CHECK( gates[5u].pins[0u].rise_fanout_delay == 1.0 );
  CHECK( gates[5u].pins[1u].name == "n2" );
  CHECK( gates[5u].pins[1u].phase == phase_type::NONINV );
  CHECK( gates[5u].pins[1u].input_load == 1.0 );
  CHECK( gates[5u].pins[1u].max_load == 999.0 );
  CHECK( gates[5u].pins[1u].rise_block_delay == 0.98 );
  CHECK( gates[5u].pins[1u].rise_fanout_delay == 1.0 );
  CHECK( gates[5u].pins[1u].rise_block_delay == 0.98 );
  CHECK( gates[5u].pins[1u].rise_fanout_delay == 1.0 );
  CHECK( gates[5u].output_name == "Y" );
}

TEST_CASE( "skip gate with invalid formula", "[genlib_reader]" )
{
  std::vector<mockturtle::gate> gates;
  std::string const file{
      "GATE or 5 Y=(n1+n2; PIN n1 NONINV 1 999 1.0 1.0 1.0 1.0; PIN n2 NONINV 1 999 0.98 1.0 0.98 1.0\n" };

  std::istringstream in( file );
  auto const result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  CHECK( gates.size() == 0u );
}
