#include <catch.hpp>

#include <kitty/kitty.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/index_list.hpp>
#include <mockturtle/algorithms/resyn_engines/xag_resyn_engines.hpp>
#include <mockturtle/algorithms/simulation.hpp>

using namespace mockturtle;

void test_aig_kresub( kitty::partial_truth_table const& target, std::vector<kitty::partial_truth_table> const& tts, uint32_t num_inserts )
{
  xag_resyn_engine_stats st;
  xag_resyn_engine_params ps;
  ps.max_size = num_inserts;
  xag_resyn_engine<kitty::partial_truth_table> engine( target, ~target.construct(), st, ps );
  for ( auto i = 0u; i < tts.size(); ++i )
  {
    engine.add_divisor( i, tts );
  }
  const auto res = engine();
  CHECK( res );
  CHECK( (*res).num_gates() == num_inserts );

  aig_network aig;
  decode( aig, *res );
  partial_simulator sim( tts );
  const auto ans = simulate<kitty::partial_truth_table, aig_network, partial_simulator>( aig, sim )[0];
  CHECK( target == ans );
}

TEST_CASE( "AIG/XAG resynthesis -- 0-resub with don't care", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 1, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );
  kitty::partial_truth_table care( 8 );
  xag_resyn_engine_stats st;
  xag_resyn_engine_params ps;
  ps.max_size = 0;

  /* const */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string(   care, "11001100" );
  xag_resyn_engine<kitty::partial_truth_table> engine1( target, care, st, ps );
  const auto res1 = engine1();
  CHECK( res1 );
  CHECK( to_index_list_string( *res1 ) == "{0 | 1 << 8 | 0 << 16, 0}" );

  /* buffer */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string(   care, "00111100" );
  kitty::create_from_binary_string( tts[0], "11110000" );
  xag_resyn_engine<kitty::partial_truth_table> engine2( target, care, st, ps );
  engine2.add_divisor( tts[0] );
  const auto res2 = engine2();
  CHECK( res2 );
  CHECK( to_index_list_string( *res2 ) == "{1 | 1 << 8 | 0 << 16, 2}" );

  /* inverter */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string(   care, "00110110" );
  kitty::create_from_binary_string( tts[0], "00000101" );
  xag_resyn_engine<kitty::partial_truth_table> engine3( target, care, st, ps );
  engine3.add_divisor( tts[0] );
  const auto res3 = engine3();
  CHECK( res3 );
  CHECK( to_index_list_string( *res3 ) == "{1 | 1 << 8 | 0 << 16, 3}" );
}

TEST_CASE( "AIG resynthesis -- 1 <= k <= 3", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "11000000" );
  kitty::create_from_binary_string( tts[1], "00110000" );
  kitty::create_from_binary_string( tts[2], "01011111" ); // binate
  test_aig_kresub( target, tts, 1 ); // 1 | 2

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "11001100" ); // binate
  kitty::create_from_binary_string( tts[1], "11111100" );
  kitty::create_from_binary_string( tts[2], "00001100" );
  test_aig_kresub( target, tts, 1 ); // 2 & ~3

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "01110010" ); // binate
  kitty::create_from_binary_string( tts[1], "11111100" ); 
  kitty::create_from_binary_string( tts[2], "10000011" ); // binate
  test_aig_kresub( target, tts, 2 ); // 2 & (1 | 3)

  tts.emplace_back( 8 );
  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "01110010" ); // binate
  kitty::create_from_binary_string( tts[1], "00110011" ); // binate
  kitty::create_from_binary_string( tts[2], "10000011" ); // binate
  kitty::create_from_binary_string( tts[3], "11001011" ); // binate
  test_aig_kresub( target, tts, 3 ); // ~(2 & 4) & (1 | 3)
}

TEST_CASE( "AIG resynthesis -- recursive", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 6, kitty::partial_truth_table( 16 ) );
  kitty::partial_truth_table target( 16 );

  kitty::create_from_binary_string( target, "1111000011111111" ); // target
  kitty::create_from_binary_string( tts[0], "0111001000000000" ); // binate
  kitty::create_from_binary_string( tts[1], "0011001100000000" ); // binate
  kitty::create_from_binary_string( tts[2], "1000001100000000" ); // binate
  kitty::create_from_binary_string( tts[3], "1100101100000000" ); // binate
  kitty::create_from_binary_string( tts[4], "0000000011111111" ); // unate
  test_aig_kresub( target, tts, 4 ); // 5 | ( ~(2 & 4) & (1 | 3) )

  tts.emplace_back( 16 );
  kitty::create_from_binary_string( target, "1111000011111100" ); // target
  kitty::create_from_binary_string( tts[0], "0111001000000000" ); // binate
  kitty::create_from_binary_string( tts[1], "0011001100000000" ); // binate
  kitty::create_from_binary_string( tts[2], "1000001100000000" ); // binate
  kitty::create_from_binary_string( tts[3], "1100101100000000" ); // binate
  kitty::create_from_binary_string( tts[4], "0000000011111110" ); // binate
  kitty::create_from_binary_string( tts[5], "0000000011111101" ); // binate
  test_aig_kresub( target, tts, 5 ); // (5 & 6) | ( ~(2 & 4) & (1 | 3) )
}
