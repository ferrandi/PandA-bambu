#include <catch.hpp>

#include <sstream>
#include <string>

#include <kitty/constructors.hpp>
#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/dimacs_reader.hpp>
#include <mockturtle/networks/xag.hpp>

#include <lorina/pla.hpp>

using namespace mockturtle;

TEST_CASE( "read a DIMACS file into an XAG", "[dimacs_reader]" )
{
  xag_network xag;

  std::string dimacs =
      "c\n"
      "c start with comments\n"
      "c\n"
      "c\n"
      "p cnf 5 3\n"
      "1 -5 4 0 "
      "-1 5 3 4 0 "
      "-3 -4 0\n";
  std::istringstream iss( dimacs );

  dimacs_reader reader( xag );
  auto result = lorina::read_dimacs( iss, reader );

  CHECK( result == lorina::return_code::success );
  CHECK( xag.num_pis() == 5 );
  CHECK( xag.num_pos() == 1 );
  CHECK( xag.num_gates() == 8 );

  kitty::static_truth_table<5u> expected;
  kitty::create_from_expression( expected, "({a!ed}{!aecd}{!c!d})" );
  CHECK( simulate<kitty::static_truth_table<5u>>( xag )[0] == expected );
}
