#include <catch.hpp>

#include <set>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/fanout_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_fanout_view()
{
  CHECK( is_network_type_v<Ntk> );
  CHECK( !has_foreach_fanout_v<Ntk> );

  using fanout_ntk = fanout_view<Ntk>;

  CHECK( is_network_type_v<fanout_ntk> );
  CHECK( has_foreach_fanout_v<fanout_ntk> );

  using fanout_fanout_ntk = fanout_view<fanout_ntk>;

  CHECK( is_network_type_v<fanout_fanout_ntk> );
  CHECK( has_foreach_fanout_v<fanout_fanout_ntk> );
};

TEST_CASE( "create different fanout views", "[fanout_view]" )
{
  test_fanout_view<aig_network>();
  test_fanout_view<mig_network>();
  test_fanout_view<xag_network>();
  test_fanout_view<xmg_network>();
  test_fanout_view<klut_network>();
}

template<typename Ntk>
void test_fanout_computation()
{
  using node = node<Ntk>;
  using nodes_t = std::set<node>;

  Ntk ntk;
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const f1 = ntk.create_and( a, b );
  auto const f2 = ntk.create_and( a, f1 );
  auto const f3 = ntk.create_and( b, f1 );
  auto const f4 = ntk.create_and( f2, f3 );
  ntk.create_po( f4 );

  fanout_view fanout_ntk{ ntk };
  {
    nodes_t nodes;
    fanout_ntk.foreach_fanout( ntk.get_node( a ), [&]( const auto& p ) { nodes.insert( p ); } );
    CHECK( nodes == nodes_t{ ntk.get_node( f1 ), ntk.get_node( f2 ) } );
  }

  {
    nodes_t nodes;
    fanout_ntk.foreach_fanout( ntk.get_node( b ), [&]( const auto& p ) { nodes.insert( p ); } );
    CHECK( nodes == nodes_t{ ntk.get_node( f1 ), ntk.get_node( f3 ) } );
  }

  {
    nodes_t nodes;
    fanout_ntk.foreach_fanout( ntk.get_node( f1 ), [&]( const auto& p ) { nodes.insert( p ); } );
    CHECK( nodes == nodes_t{ ntk.get_node( f2 ), ntk.get_node( f3 ) } );
  }

  {
    nodes_t nodes;
    fanout_ntk.foreach_fanout( ntk.get_node( f2 ), [&]( const auto& p ) { nodes.insert( p ); } );
    CHECK( nodes == nodes_t{ ntk.get_node( f4 ) } );
  }

  {
    nodes_t nodes;
    fanout_ntk.foreach_fanout( ntk.get_node( f3 ), [&]( const auto& p ) { nodes.insert( p ); } );
    CHECK( nodes == nodes_t{ ntk.get_node( f4 ) } );
  }
}

TEST_CASE( "compute fanouts for network", "[fanout_view]" )
{
  test_fanout_computation<aig_network>();
  test_fanout_computation<xag_network>();
  test_fanout_computation<mig_network>();
  test_fanout_computation<xmg_network>();
  test_fanout_computation<klut_network>();
}

TEST_CASE( "compute fanouts during node construction after move ctor", "[fanout_view]" )
{
  xag_network xag{};
  auto tmp = new fanout_view<xag_network>{ xag };
  fanout_view<xag_network> fxag{ std::move( *tmp ) }; // move ctor
  delete tmp;

  auto const a = fxag.create_pi();
  auto const b = fxag.create_pi();
  auto const c = fxag.create_pi();
  auto const f = fxag.create_xor( b, fxag.create_and( fxag.create_xor( a, b ), fxag.create_xor( b, c ) ) );
  fxag.create_po( f );

  using node = node<xag_network>;
  xag.foreach_node( [&]( node const& n ) {
    std::set<node> fanouts;
    fxag.foreach_fanout( n, [&]( node const& fo ) {
      fanouts.insert( fo );
    } );

    /* `fanout_size` counts internal and external fanouts (POs), but `fanouts` only contains internal fanouts */
    CHECK( fanouts.size() + ( xag.get_node( f ) == n ) == xag.fanout_size( n ) );
  } );
}

TEST_CASE( "compute fanouts during node construction after copy ctor", "[fanout_view]" )
{
  xag_network xag{};
  auto tmp = new fanout_view<xag_network>{ xag };
  fanout_view<xag_network> fxag{ *tmp }; // copy ctor
  delete tmp;

  auto const a = fxag.create_pi();
  auto const b = fxag.create_pi();
  auto const c = fxag.create_pi();
  auto const f = fxag.create_xor( b, fxag.create_and( fxag.create_xor( a, b ), fxag.create_xor( b, c ) ) );
  fxag.create_po( f );

  using node = node<xag_network>;
  xag.foreach_node( [&]( node const& n ) {
    std::set<node> fanouts;
    fxag.foreach_fanout( n, [&]( node const& fo ) {
      fanouts.insert( fo );
    } );

    /* `fanout_size` counts internal and external fanouts (POs), but `fanouts` only contains internal fanouts */
    CHECK( fanouts.size() + ( xag.get_node( f ) == n ) == xag.fanout_size( n ) );
  } );
}

