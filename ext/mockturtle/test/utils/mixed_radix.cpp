#include <catch.hpp>

#include <mockturtle/utils/mixed_radix.hpp>

using namespace mockturtle;

TEST_CASE( "mixed radix loop", "[mixed_radix]" )
{
  uint32_t m[] = { 3, 2, 4 };
  uint32_t count = 0u;
  foreach_mixed_radix_tuple( m, m + 3, [&count]( auto, auto ) {
    ++count;
  } );

  CHECK( count == 24u );
  count = 0u;

  foreach_mixed_radix_tuple( m, m + 3, [&count]( auto, auto ) {
    ++count;
    return true;
  } );

  CHECK( count == 24u );
  count = 0u;

  foreach_mixed_radix_tuple( m, m + 3, [&count]( auto, auto ) {
    ++count;
    return false;
  } );

  CHECK( count == 1u );
}
