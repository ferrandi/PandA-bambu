#include <catch.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/crossed.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/muxig.hpp>
#include <mockturtle/views/binding_view.hpp>
#include <mockturtle/views/rank_view.hpp>

using namespace mockturtle;

TEST_CASE( "write single-gate AIG into Verilog file", "[write_verilog]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_or( a, b );
  aig.create_po( f1 );

  std::ostringstream out;
  write_verilog( aig, out );

  CHECK( out.str() == "module top( x0 , x1 , y0 );\n"
                      "  input x0 , x1 ;\n"
                      "  output y0 ;\n"
                      "  wire n3 ;\n"
                      "  assign n3 = ~x0 & ~x1 ;\n"
                      "  assign y0 = ~n3 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write AIG for XOR into Verilog file", "[write_verilog]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  std::ostringstream out;
  write_verilog( aig, out );

  CHECK( out.str() == "module top( x0 , x1 , y0 );\n"
                      "  input x0 , x1 ;\n"
                      "  output y0 ;\n"
                      "  wire n3 , n4 , n5 , n6 ;\n"
                      "  assign n3 = x0 & x1 ;\n"
                      "  assign n4 = x0 & ~n3 ;\n"
                      "  assign n5 = x1 & ~n3 ;\n"
                      "  assign n6 = ~n4 & ~n5 ;\n"
                      "  assign y0 = ~n6 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write MIG into Verilog file", "[write_verilog]" )
{
  mig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_or( a, b );
  const auto f3 = aig.create_maj( f1, f2, c );
  aig.create_po( f3 );

  std::ostringstream out;
  write_verilog( aig, out );

  CHECK( out.str() == "module top( x0 , x1 , x2 , y0 );\n"
                      "  input x0 , x1 , x2 ;\n"
                      "  output y0 ;\n"
                      "  wire n4 , n5 , n6 ;\n"
                      "  assign n4 = x0 & x1 ;\n"
                      "  assign n5 = x0 | x1 ;\n"
                      "  assign n6 = ( x2 & n4 ) | ( x2 & n5 ) | ( n4 & n5 ) ;\n"
                      "  assign y0 = n6 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write MuxIG into Verilog file", "[write_verilog]" )
{
  muxig_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto f1 = ntk.create_ite( a, b, c );
  ntk.create_po( f1 );

  std::ostringstream out;
  write_verilog( ntk, out );

  CHECK( out.str() == "module top( x0 , x1 , x2 , y0 );\n"
                      "  input x0 , x1 , x2 ;\n"
                      "  output y0 ;\n"
                      "  wire n4 ;\n"
                      "  assign n4 = x0 ? x1 : x2 ;\n"
                      "  assign y0 = n4 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write Verilog with register names", "[write_verilog]" )
{
  mig_network mig;

  std::vector<mig_network::signal> as( 3u );
  std::vector<mig_network::signal> bs( 3u );
  std::generate( as.begin(), as.end(), [&]() { return mig.create_pi(); } );
  std::generate( bs.begin(), bs.end(), [&]() { return mig.create_pi(); } );
  auto carry = mig.get_constant( false );
  carry_ripple_adder_inplace( mig, as, bs, carry );
  as.push_back( carry );
  std::for_each( as.begin(), as.end(), [&]( auto const& f ) { mig.create_po( f ); } );

  std::ostringstream out;
  write_verilog_params ps;
  ps.input_names = { { "a", 3u }, { "b", 3u } };
  ps.output_names = { { "y", 4u } };
  write_verilog( mig, out, ps );

  CHECK( out.str() == "module top( a , b , y );\n"
                      "  input [2:0] a ;\n"
                      "  input [2:0] b ;\n"
                      "  output [3:0] y ;\n"
                      "  wire n7 , n8 , n9 , n10 , n11 , n12 , n13 , n14 , n15 , n16 , n17 , n18 ;\n"
                      "  assign n8 = a[0] & ~b[0] ;\n"
                      "  assign n9 = a[0] | b[0] ;\n"
                      "  assign n10 = ( ~a[0] & n8 ) | ( ~a[0] & n9 ) | ( n8 & n9 ) ;\n"
                      "  assign n7 = a[0] & b[0] ;\n"
                      "  assign n12 = ( a[1] & ~b[1] ) | ( a[1] & n7 ) | ( ~b[1] & n7 ) ;\n"
                      "  assign n13 = ( a[1] & b[1] ) | ( a[1] & ~n7 ) | ( b[1] & ~n7 ) ;\n"
                      "  assign n14 = ( ~a[1] & n12 ) | ( ~a[1] & n13 ) | ( n12 & n13 ) ;\n"
                      "  assign n11 = ( a[1] & b[1] ) | ( a[1] & n7 ) | ( b[1] & n7 ) ;\n"
                      "  assign n16 = ( a[2] & ~b[2] ) | ( a[2] & n11 ) | ( ~b[2] & n11 ) ;\n"
                      "  assign n17 = ( a[2] & b[2] ) | ( a[2] & ~n11 ) | ( b[2] & ~n11 ) ;\n"
                      "  assign n18 = ( ~a[2] & n16 ) | ( ~a[2] & n17 ) | ( n16 & n17 ) ;\n"
                      "  assign n15 = ( a[2] & b[2] ) | ( a[2] & n11 ) | ( b[2] & n11 ) ;\n"
                      "  assign y[0] = n10 ;\n"
                      "  assign y[1] = n14 ;\n"
                      "  assign y[2] = n18 ;\n"
                      "  assign y[3] = n15 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write buffered AIG into Verilog file", "[write_verilog]" )
{
  buffered_aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto buf_a1 = aig.create_buf( a );
  const auto buf_a2 = aig.create_buf( buf_a1 );

  const auto f1 = aig.create_or( buf_a2, b );
  const auto buf_f1 = aig.create_buf( f1 );
  aig.create_po( buf_f1 );

  std::ostringstream out;
  write_verilog( aig, out );

  CHECK( out.str() == "module buffer( i , o );\n"
                      "  input i ;\n"
                      "  output o ;\n"
                      "endmodule\n"
                      "module inverter( i , o );\n"
                      "  input i ;\n"
                      "  output o ;\n"
                      "endmodule\n"
                      "module top( x0 , x1 , y0 );\n"
                      "  input x0 , x1 ;\n"
                      "  output y0 ;\n"
                      "  wire n3 , n4 , n5 , n6 ;\n"
                      "  buffer buf_n3( .i (x0), .o (n3) );\n"
                      "  buffer buf_n4( .i (n3), .o (n4) );\n"
                      "  assign n5 = ~x1 & ~n4 ;\n"
                      "  inverter inv_n6( .i (n5), .o (n6) );\n"
                      "  assign y0 = n6 ;\n"
                      "endmodule\n" );
}

TEST_CASE( "write mapped network into Verilog file", "[write_verilog]" )
{
  std::string const simple_test_library = "GATE   zero    0 O=0;\n"
                                          "GATE   inv1    1 O=!a;     PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                          "GATE   inv2    2 O=!a;     PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                          "GATE   buf     2 O=a;      PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                          "GATE   nand2   2 O=!(a*b); PIN * INV 1 999 1.0 0.2 1.0 0.2\n";

  std::vector<gate> gates;
  std::istringstream in( simple_test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  binding_view<klut_network> klut( gates );

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  /* create buffer */
  uint64_t buf_func = 0x2;
  kitty::dynamic_truth_table tt_buf( 1 );
  kitty::create_from_words( tt_buf, &buf_func, &buf_func + 1 );
  const auto buf = klut.create_node( { a }, tt_buf );

  const auto f1 = klut.create_nand( b, c );
  const auto f2 = klut.create_not( f1 );

  klut.create_po( klut.get_constant( false ) );
  klut.create_po( buf );
  klut.create_po( f1 );
  klut.create_po( f2 );

  klut.add_binding( klut.get_node( klut.get_constant( false ) ), 0 );
  klut.add_binding( klut.get_node( buf ), 3 );
  klut.add_binding( klut.get_node( f1 ), 4 );
  klut.add_binding( klut.get_node( f2 ), 2 );

  std::ostringstream out;
  write_verilog_with_binding( klut, out );

  CHECK( out.str() == "module top( x0 , x1 , x2 , y0 , y1 , y2 , y3 );\n"
                      "  input x0 , x1 , x2 ;\n"
                      "  output y0 , y1 , y2 , y3 ;\n"
                      "  zero  g0( .O (y0) );\n"
                      "  buf   g1( .a (x0), .O (y1) );\n"
                      "  nand2 g2( .a (x1), .b (x2), .O (y2) );\n"
                      "  inv2  g3( .a (y2), .O (y3) );\n"
                      "endmodule\n" );
}

TEST_CASE( "write mapped network with multiple driven POs and register names into Verilog file", "[write_verilog]" )
{
  std::string const simple_test_library = "GATE   inv1    1 Y=!a;     PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                          "GATE   inv2    2 Y=!a;     PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                          "GATE   buf     2 Y=a;      PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                          "GATE   nand2   2 Y=!(a*b); PIN * INV 1 999 1.0 0.2 1.0 0.2\n";

  std::vector<gate> gates;
  std::istringstream in( simple_test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  binding_view<klut_network> klut( gates );

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  /* create buffer */
  uint64_t buf_func = 0x2;
  kitty::dynamic_truth_table tt_buf( 1 );
  kitty::create_from_words( tt_buf, &buf_func, &buf_func + 1 );
  const auto buf = klut.create_node( { a }, tt_buf );

  const auto f1 = klut.create_nand( b, c );
  const auto f2 = klut.create_not( f1 );

  klut.create_po( buf );
  klut.create_po( f1 );
  klut.create_po( f1 );
  klut.create_po( f2 );

  klut.add_binding( klut.get_node( buf ), 2 );
  klut.add_binding( klut.get_node( f1 ), 3 );
  klut.add_binding( klut.get_node( f2 ), 1 );

  std::ostringstream out;
  write_verilog_params ps;
  ps.input_names = { { "ref", 1u }, { "data", 2u } };
  ps.output_names = { { "y", 4u } };
  write_verilog_with_binding( klut, out, ps );

  CHECK( out.str() == "module top( ref , data , y );\n"
                      "  input [0:0] ref ;\n"
                      "  input [1:0] data ;\n"
                      "  output [3:0] y ;\n"
                      "  buf   g0( .a (ref[0]), .Y (y[0]) );\n"
                      "  nand2 g1( .a (data[0]), .b (data[1]), .Y (y[1]) );\n"
                      "  nand2 g2( .a (data[0]), .b (data[1]), .Y (y[2]) );\n"
                      "  inv2  g3( .a (y[1]), .Y (y[3]) );\n"
                      "endmodule\n" );
}

TEST_CASE( "write buffered_crossed_klut into Verilog file", "[write_verilog]" )
{
  buffered_crossed_klut_network crossed;

  // level 0
  auto const x0 = crossed.create_pi();
  auto const x1 = crossed.create_pi();

  // level 1
  auto const b4 = crossed.create_buf( x0 );
  auto const [c5x0, c5x1] = crossed.create_crossing( x0, x1 );
  auto const b6 = crossed.create_buf( x1 );

  // level 2
  auto const b7 = crossed.create_buf( b4 );
  auto const [c8x0, c8x1] = crossed.create_crossing( b4, c5x1 );
  auto const [c9x0, c9x1] = crossed.create_crossing( c5x0, b6 );
  auto const b10 = crossed.create_buf( b6 );

  // level 3
  auto const n11 = crossed.create_nand( b7, c8x1 );
  auto const n12 = crossed.create_or( c8x0, c9x1 );
  auto const n13 = crossed.create_xor( c9x0, b10 );

  crossed.create_po( n11 );
  crossed.create_po( n12 );
  crossed.create_po( n13 );

  rank_view ranked( crossed ); // I just want `is_topologically_sorted = true` set by the view to prevent topo_view in write_verilog to mess the order up
  std::ostringstream out;
  write_verilog( ranked, out );
  CHECK( out.str() == "module buffer( i , o );\n"
                      "  input i ;\n"
                      "  output o ;\n"
                      "endmodule\n"
                      "module inverter( i , o );\n"
                      "  input i ;\n"
                      "  output o ;\n"
                      "endmodule\n"
                      "module crossing( i1 , i2 , o1 , o2 );\n"
                      "  input i1 , i2 ;\n"
                      "  output o1 , o2 ;\n"
                      "endmodule\n"
                      "module top( x0 , x1 , y0 , y1 , y2 );\n"
                      "  input x0 , x1 ;\n"
                      "  output y0 , y1 , y2 ;\n"
                      "  wire n4 , n5 , n6 , n7 , n8 , n9 , n10 , n11 , n12 , n13 ;\n"
                      "  buffer buf_n4( .i (x0), .o (n4) );\n"
                      "  crossing cross_n5( .i1 (x0), .i2 (x1), .o1 (n5_1), .o2 (n5_2) );\n"
                      "  buffer buf_n6( .i (x1), .o (n6) );\n"
                      "  buffer buf_n7( .i (n4), .o (n7) );\n"
                      "  crossing cross_n8( .i1 (n4), .i2 (n5_2), .o1 (n8_1), .o2 (n8_2) );\n"
                      "  crossing cross_n9( .i1 (n5_1), .i2 (n6), .o1 (n9_1), .o2 (n9_2) );\n"
                      "  buffer buf_n10( .i (n6), .o (n10) );\n"
                      "  assign n11 = ~n7 | ~n8_2 ;\n"
                      "  assign n12 = n8_1 | n9_2 ;\n"
                      "  assign n13 = n9_1 ^ n10 ;\n"
                      "  assign y0 = n11 ;\n"
                      "  assign y1 = n12 ;\n"
                      "  assign y2 = n13 ;\n"
                      "endmodule\n" );
}
