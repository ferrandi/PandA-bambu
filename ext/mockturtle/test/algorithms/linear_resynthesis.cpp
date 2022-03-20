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

  const auto f1 = simulate<kitty::static_truth_table<7u>>( xag );
  const auto f2 = simulate<kitty::static_truth_table<7u>>( xag2 );
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

  const auto f1 = simulate<kitty::static_truth_table<5u>>( xag );
  const auto f2 = simulate<kitty::static_truth_table<5u>>( xag2 );
  for ( auto i = 0u; i < f1.size(); ++i )
  {
    CHECK( f1[i] == f2[i] );
  }
}

TEST_CASE( "Extract linear matrix from linear network", "[linear_resynthesis]" )
{
  xag_network xag;
  std::vector<xag_network::signal> xs( 5u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );

  xag.create_po( xag.create_nary_xor( xs ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[2], xs[3]} ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[1], xs[2], xs[4]} ) );
  xag.create_po( xag.create_nary_xor( {xs[2], xs[3], xs[4]} ) );
  xag.create_po( xag.create_nary_xor( {xs[0], xs[4]} ) );

  const auto matrix = get_linear_matrix( xag );

  std::vector<std::vector<bool>> expected = {
    {true, true, true, true, true},
    {true, true, true, true, false},
    {true, true, true, false, true},
    {false, false, true, true, true},
    {true, false, false, false, true}
  };

  CHECK( xag.num_gates() == 9u );
  CHECK( matrix == expected );

  const auto xag_optimized = *exact_linear_resynthesis( xag );
  CHECK( xag_optimized.num_gates() == 6u );
}

TEST_CASE( "Exact linear synthesis with SAT (example from paper)", "[linear_resynthesis]" )
{
  std::vector<std::vector<bool>> matrix = {
    {true, true, true, true, true},
    {true, true, true, true, false},
    {true, true, true, false, true},
    {false, false, true, true, true},
    {true, false, false, false, true}
  };

  const auto xag = *exact_linear_synthesis<xag_network>( matrix );

  CHECK( xag.num_gates() == 6u );
}

TEST_CASE( "More difficult example", "[linear_resynthesis]" )
{
  std::vector<std::vector<bool>> matrix = {
      {false, true, false, false, false, false, true, true},
      {false, false, false, true, false, false, false, false},
      {true, true, true, true, false, false, false, false},
      {false, true, false, true, true, true, false, false},
      {true, false, false, true, true, true, false, false},
      {false, true, false, false, false, false, true, false},
      {true, true, false, false, false, false, true, false}};

  exact_linear_synthesis_params ps;
  ps.conflict_limit = 10000;
  const auto xag = *exact_linear_synthesis<xag_network>( matrix, ps );

  CHECK( get_linear_matrix( xag ) == matrix );
  CHECK( xag.num_gates() == 10u );
}

TEST_CASE( "More difficult example with upper bound", "[linear_resynthesis]" )
{
  std::vector<std::vector<bool>> matrix = {
      {false, true, false, false, false, false, true, true},
      {false, false, false, true, false, false, false, false},
      {true, true, true, true, false, false, false, false},
      {false, true, false, true, true, true, false, false},
      {true, false, false, true, true, true, false, false},
      {false, true, false, false, false, false, true, false},
      {true, true, false, false, false, false, true, false}};

  exact_linear_synthesis_params ps;
  ps.conflict_limit = 10000;
  ps.upper_bound = 15u;
  const auto xag = *exact_linear_synthesis<xag_network>( matrix, ps );

  CHECK( get_linear_matrix( xag ) == matrix );
  CHECK( xag.num_gates() == 10u );
}

TEST_CASE( "Example from SEA'10 Boyar-Peralta paper; with cancellations", "[linear_resynthesis]" )
{
  std::vector<std::vector<bool>> matrix = {
    {true, true, false, false},
    {true, true, true, false},
    {true, true, true, true},
    {false, true, true, true}
  };

  const auto xag = *exact_linear_synthesis<xag_network>( matrix );

  CHECK( get_linear_matrix( xag ) == matrix );
  CHECK( xag.num_gates() == 4u );
}

TEST_CASE( "Example from SEA'10 Boyar-Peralta paper; cancellation-free", "[linear_resynthesis]" )
{
  std::vector<std::vector<bool>> matrix = {
    {true, true, false, false},
    {true, true, true, false},
    {true, true, true, true},
    {false, true, true, true}
  };

  exact_linear_synthesis_params ps;
  ps.cancellation_free = true;
  const auto xag = *exact_linear_synthesis<xag_network>( matrix, ps );

  CHECK( get_linear_matrix( xag ) == matrix );
  CHECK( xag.num_gates() == 5u );
}
