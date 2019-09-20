#include <catch.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/linear_resynthesis.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Linear resynthesis with Paar algorithm", "[linear_resynthesis]" )
{
  xag_network xag;
  std::vector<xag_network::signal> xs( 7u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[2], xs[4], xs[6]} ) );
  xag.create_po( xag.create_nary_xor( {xs[1], xs[2], xs[4], xs[5]} ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[2]} ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[3], xs[4], xs[6]} ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[2], xs[3], xs[5], xs[6]} ) );

  const auto xag2 = linear_resynthesis_paar( xag );

  CHECK( 7u == xag2.num_pis() );
  CHECK( 5u == xag2.num_pos() );

  const auto f1 = simulate<kitty::static_truth_table<7>>( xag );
  const auto f2 = simulate<kitty::static_truth_table<7>>( xag2 );
  for ( auto i = 0u; i < f1.size(); ++i )
  {
    CHECK( f1[i] == f2[i] );
  }
}


TEST_CASE( "Linear resynthesis with Paar algorithm 2", "[linear_resynthesis]" )
{
  xag_network xag;
  std::vector<xag_network::signal> xs( 5u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[2]} ) );
  xag.create_po( xag.create_nary_xor( {xs[1], xs[2], xs[3]} ) );
  xag.create_po( xag.create_nary_xor( {xs[2], xs[3], xs[4]} ) );

  const auto xag2 = linear_resynthesis_paar( xag );

  CHECK( 5u == xag2.num_pis() );
  CHECK( 3u == xag2.num_pos() );

  const auto f1 = simulate<kitty::static_truth_table<5>>( xag );
  const auto f2 = simulate<kitty::static_truth_table<5>>( xag2 );
  for ( auto i = 0u; i < f1.size(); ++i )
  {
    CHECK( f1[i] == f2[i] );
  }
}
