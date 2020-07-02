#include <catch.hpp>

#include <vector>

#include <mockturtle/algorithms/detail/minmc_xags.hpp>
#include <mockturtle/algorithms/equivalence_classes.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/index_list.hpp>
#include <mockturtle/networks/xag.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/spectral.hpp>

using namespace mockturtle;

TEST_CASE( "Synthesize MAJ from AND", "[equivalence_classes]" )
{
  kitty::dynamic_truth_table func( 3u );
  kitty::create_majority( func );

  std::vector<kitty::detail::spectral_operation> transformations;
  const auto repr = kitty::hybrid_exact_spectral_canonization( func, [&]( auto const& _transformations ) {
    transformations = _transformations;
  } );

  auto expected = func.construct();
  kitty::create_from_expression( expected, "(ab)" );
  CHECK( repr == expected );

  xag_network xag;
  std::vector<xag_network::signal> pis( func.num_vars() );
  std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

  const auto f = apply_spectral_transformations( xag, transformations, pis, [&]( xag_network& ntk, std::vector<xag_network::signal> const& leaves ) {
    return ntk.create_and( leaves[0], leaves[1] );
  } );
  xag.create_po( f );

  CHECK( simulate<kitty::dynamic_truth_table>( xag, {static_cast<uint32_t>( func.num_vars() )} )[0] == func );
}

TEST_CASE( "Synthesize from database for 4-input functions", "[equivalence_classes]" )
{
  /* build database */
  std::unordered_map<kitty::dynamic_truth_table, std::vector<uint32_t> const*, kitty::hash<kitty::dynamic_truth_table>> db;
  for ( auto const& [cls, tt, list, expr] : detail::minmc_xags[4u] )
  {
    (void)cls;
    (void)expr;
    kitty::dynamic_truth_table key( 4u );
    kitty::create_from_words( key, &tt, &tt + 1 );
    db[key] = &list;
  }

  for ( auto i = 0u; i < 100u; ++i )
  {
    kitty::dynamic_truth_table func( 4u );
    kitty::create_random( func );

    std::vector<kitty::detail::spectral_operation> transformations;
    const auto repr = kitty::hybrid_exact_spectral_canonization( func, [&]( auto const& _transformations ) {
      transformations = _transformations;
    } );
    CHECK( repr == kitty::spectral_representative( func ) );

    xag_network xag;
    std::vector<xag_network::signal> pis( func.num_vars() );
    std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

    const auto f = apply_spectral_transformations( xag, transformations, pis, [&]( xag_network& ntk, std::vector<xag_network::signal> const& leaves ) {
      return create_from_binary_index_list( ntk, db[repr]->begin(), leaves.begin() )[0u];
    } );
    xag.create_po( f );
    CHECK( simulate<kitty::dynamic_truth_table>( xag, {static_cast<uint32_t>( func.num_vars() )} )[0] == func );
  }
}
