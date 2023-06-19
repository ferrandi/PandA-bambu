#include <catch.hpp>

#include <sstream>
#include <string>

#include <mockturtle/io/bench_reader.hpp>
#include <mockturtle/networks/klut.hpp>

#include <lorina/bench.hpp>

using namespace mockturtle;

TEST_CASE( "read a BENCH file into K-LUT network", "[bench_reader]" )
{
  klut_network klut;

  std::string file{ "INPUT(a)\n"
                    "INPUT(b)\n"
                    "INPUT(c)\n"
                    "OUTPUT(sum)\n"
                    "OUTPUT(carry)\n"
                    "sum = LUT 0x96 (a, b, c)\n"
                    "carry = LUT 0xe8 (a, b, c)\n" };

  std::istringstream in( file );
  auto const result = lorina::read_bench( in, bench_reader( klut ) );
  CHECK( result == lorina::return_code::success );

  CHECK( klut.size() == 7 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 2 );
  CHECK( klut.num_gates() == 2 );

  klut.foreach_po( [&]( auto const& f, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( klut.node_function( klut.get_node( f ) )._bits[0] == 0x96 );
      break;
    case 1:
      CHECK( klut.node_function( klut.get_node( f ) )._bits[0] == 0xe8 );
      break;
    }
  } );
}
