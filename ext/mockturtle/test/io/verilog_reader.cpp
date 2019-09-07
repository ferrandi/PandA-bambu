#include <catch.hpp>

#include <sstream>
#include <string>

#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/simulation.hpp>

#include <kitty/kitty.hpp>
#include <lorina/verilog.hpp>

using namespace mockturtle;

TEST_CASE( "read a VERILOG file into MIG network", "[verilog_reader]" )
{
  mig_network mig;

  std::string file{
    "module top( y1, y2, a, b, c ) ;\n"
    "  input a , b , c ;\n"
    "  output y1 , y2 ;\n"
    "  wire zero, g0, g1 , g2 , g3 , g4 ;\n"
    "  assign zero = 0 ;\n"
    "  assign g0 = a ;\n"
    "  assign g1 = ~c ;\n"
    "  assign g2 = g0 & g1 ;\n"
    "  assign g3 = a | g2 ;\n"
    "  assign g4 = ( ~a & b ) | ( ~a & c ) | ( b & c ) ;\n"
    "  assign y1 = g3 ;\n"
    "  assign y2 = g4 ;\n"
      "endmodule\n"};

  std::istringstream in( file );
  auto result = lorina::read_verilog( in, verilog_reader( mig ) );

  /* structural checks */
  CHECK( result == lorina::return_code::success );
  CHECK( mig.size() == 7 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 2 );
  CHECK( mig.num_gates() == 3 );

  /* functional checks */
  default_simulator<kitty::dynamic_truth_table> sim( mig.num_pis() );
  const auto tts = simulate<kitty::dynamic_truth_table>( mig, sim );
  mig.foreach_po( [&]( auto const&, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( kitty::to_hex( tts[i] ) == "aa" );
      break;
    case 1:
      CHECK( kitty::to_hex( tts[i] ) == "d4" );
      break;
    }
    } );
}
