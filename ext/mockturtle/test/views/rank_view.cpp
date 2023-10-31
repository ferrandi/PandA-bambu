#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/cover.hpp>
#include <mockturtle/networks/crossed.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/rank_view.hpp>

#include <functional>
#include <memory>

using namespace mockturtle;

TEMPLATE_TEST_CASE( "traits", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  CHECK( is_network_type_v<TestType> );
  CHECK( !has_rank_position_v<TestType> );
  CHECK( !has_at_rank_position_v<TestType> );
  CHECK( !has_width_v<TestType> );
  CHECK( !has_sort_rank_v<TestType> );
  CHECK( !has_foreach_node_in_rank_v<TestType> );
  CHECK( !has_foreach_gate_in_rank_v<TestType> );
  CHECK( !is_topologically_sorted_v<TestType> );

  using rank_ntk = rank_view<TestType>;

  CHECK( is_network_type_v<rank_ntk> );
  CHECK( has_rank_position_v<rank_ntk> );
  CHECK( has_at_rank_position_v<rank_ntk> );
  CHECK( has_width_v<rank_ntk> );
  CHECK( has_sort_rank_v<rank_ntk> );
  CHECK( has_foreach_node_in_rank_v<rank_ntk> );
  CHECK( has_foreach_gate_in_rank_v<rank_ntk> );
  CHECK( is_topologically_sorted_v<rank_ntk> );

  using rank_rank_ntk = rank_view<rank_ntk>;

  CHECK( is_network_type_v<rank_rank_ntk> );
  CHECK( has_rank_position_v<rank_rank_ntk> );
  CHECK( has_at_rank_position_v<rank_rank_ntk> );
  CHECK( has_width_v<rank_rank_ntk> );
  CHECK( has_sort_rank_v<rank_rank_ntk> );
  CHECK( has_foreach_node_in_rank_v<rank_rank_ntk> );
  CHECK( has_foreach_gate_in_rank_v<rank_rank_ntk> );
  CHECK( is_topologically_sorted_v<rank_rank_ntk> );
}

TEMPLATE_TEST_CASE( "compute ranks for a simple network", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  TestType ntk{};
  auto const x1 = ntk.create_pi();
  auto const x2 = ntk.create_pi();
  auto const a1 = ntk.create_and( x1, x2 );
  ntk.create_po( a1 );

  rank_view rank_ntk{ ntk };

  REQUIRE( rank_ntk.size() == ntk.size() );

  CHECK( rank_ntk.width() == 2u );

  auto const x1_lvl = rank_ntk.level( rank_ntk.get_node( x1 ) );
  auto const x2_lvl = rank_ntk.level( rank_ntk.get_node( x2 ) );
  auto const a1_lvl = rank_ntk.level( rank_ntk.get_node( a1 ) );

  CHECK( x1_lvl == 0u );
  CHECK( x2_lvl == 0u );
  CHECK( a1_lvl == 1u );

  auto const x1_pos = rank_ntk.rank_position( rank_ntk.get_node( x1 ) );
  auto const x2_pos = rank_ntk.rank_position( rank_ntk.get_node( x2 ) );
  auto const a1_pos = rank_ntk.rank_position( rank_ntk.get_node( a1 ) );

  CHECK( ( x1_pos == 0u || x1_pos == 1u ) );
  CHECK( ( x2_pos == 0u || x2_pos == 1u ) );
  CHECK( a1_pos == 0u );

  CHECK( rank_ntk.at_rank_position( x1_lvl, x1_pos ) == rank_ntk.get_node( x1 ) );
  CHECK( rank_ntk.at_rank_position( x2_lvl, x2_pos ) == rank_ntk.get_node( x2 ) );
  CHECK( rank_ntk.at_rank_position( a1_lvl, a1_pos ) == rank_ntk.get_node( a1 ) );

  rank_ntk.foreach_node_in_rank( 0ul, [&]( auto const& n, auto i ) {
    switch (i)
    {
    case( 0ul ):
    {
      CHECK( n == rank_ntk.get_node( x1 ) );
      break;
    }
    case( 1ul ):
    {
      CHECK( n == rank_ntk.get_node( x2 ) );
      break;
    }
    default:
    {
      CHECK( false );
    }
    } } );

  rank_ntk.foreach_node_in_rank( 1ul, [&]( auto const& n ) { CHECK( n == rank_ntk.get_node( a1 ) ); } );

  rank_ntk.swap( rank_ntk.get_node( x1 ), rank_ntk.get_node( x2 ) );

  CHECK( rank_ntk.at_rank_position( x1_lvl, x1_pos ) == rank_ntk.get_node( x2 ) );
  CHECK( rank_ntk.at_rank_position( x2_lvl, x2_pos ) == rank_ntk.get_node( x1 ) );

  rank_ntk.foreach_node_in_rank( 0ul, [&]( auto const& n, auto i ) {
    switch (i)
    {
    case( 0ul ):
    {
      CHECK( n == rank_ntk.get_node( x2 ) );
      break;
    }
    case( 1ul ):
    {
      CHECK( n == rank_ntk.get_node( x1 ) );
      break;
    }
    default:
    {
      CHECK( false );
    }
    } } );
}

