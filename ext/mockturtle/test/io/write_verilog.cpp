#include <catch.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>

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
  ps.input_names = {{"a", 3u}, {"b", 3u}};
  ps.output_names = {{"y", 4u}};
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
