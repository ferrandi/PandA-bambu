#include <catch.hpp>

#include <lorina/genlib.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_genlib.hpp>

#include <sstream>
#include <string>

using namespace mockturtle;

TEST_CASE( "Write genlib file", "[write_genlib]" )
{
  gate g0, g1, g2, g3;
  pin p;

  g0.id = 0;
  g0.name = "zero";
  g0.expression = "CONST0";
  g0.num_vars = 0;
  g0.area = 0;
  g0.output_name = "O";

  g1.id = 1;
  g1.name = "one";
  g1.expression = "CONST1";
  g1.num_vars = 0;
  g1.area = 0;
  g1.output_name = "O";

  g2.id = 2;
  g2.name = "inverter";
  g2.expression = "!a";
  g2.num_vars = 1;
  g2.area = 1;
  g2.output_name = "O";
  p.name = "*";
  p.phase = phase_type::INV;
  p.input_load = 1;
  p.max_load = 999;
  p.rise_block_delay = 1.0;
  p.rise_fanout_delay = 1.0;
  p.fall_block_delay = 1.0;
  p.fall_fanout_delay = 1.0;
  g2.pins.push_back( p );

  g3.id = 3;
  g3.name = "or";
  g3.expression = "n1+n2";
  g3.num_vars = 2;
  g3.area = 5;
  g3.output_name = "Y";
  p.name = "n1";
  p.phase = phase_type::NONINV;
  p.input_load = 1;
  p.max_load = 999;
  p.rise_block_delay = 1.0;
  p.rise_fanout_delay = 1.0;
  p.fall_block_delay = 1.0;
  p.fall_fanout_delay = 1.0;
  g3.pins.push_back( p );
  p.name = "n2";
  p.phase = phase_type::NONINV;
  p.input_load = 1;
  p.max_load = 999;
  p.rise_block_delay = 0.97;
  p.rise_fanout_delay = 1.0;
  p.fall_block_delay = 0.98;
  p.fall_fanout_delay = 1.0;
  g3.pins.push_back( p );

  std::vector<gate> gates = { g0, g1, g2, g3 };

  std::string const gold_file{
      "GATE zero      0.0000 O=CONST0;\n"
      "GATE one       0.0000 O=CONST1;\n"
      "GATE inverter  1.0000 O=!a;\n\tPIN * INV   1 999 1.0000 1.0000 1.0000 1.0000\n"
      "GATE or        5.0000 Y=n1+n2;\n\tPIN n1 NONINV   1 999 1.0000 1.0000 1.0000 1.0000"
      "\n\tPIN n2 NONINV   1 999 0.9700 1.0000 0.9800 1.0000\n" };

  std::ostringstream ss;
  write_genlib( gates, ss );

  CHECK( gold_file == ss.str() );
}