TEMPLATE_TEST_CASE( "compute ranks during node construction", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  depth_view const depth_ntk{ TestType{} };
  rank_view rank_ntk{ depth_ntk };

  auto const a = rank_ntk.create_pi();
  auto const b = rank_ntk.create_pi();
  auto const c = rank_ntk.create_pi();

  auto const a1 = rank_ntk.create_and( a, b );
  auto const a2 = rank_ntk.create_and( a1, c );
  rank_ntk.create_po( a2 );

  CHECK( rank_ntk.width() == 3u );

  rank_ntk.foreach_node_in_rank( 0ul, [&]( auto const& n, auto i ) {
    switch (i)
    {
    case( 0ul ):
    {
      CHECK( n == rank_ntk.get_node( a ) );
      break;
    }
    case( 1ul ):
    {
      CHECK( n == rank_ntk.get_node( b ) );
      break;
    }
    case ( 2ul ):
    {
      CHECK( n == rank_ntk.get_node( c ) );
      break;
    }
    default:
    {
      CHECK( false );
    }
    } } );

  rank_ntk.foreach_node_in_rank( 1ul, [&]( auto const& n, auto i ) {
    switch (i)
    {
    case( 0ul ):
    {
      CHECK( n == rank_ntk.get_node( a1 ) );
      break;
    }
    case( 1ul ):
    {
      CHECK( n == rank_ntk.get_node( a2 ) );
      break;
    }
    default:
    {
      CHECK( false );
    }
    } } );

  auto const a3 = rank_ntk.create_and( b, c );
  auto const a4 = rank_ntk.create_and( a, c );
  auto const o1 = rank_ntk.create_or( a, b );
  auto const o2 = rank_ntk.create_or( a3, a4 );
  rank_ntk.create_po( o1 );
  rank_ntk.create_po( o2 );

  CHECK( rank_ntk.width() == 4u );
}

TEMPLATE_TEST_CASE( "compute ranks during node construction after copy ctor", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  TestType ntk{};
  {
    auto tmp = std::make_unique<rank_view<TestType>>( ntk );
    CHECK( ntk.events().on_add.size() == 2u );

    rank_view cpy_rank{ *tmp }; // copy ctor
    CHECK( ntk.events().on_add.size() == 4u );

    tmp.reset(); // don't access tmp anymore after this line!
    CHECK( ntk.events().on_add.size() == 2u );

    auto const a = cpy_rank.create_pi();
    auto const b = cpy_rank.create_pi();
    auto const c = cpy_rank.create_pi();
    auto const t0 = cpy_rank.create_or( a, b );
    auto const t1 = cpy_rank.create_or( b, c );
    auto const t2 = cpy_rank.create_and( t0, t1 );
    auto const t3 = cpy_rank.create_or( b, t2 );
    cpy_rank.create_po( t3 );
    CHECK( cpy_rank.width() == 3u );

    auto const t4 = cpy_rank.create_and( a, c );
    auto const t5 = cpy_rank.create_or( a, c );
    auto const t6 = cpy_rank.create_and( t4, t5 );
    cpy_rank.create_po( t6 );
    CHECK( cpy_rank.width() == 4u );

    CHECK( ntk.events().on_add.size() == 2u );
  }

  CHECK( ntk.events().on_add.size() == 0u );
}

TEMPLATE_TEST_CASE( "compute ranks during node construction after copy assignment", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  rank_view<TestType> rank_ntk{};
  {
    auto tmp = std::make_unique<rank_view<TestType>>( rank_ntk );
    rank_ntk = *tmp; // copy assignment
    tmp.reset();
  }

  auto const a = rank_ntk.create_pi();
  auto const b = rank_ntk.create_pi();
  auto const c = rank_ntk.create_pi();
  rank_ntk.create_po( rank_ntk.create_or( b, rank_ntk.create_and( rank_ntk.create_or( a, b ), rank_ntk.create_or( b, c ) ) ) );

  CHECK( rank_ntk.width() == 3u );
}

