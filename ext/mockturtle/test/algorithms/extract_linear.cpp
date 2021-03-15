#include <catch.hpp>

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/extract_linear.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Extract linear part from MAJ XAG", "[extract_linear]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();
  const auto w1 = xag.create_xor( a, b );
  const auto w2 = xag.create_xor( b, c );
  const auto w3 = xag.create_and( w1, w2 );
  const auto f = xag.create_xor( w3, b );
  xag.create_po( f );

  CHECK( 3u == xag.num_pis() );
  CHECK( 1u == xag.num_pos() );
  CHECK( 4u == xag.num_gates() );

  const auto [linxag, signals] = extract_linear_circuit( xag );

  CHECK( 4u == linxag.num_pis() );
  CHECK( 3u == linxag.num_pos() );
  CHECK( 3u == linxag.num_gates() );

  const auto xag2 = merge_linear_circuit( linxag, 1u );

  CHECK( 3u == xag2.num_pis() );
  CHECK( 1u == xag2.num_pos() );
  CHECK( 4u == xag2.num_gates() );

  const auto func = simulate<kitty::static_truth_table<3u>>( xag2 )[0u];
  CHECK( 0xe8u == func._bits );
}
