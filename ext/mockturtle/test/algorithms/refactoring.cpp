#include <catch.hpp>

#include <mockturtle/algorithms/refactoring.hpp>
#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "Refactoring of bad MAJ", "[refactoring]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

TEST_CASE( "Refactoring with Akers synthesis", "[refactoring]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f = mig.create_maj( a, mig.create_maj( a, b, c ), c );
  mig.create_po( f );

  akers_resynthesis<mig_network> resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 5 );
  CHECK( mig.num_pis() == 3 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 1 );
}

namespace detail
{
template<class Ntk>
struct free_xor_cost
{
  uint32_t operator()( Ntk const& ntk, node<Ntk> const& n ) const
  {
    return ntk.is_xor( n ) ? 0 : 1;
  }
};
} // namespace detail

TEST_CASE( "Refactoring with bi-decomposition", "[refactoring]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f = xag.create_or( xag.create_and(a, xag.create_not(b)), xag.create_and( xag.create_not(a), b));
  xag.create_po( f );

  bidecomposition_resynthesis<xag_network> resyn;
  refactoring( xag, resyn );

  xag = cleanup_dangling( xag );

  CHECK( xag.size() == 4 );
  CHECK( xag.num_pis() == 2 );
  CHECK( xag.num_pos() == 1 );
  CHECK( xag.num_gates() == 1 );
}

TEST_CASE( "Refactoring with bi-decomposition and different cost (free xor)", "[refactoring]" )
{
  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f = xag.create_or( xag.create_and(a, xag.create_not(b)), xag.create_and( xag.create_not(a), b));
  xag.create_po( f );

  bidecomposition_resynthesis<xag_network> resyn;
  refactoring( xag, resyn, {}, nullptr, ::detail::free_xor_cost<xag_network>());

  xag = cleanup_dangling( xag );

  CHECK( xag.size() == 4 );
  CHECK( xag.num_pis() == 2 );
  CHECK( xag.num_pos() == 1 );
  CHECK( xag.num_gates() == 1 );
}

TEST_CASE( "Refactoring from constant", "[refactoring]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( false ) );

  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( false ) );
  } );
}

TEST_CASE( "Refactoring from inverted constant", "[refactoring]" )
{
  mig_network mig;
  mig.create_po( mig.get_constant( true ) );

  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 1 );
  CHECK( mig.num_pis() == 0 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( f == mig.get_constant( true ) );
  } );
}

TEST_CASE( "Refactoring from projection", "[refactoring]" )
{
  mig_network mig;
  mig.create_po( mig.create_pi() );

  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( !mig.is_complemented( f ) );
  } );
}

TEST_CASE( "Refactoring from inverted projection", "[refactoring]" )
{
  mig_network mig;
  mig.create_po( !mig.create_pi() );

  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );

  mig = cleanup_dangling( mig );

  CHECK( mig.size() == 2 );
  CHECK( mig.num_pis() == 1 );
  CHECK( mig.num_pos() == 1 );
  CHECK( mig.num_gates() == 0 );

  mig.foreach_po( [&]( auto const& f ) {
    CHECK( mig.get_node( f ) == 1 );
    CHECK( mig.is_complemented( f ) );
  } );
}
