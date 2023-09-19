#include <catch.hpp>
#include <kitty/kitty.hpp>
#include <mockturtle/algorithms/resyn_engines/mux_resyn.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/muxig.hpp>

using namespace mockturtle;

TEST_CASE( "MuxIG resynthesis (0-resyn)", "[mux_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 10 ) );
  kitty::partial_truth_table care( 10 );
  std::vector<uint32_t> divs = { 1, 2, 3 };

  kitty::create_from_binary_string( care,   "1111001111" );
  kitty::create_from_binary_string( tts[0], "0011000110" );
  kitty::create_from_binary_string( tts[1], "1111001100" );
  kitty::create_from_binary_string( tts[2], "1000000001" );
  kitty::create_from_binary_string( tts[3], "1100001001" );

  null_stats st;
  mux_resyn<kitty::partial_truth_table> engine( st );
  const auto res = engine( tts[0], care, divs.begin(), divs.end(), tts, 0 );
  CHECK( res );
  CHECK( ( *res ).num_gates() == 0u );
  CHECK( ( *res ).raw()[1] == 7u );
}

TEST_CASE( "MuxIG resynthesis (onehot)", "[mux_resyn]" )
{
  kitty::static_truth_table<3> target;
  std::vector<kitty::static_truth_table<3>> tts( 3 );
  std::vector<uint32_t> divs;
  kitty::create_from_binary_string( target, "10000001" );
  for ( auto i = 0u; i < 3u; ++i )
  {
    kitty::create_nth_var( tts[i], i );
    divs.emplace_back( i );
  }

  null_stats st;
  mux_resyn<kitty::static_truth_table<3>> engine( st );
  const auto res = engine( target, ~target.construct(), divs.begin(), divs.end(), tts, 10 );
  CHECK( res );
  CHECK( res->raw() == std::vector<uint32_t>{3 | ( 1 << 8 ) | ( 3 << 16 ), 5, 0, 6, 4, 0, 7, 2, 8, 10, 12} );
  /* x1 ? (!x2 ? 0 : x3) : (x2 ? 0 : !x3) */

  muxig_network ntk;
  decode( ntk, *res );
  CHECK( ntk.num_gates() == 3u );

  const auto tt = simulate<kitty::static_truth_table<3u>>( ntk )[0];
  CHECK( tt._bits == 0x81 );
}
