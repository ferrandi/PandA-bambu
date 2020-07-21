#include <catch.hpp>

#include <sstream>
#include <string>

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/pla_reader.hpp>
#include <mockturtle/networks/xag.hpp>

#include <lorina/pla.hpp>

using namespace mockturtle;

TEST_CASE( "read a PLA file into an XAG", "[pla_reader]" )
{
  xag_network xag;

  std::string file{".i 3\n"
                   ".o 2\n"
                   "1-1 11\n"
                   "00- 10\n"
                   "-11 01\n"
                   ".e\n"};

  std::istringstream in( file );
  lorina::read_pla( in, pla_reader( xag ) );

  CHECK( xag.size() == 9 );
  CHECK( xag.num_pis() == 3 );
  CHECK( xag.num_pos() == 2 );
  CHECK( xag.num_gates() == 5 );

  CHECK( simulate<kitty::static_truth_table<3u>>( xag )[0]._bits == 0xb1u );
  CHECK( simulate<kitty::static_truth_table<3u>>( xag )[1]._bits == 0xe0u );
}
