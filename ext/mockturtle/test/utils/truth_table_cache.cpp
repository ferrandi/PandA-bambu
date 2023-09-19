#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/utils/truth_table_cache.hpp>

using namespace mockturtle;

TEST_CASE( "working with a truth table cache", "[truth_table_cache]" )
{
  truth_table_cache<kitty::dynamic_truth_table> cache;

  kitty::dynamic_truth_table zero( 0u ), x1( 1u ), f_and( 2u ), f_or( 2u ), f_maj( 3u );

  kitty::create_from_hex_string( x1, "2" );
  kitty::create_from_hex_string( f_and, "8" );
  kitty::create_from_hex_string( f_or, "e" );
  kitty::create_from_hex_string( f_maj, "e8" );

  CHECK( cache.size() == 0 );
  CHECK( cache.insert( zero ) == 0 );
  CHECK( cache.insert( x1 ) == 2 );
  CHECK( cache.insert( f_and ) == 4 );
  CHECK( cache.insert( f_or ) == 6 );
  CHECK( cache.insert( f_maj ) == 8 );

  CHECK( cache.size() == 5 );

  CHECK( cache.insert( ~zero ) == 1 );
  CHECK( cache.insert( ~x1 ) == 3 );
  CHECK( cache.insert( ~f_and ) == 5 );
  CHECK( cache.insert( ~f_or ) == 7 );
  CHECK( cache.insert( ~f_maj ) == 9 );

  CHECK( cache.size() == 5 );

  CHECK( cache[0] == zero );
  CHECK( cache[1] == ~zero );
  CHECK( cache[2] == x1 );
  CHECK( cache[3] == ~x1 );
  CHECK( cache[4] == f_and );
  CHECK( cache[5] == ~f_and );
  CHECK( cache[6] == f_or );
  CHECK( cache[7] == ~f_or );
  CHECK( cache[8] == f_maj );
  CHECK( cache[9] == ~f_maj );
}
