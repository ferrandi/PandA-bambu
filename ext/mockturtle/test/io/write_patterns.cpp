#include <catch.hpp>

#include <sstream>

#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_patterns.hpp>

using namespace mockturtle;

TEST_CASE( "write patterns", "[write_patterns]" )
{
  partial_simulator sim( 3, 0 );

  sim.add_pattern( { 0, 0, 0 } );
  sim.add_pattern( { 0, 0, 1 } );
  sim.add_pattern( { 0, 1, 0 } );
  sim.add_pattern( { 1, 0, 1 } );

  sim.add_pattern( { 1, 1, 1 } );
  sim.add_pattern( { 1, 0, 0 } );
  sim.add_pattern( { 1, 1, 0 } );
  sim.add_pattern( { 0, 1, 1 } );

  sim.add_pattern( { 1, 0, 1 } );

  std::ostringstream out;
  write_patterns( sim, out );

  CHECK( out.str() == "178\n"
                      "0d4\n"
                      "19a\n" );
}
