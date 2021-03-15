#include <catch.hpp>

#include <mockturtle/algorithms/exorcism.hpp>

using namespace mockturtle;


TEST_CASE( "Call exorcism on 4-input function", "[exorcism]" )
{
  kitty::dynamic_truth_table func( 4u );

  for ( auto i = 0u; i < 1000u; ++i )
  {
    kitty::create_random( func );
    auto esop = exorcism( func );
  
    auto func2 = func.construct();
    kitty::create_from_cubes( func2, esop, true );

    CHECK( func == func2 );
  }
}
