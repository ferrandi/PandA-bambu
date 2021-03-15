#include <catch.hpp>

#include <sstream>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <lorina/verilog.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/verilog_reader.hpp>
#include <mockturtle/networks/abstract_xag.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "Create majority in abstract XAG", "[abstract_xag]" )
{
  abstract_xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto f = xag.create_maj( a, b, c );
  xag.create_po( f );

  CHECK( xag.num_pis() == 3u );
  CHECK( xag.num_pos() == 1u );
  CHECK( xag.num_gates() == 4u );
  CHECK( xag.size() == 8u );
  CHECK( xag.depth() == 1u );
  CHECK( xag.level( xag.get_node( a ) ) == 0u );
  CHECK( xag.level( xag.get_node( b ) ) == 0u );
  CHECK( xag.level( xag.get_node( c ) ) == 0u );
  CHECK( xag.level( xag.get_node( f ) ) == 1u );
}

TEST_CASE( "Check subset resolution", "[abstract_xag]" )
{
  const auto init_ntk = []( auto& ntk ) {
    const auto a = ntk.create_pi();
    const auto b = ntk.create_pi();
    const auto c = ntk.create_pi();

    ntk.create_po( ntk.create_and( ntk.create_xor( b, a ), ntk.create_nary_xor( {a, c, b} ) ) );
    ntk.create_po( ntk.create_and( ntk.create_nary_xor( {c, b, a} ), ntk.create_xor( c, a ) ) );
    ntk = cleanup_dangling( ntk );
  };

  abstract_xag_network axag;
  xag_network xag;

  init_ntk( axag );
  init_ntk( xag );

  default_simulator<kitty::dynamic_truth_table> sim( 3u );
  CHECK( simulate<kitty::dynamic_truth_table>( axag, sim ) == simulate<kitty::dynamic_truth_table>( xag, sim ) );
}

TEST_CASE( "Parse GF(2^4) function", "[abstract_xag]" )
{
  auto verilog = "module GF24Inversion(x, y);\n"
                 "    input [3:0] x;\n"
                 "    output [3:0] y;\n"
                 "    wire t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13;\n"
                 "\n"
                 "    assign t1 = x[2] ^ x[3];\n"
                 "    assign t2 = x[2] & x[0];\n"
                 "    assign t3 = x[1] ^ t2;\n"
                 "    assign t4 = x[0] ^ x[1];\n"
                 "    assign t5 = x[3] ^ t2;\n"
                 "    assign t6 = t5 & t4;\n"
                 "    assign t7 = t3 & t1;\n"
                 "    assign t8 = x[0] & x[3];\n"
                 "    assign t9 = t4 & t8;\n"
                 "    assign t10 = t4 ^ t9;\n"
                 "    assign t11 = x[1] & x[2];\n"
                 "    assign t12 = t1 & t11;\n"
                 "    assign t13 = t1 ^ t12;\n"
                 "    assign y[0] = t2 ^ t13;\n"
                 "    assign y[1] = x[3] ^ t7;\n"
                 "    assign y[2] = t2 ^ t10;\n"
                 "    assign y[3] = x[1] ^ t6;\n"
                 "endmodule\n";

  abstract_xag_network xag;

  std::stringstream str;
  str << verilog;
  const auto result = lorina::read_verilog( str, verilog_reader( xag ) );
  CHECK( result == lorina::return_code::success );
  xag = cleanup_dangling( xag );
  CHECK( xag.num_pis() == 4u );
  CHECK( xag.num_pos() == 4u );
}
