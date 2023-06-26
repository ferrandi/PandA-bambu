#include <catch.hpp>

#include <algorithm>
#include <array>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/majority.hpp>
#include <mockturtle/networks/mig.hpp>

using namespace mockturtle;

template<class Ntk, int Count>
inline std::pair<Ntk, std::array<typename Ntk::signal, Count>> init_network()
{
  Ntk ntk;
  std::array<typename Ntk::signal, Count> pis;
  std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
  return { ntk, pis };
}

template<class Ntk>
inline bool implements_majority( Ntk const& ntk )
{
  CHECK( ntk.num_pos() == 1u );

  default_simulator<kitty::dynamic_truth_table> sim( ntk.num_pis() );
  kitty::dynamic_truth_table maj( ntk.num_pis() );
  kitty::create_majority( maj );
  return maj == simulate<kitty::dynamic_truth_table>( ntk, sim )[0];
}

TEST_CASE( "build majority-5 with 4 gates", "[majority]" )
{
  auto [mig, pis] = init_network<mig_network, 5>();
  mig.create_po( majority5( mig, pis ) );

  CHECK( 4u == mig.num_gates() );
  CHECK( 5u == mig.num_pis() );
  CHECK( implements_majority( mig ) );
}

TEST_CASE( "build majority-7 with 7 gates", "[majority]" )
{
  auto [mig, pis] = init_network<mig_network, 7>();
  mig.create_po( majority7( mig, pis ) );

  CHECK( 7u == mig.num_gates() );
  CHECK( 7u == mig.num_pis() );
  CHECK( implements_majority( mig ) );
}

TEST_CASE( "build majority-9 with 13 gates and no inverters", "[majority]" )
{
  auto [mig, pis] = init_network<mig_network, 9>();
  mig.create_po( majority9_13( mig, pis ) );

  CHECK( 13u == mig.num_gates() );
  CHECK( 9u == mig.num_pis() );
  CHECK( implements_majority( mig ) );
}

TEST_CASE( "build majority-9 with 12 gates and no inverters", "[majority]" )
{
  auto [mig, pis] = init_network<mig_network, 9>();
  mig.create_po( majority9_12( mig, pis ) );

  CHECK( 12u == mig.num_gates() );
  CHECK( 9u == mig.num_pis() );
  CHECK( implements_majority( mig ) );
}
