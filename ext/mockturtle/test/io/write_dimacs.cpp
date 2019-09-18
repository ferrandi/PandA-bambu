#include <catch.hpp>

#include <sstream>

#include <mockturtle/io/write_dimacs.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "write XAG into DIMACS", "[write_dimacs]" )
{
  xag_network xag;

  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f1 = xag.create_nand( a, b );
  const auto f2 = xag.create_nand( a, f1 );
  const auto f3 = xag.create_nand( b, f1 );
  const auto f4 = xag.create_nand( f2, f3 );

  xag.create_po( f4 );

  std::ostringstream out;
  write_dimacs( xag, out );

  CHECK( out.str() == "p cnf 7 14\n"
                      "-1 0\n"
                      "2 -4 0\n"
                      "3 -4 0\n"
                      "-2 -3 4 0\n"
                      "2 -5 0\n"
                      "-4 -5 0\n"
                      "-2 4 5 0\n"
                      "3 -6 0\n"
                      "-4 -6 0\n"
                      "-3 4 6 0\n"
                      "-5 -7 0\n"
                      "-6 -7 0\n"
                      "5 6 7 0\n"
                      "-7 0\n" );
}