TEST_CASE( "compute fanouts during node construction after copy assignment", "[fanout_view]" )
{
  xag_network xag{};
  fanout_view<xag_network> fxag;
  {
    auto tmp = new fanout_view<xag_network>{ xag };
    fxag = *tmp; /* copy assignment */
    delete tmp;
  }

  auto const a = fxag.create_pi();
  auto const b = fxag.create_pi();
  auto const c = fxag.create_pi();
  auto const f = fxag.create_xor( b, fxag.create_and( fxag.create_xor( a, b ), fxag.create_xor( b, c ) ) );
  fxag.create_po( f );

  using node = node<xag_network>;
  xag.foreach_node( [&]( node const& n ) {
    std::set<node> fanouts;
    fxag.foreach_fanout( n, [&]( node const& fo ) {
      fanouts.insert( fo );
    } );

    /* `fanout_size` counts internal and external fanouts (POs), but `fanouts` only contains internal fanouts */
    CHECK( fanouts.size() + ( xag.get_node( f ) == n ) == xag.fanout_size( n ) );
  } );
}

TEST_CASE( "compute fanouts during node construction after move assignment", "[fanout_view]" )
{
  xag_network xag{};
  fanout_view<xag_network> fxag;
  {
    auto tmp = new fanout_view<xag_network>{ xag };
    fxag = std::move( *tmp ); /* move assignment */
    delete tmp;
  }

  auto const a = fxag.create_pi();
  auto const b = fxag.create_pi();
  auto const c = fxag.create_pi();
  auto const f = fxag.create_xor( b, fxag.create_and( fxag.create_xor( a, b ), fxag.create_xor( b, c ) ) );
  fxag.create_po( f );

  using node = node<xag_network>;
  xag.foreach_node( [&]( node const& n ) {
    std::set<node> fanouts;
    fxag.foreach_fanout( n, [&]( node const& fo ) {
      fanouts.insert( fo );
    } );

    /* `fanout_size` counts internal and external fanouts (POs), but `fanouts` only contains internal fanouts */
    CHECK( fanouts.size() + ( xag.get_node( f ) == n ) == xag.fanout_size( n ) );
  } );
}

TEST_CASE( "substitute node with dependency in fanout view", "[fanout_view]" )
{
  aig_network aig{};
  fanout_view faig( aig );

  auto const a = faig.create_pi();
  auto const b = faig.create_pi();
  auto const c = faig.create_pi();          /* place holder */
  auto const tmp = faig.create_and( b, c ); /* place holder */
  auto const f1 = faig.create_and( a, b );
  auto const f2 = faig.create_and( tmp, a );
  auto const f3 = faig.create_and( f1, f2 );
  faig.create_po( f3 );
  faig.substitute_node( faig.get_node( tmp ), f1 );

  /**
   * issue #545
   *
   *      f3
   *     /  \
   *    /   f2
   *    \  /  \
   *  1->f1    a
   *
   * stack:
   * 1. push (f3->f2)
   * 2. push (f2->a)
   * 3. pop (f2->a)
   * 4. pop (f3->f2) but, f2 is dead !!!
   */

  faig.substitute_node( faig.get_node( f1 ), faig.get_constant( 1 ) /* constant 1 */ );

  CHECK( faig.is_dead( faig.get_node( f1 ) ) );
  CHECK( faig.is_dead( faig.get_node( f2 ) ) );
  CHECK( faig.is_dead( faig.get_node( f3 ) ) );
  aig.foreach_po( [&]( auto s ) {
    CHECK( aig.is_dead( aig.get_node( s ) ) == false );
  } );
}

TEST_CASE( "substitute node with complemented node in fanout view", "[fanout_view]" )
{
  aig_network aig;
  fanout_view faig( aig );
  auto const x1 = faig.create_pi();
  auto const x2 = faig.create_pi();
  auto const f1 = faig.create_and( x1, x2 );
  auto const f2 = faig.create_and( x1, f1 );
  faig.create_po( f2 );

  CHECK( faig.fanout_size( faig.get_node( x1 ) ) == 2 );
  CHECK( faig.fanout_size( faig.get_node( x2 ) ) == 1 );
  CHECK( faig.fanout_size( faig.get_node( f1 ) ) == 1 );
  CHECK( faig.fanout_size( faig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( faig )[0]._bits == 0x8 );

  faig.substitute_node( faig.get_node( f2 ), !f2 );

  CHECK( faig.fanout_size( faig.get_node( x1 ) ) == 2 );
  CHECK( faig.fanout_size( faig.get_node( x2 ) ) == 1 );
  CHECK( faig.fanout_size( faig.get_node( f1 ) ) == 1 );
  CHECK( faig.fanout_size( faig.get_node( f2 ) ) == 1 );

  CHECK( simulate<kitty::static_truth_table<2u>>( faig )[0]._bits == 0x7 );
}