TEMPLATE_TEST_CASE( "sort ranks according to a comparator", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  using Ntk = rank_view<TestType>;

  Ntk rank_ntk{};

  auto const a = rank_ntk.create_pi();
  auto const b = rank_ntk.create_pi();

  auto const a1 = rank_ntk.create_and( a, b );

  auto const c = rank_ntk.create_pi();

  auto const a2 = rank_ntk.create_and( a1, c );

  rank_ntk.create_po( a2 );

  // verify order before sorting
  REQUIRE( rank_ntk.rank_position( rank_ntk.get_node( a ) ) == 0u );
  REQUIRE( rank_ntk.rank_position( rank_ntk.get_node( b ) ) == 1u );
  REQUIRE( rank_ntk.rank_position( rank_ntk.get_node( c ) ) == 2u );

  REQUIRE( rank_ntk.at_rank_position( 0u, 0u ) == rank_ntk.get_node( a ) );
  REQUIRE( rank_ntk.at_rank_position( 0u, 1u ) == rank_ntk.get_node( b ) );
  REQUIRE( rank_ntk.at_rank_position( 0u, 2u ) == rank_ntk.get_node( c ) );

  // sort rank 0 in descending order
  rank_ntk.sort_rank( 0u, std::greater<node<Ntk>>{} );

  // check order after sorting
  CHECK( rank_ntk.rank_position( rank_ntk.get_node( a ) ) == 2u );
  CHECK( rank_ntk.rank_position( rank_ntk.get_node( b ) ) == 1u );
  CHECK( rank_ntk.rank_position( rank_ntk.get_node( c ) ) == 0u );

  CHECK( rank_ntk.at_rank_position( 0u, 2u ) == rank_ntk.get_node( a ) );
  CHECK( rank_ntk.at_rank_position( 0u, 0u ) == rank_ntk.get_node( c ) );
  CHECK( rank_ntk.at_rank_position( 0u, 1u ) == rank_ntk.get_node( b ) );

  // sort rank 0 in ascending order
  rank_ntk.sort_rank( 0u, std::less<node<Ntk>>{} );

  CHECK( rank_ntk.rank_position( rank_ntk.get_node( a ) ) == 0u );
  CHECK( rank_ntk.rank_position( rank_ntk.get_node( b ) ) == 1u );
  CHECK( rank_ntk.rank_position( rank_ntk.get_node( c ) ) == 2u );

  CHECK( rank_ntk.at_rank_position( 0u, 0u ) == rank_ntk.get_node( a ) );
  CHECK( rank_ntk.at_rank_position( 0u, 1u ) == rank_ntk.get_node( b ) );
  CHECK( rank_ntk.at_rank_position( 0u, 2u ) == rank_ntk.get_node( c ) );
}

template<typename Ntk>
void check_pi_permutation( Ntk const& ntk, std::vector<node<Ntk>> const& perm )
{
  REQUIRE( ntk.num_pis() == perm.size() );
  ntk.foreach_pi( [&]( auto const& n, auto const i ) {
    CHECK( n == perm[i] );
  } );
}

TEMPLATE_TEST_CASE( "sort primary inputs", "[rank_view]", aig_network, mig_network, xag_network, xmg_network, klut_network, cover_network, buffered_aig_network, buffered_mig_network, crossed_klut_network, buffered_crossed_klut_network )
{
  using Ntk = rank_view<TestType>;

  Ntk rank_ntk{};

  auto const a = rank_ntk.get_node( rank_ntk.create_pi() );
  auto const b = rank_ntk.get_node( rank_ntk.create_pi() );
  auto const c = rank_ntk.get_node( rank_ntk.create_pi() );
  auto const d = rank_ntk.get_node( rank_ntk.create_pi() );
  auto const e = rank_ntk.get_node( rank_ntk.create_pi() );
  // order is a, b, c, d, e
  check_pi_permutation( rank_ntk, { a, b, c, d, e } );

  rank_ntk.swap( a, b );
  // order is b, a, c, d, e
  check_pi_permutation( rank_ntk, { b, a, c, d, e } );

  rank_ntk.swap( c, d );
  // order is b, a, d, c, e
  check_pi_permutation( rank_ntk, { b, a, d, c, e } );

  rank_ntk.swap( c, e );
  // order is b, a, d, e, c
  check_pi_permutation( rank_ntk, { b, a, d, e, c } );
}
