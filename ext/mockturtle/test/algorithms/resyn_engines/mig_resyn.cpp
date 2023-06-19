#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/partial_truth_table.hpp>

#include <mockturtle/algorithms/resyn_engines/mig_resyn.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/mig.hpp>

using namespace mockturtle;

template<class Engine>
void test_0resub()
{
  mig_resyn_stats st;
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );
  std::vector<uint32_t> divs = { 1, 2, 3 };

  kitty::create_from_binary_string( tts[0], "00110110" );
  kitty::create_from_binary_string( tts[1], "11111100" );
  kitty::create_from_binary_string( tts[2], "10000001" );
  kitty::create_from_binary_string( tts[3], "11001001" );

  Engine engine( st );
  const auto res = engine( tts[0], ~tts[0].construct(), divs.begin(), divs.end(), tts, 0 );
  CHECK( res );
  CHECK( ( *res ).num_gates() == 0u );
  CHECK( ( *res ).raw()[1] == 7u );
}

template<class Engine>
void test_1resub()
{
  mig_resyn_stats st;
  std::vector<kitty::partial_truth_table> tts( 3, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );
  std::vector<uint32_t> divs = { 0, 1, 2 };

  kitty::create_from_binary_string( target, "01110110" );
  kitty::create_from_binary_string( tts[0], "11110100" );
  kitty::create_from_binary_string( tts[1], "11001001" );
  kitty::create_from_binary_string( tts[2], "01000111" );

  Engine engine( st ); // target = <1,~2,3>
  const auto res = engine( target, ~target.construct(), divs.begin(), divs.end(), tts, 1 );
  CHECK( res );
  CHECK( ( *res ).num_gates() == 1u );

  mig_network mig;
  decode( mig, *res );
  partial_simulator sim( tts );
  const auto ans = simulate<kitty::partial_truth_table, mig_network, partial_simulator>( mig, sim )[0];
  CHECK( target == ans );
}

template<class Engine>
void test_2resub()
{
  mig_resyn_stats st;
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );
  std::vector<uint32_t> divs = { 0, 1, 2, 3 };

  kitty::create_from_binary_string( target, "00101110" );
  kitty::create_from_binary_string( tts[0], "11101111" );
  kitty::create_from_binary_string( tts[1], "00100000" );
  kitty::create_from_binary_string( tts[2], "10011110" );
  kitty::create_from_binary_string( tts[3], "01011111" );

  Engine engine( st ); // target = <<1,2,3>,2,4>
  const auto res = engine( target, ~target.construct(), divs.begin(), divs.end(), tts, 2 );
  CHECK( res );
  CHECK( ( *res ).num_gates() == 2u );

  mig_network mig;
  decode( mig, *res );
  partial_simulator sim( tts );
  const auto ans = simulate<kitty::partial_truth_table, mig_network, partial_simulator>( mig, sim )[0];
  CHECK( target == ans );
}

TEST_CASE( "MIG resynthesis engines -- 0-resub", "[mig_resyn]" )
{
  test_0resub<mig_resyn_bottomup<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_0resub<mig_resyn_topdown<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_0resub<mig_resyn_akers<mig_resyn_static_params>>();
}

TEST_CASE( "MIG resynthesis engines -- 1-resub", "[mig_resyn]" )
{
  test_1resub<mig_resyn_bottomup<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_1resub<mig_resyn_topdown<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_1resub<mig_resyn_akers<mig_resyn_static_params>>();
}

TEST_CASE( "MIG resynthesis engines -- 2-resub", "[mig_resyn]" )
{
  test_2resub<mig_resyn_bottomup<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_2resub<mig_resyn_topdown<kitty::partial_truth_table, mig_resyn_static_params>>();
  test_2resub<mig_resyn_akers<mig_resyn_static_params>>();
}
