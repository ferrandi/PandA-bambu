#include <catch.hpp>

#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/properties/litcost.hpp>

using namespace mockturtle;

TEST_CASE( "count factored form literals for constants", "[litcost]" )
{
  kitty::dynamic_truth_table tt( 3 );

  CHECK( factored_literal_cost( tt ) == 0u );
  CHECK( factored_literal_cost( ~tt ) == 0u );
}

TEST_CASE( "count factored form literals for constants cube", "[litcost]" )
{
  std::vector<kitty::cube> sop;
  sop.emplace_back( kitty::cube{ 3, 0 } );

  CHECK( factored_literal_cost( sop, 2 ) == 0u );
}

TEST_CASE( "count factored form literals for sop", "[litcost]" )
{
  std::vector<kitty::cube> sop;
  sop.emplace_back( kitty::cube{ 3, 3 } );
  sop.emplace_back( kitty::cube{ 5, 5 } );

  CHECK( factored_literal_cost( sop, 3 ) == 3u );
}

TEST_CASE( "count factored form literals for constants using cares", "[litcost]" )
{
  kitty::dynamic_truth_table tt( 2 );
  kitty::create_from_binary_string( tt, "0110" );

  kitty::dynamic_truth_table dc( 2 );
  kitty::create_from_binary_string( dc, "1001" );

  CHECK( factored_literal_cost( tt, dc ) == 0u );
}

TEST_CASE( "count factored form literals 1", "[litcost]" )
{
  kitty::dynamic_truth_table tt( 3 );
  kitty::create_from_binary_string( tt, "11100000" );

  CHECK( factored_literal_cost( tt, true ) == 3u );
}

TEST_CASE( "count factored form literals maj3", "[litcost]" )
{
  kitty::dynamic_truth_table tt( 3 );
  kitty::create_from_binary_string( tt, "11101000" );

  CHECK( factored_literal_cost( tt, true ) == 5u );
}

TEST_CASE( "count factored form literals maj3 dc", "[litcost]" )
{
  kitty::dynamic_truth_table tt( 3 );
  kitty::create_from_binary_string( tt, "11101000" );

  kitty::dynamic_truth_table dc( 3 );
  kitty::create_from_binary_string( dc, "00001000" );

  CHECK( factored_literal_cost( tt, dc, true ) == 3u );
}