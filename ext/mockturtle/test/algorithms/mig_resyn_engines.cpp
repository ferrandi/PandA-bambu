#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/partial_truth_table.hpp>
#include <kitty/operations.hpp>

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/resyn_engines/mig_resyn_engines.hpp>
#include <mockturtle/algorithms/simulation.hpp>

using namespace mockturtle;

template<class Engine>
void test_0resub()
{
  mig_resyn_engine_stats st;
  mig_resyn_engine_params ps;
  ps.max_size = 0;
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );

  kitty::create_from_binary_string( tts[0], "00110110" );
  kitty::create_from_binary_string( tts[1], "11111100" );
  kitty::create_from_binary_string( tts[2], "10000001" );
  kitty::create_from_binary_string( tts[3], "11001001" );

  Engine engine( tts[0], ~tts[0].construct(), st, ps );
  engine.add_divisor( 1, tts );
  engine.add_divisor( 2, tts );
  engine.add_divisor( 3, tts );

  const auto res = engine();
  CHECK( res );
  CHECK( (*res).num_gates() == 0u );
  CHECK( (*res).raw()[1] == 7u );
}

template<class Engine>
void test_1resub()
{
  mig_resyn_engine_stats st;
  mig_resyn_engine_params ps;
  ps.max_size = 1;
  std::vector<kitty::partial_truth_table> tts( 3, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );

  kitty::create_from_binary_string( target, "01110110" );
  kitty::create_from_binary_string( tts[0], "11110100" );
  kitty::create_from_binary_string( tts[1], "11001001" );
  kitty::create_from_binary_string( tts[2], "01000111" );

  Engine engine( target, ~target.construct(), st, ps );
  for ( auto i = 0u; i < tts.size(); ++i )
  {
    engine.add_divisor( i, tts );
  }
  // target = <1,~2,3>

  const auto res = engine();
  CHECK( res );
  CHECK( (*res).num_gates() == 1u );

  mig_network mig;
  decode( mig, *res );
  partial_simulator sim( tts );
  const auto ans = simulate<kitty::partial_truth_table, mig_network, partial_simulator>( mig, sim )[0];
  CHECK( target == ans );
}

template<class Engine>
void test_2resub()
{
  mig_resyn_engine_stats st;
  mig_resyn_engine_params ps;
  ps.max_size = 2;
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );

  kitty::create_from_binary_string( target, "00101110" );
  kitty::create_from_binary_string( tts[0], "11101111" );
  kitty::create_from_binary_string( tts[1], "00100000" );
  kitty::create_from_binary_string( tts[2], "10011110" );
  kitty::create_from_binary_string( tts[3], "01011111" );

  Engine engine( target, ~target.construct(), st, ps );
  for ( auto i = 0u; i < tts.size(); ++i )
  {
    engine.add_divisor( i, tts );
  }
  // target = <<1,2,3>,2,4>

  const auto res = engine();
  CHECK( res );
  CHECK( (*res).num_gates() == 2u );

  mig_network mig;
  decode( mig, *res );
  partial_simulator sim( tts );
  const auto ans = simulate<kitty::partial_truth_table, mig_network, partial_simulator>( mig, sim )[0];
  CHECK( target == ans );
}

TEST_CASE( "MIG resynthesis engines -- 0-resub", "[mig_resyn]" )
{
  test_0resub<mig_resyn_engine_bottom_up<kitty::partial_truth_table>>();
  test_0resub<mig_resyn_engine<kitty::partial_truth_table>>();
  test_0resub<mig_resyn_engine_akers>();
}

TEST_CASE( "MIG resynthesis engines -- 1-resub", "[mig_resyn]" )
{
  test_1resub<mig_resyn_engine_bottom_up<kitty::partial_truth_table>>();
  test_1resub<mig_resyn_engine<kitty::partial_truth_table>>();
  test_1resub<mig_resyn_engine_akers>();
}

TEST_CASE( "MIG resynthesis engines -- 2-resub", "[mig_resyn]" )
{
  test_2resub<mig_resyn_engine_bottom_up<kitty::partial_truth_table>>();
  test_2resub<mig_resyn_engine<kitty::partial_truth_table>>();
  test_2resub<mig_resyn_engine_akers>();
}
