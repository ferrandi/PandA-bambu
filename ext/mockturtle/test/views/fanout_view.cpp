#include <catch.hpp>

#include <set>

#include <mockturtle/traits.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/klut.hpp>
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
  test_fanout_view<klut_network>();
}

TEST_CASE( "compute fanout for AIG", "[fanout_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  fanout_view fanout_aig{aig};

  {
    std::set<node<aig_network>> nodes;
    fanout_aig.foreach_fanout( aig.get_node( a ), [&]( const auto& p ){ nodes.insert( p ); } );
    CHECK( nodes == std::set<node<aig_network>>{ aig.get_node( f1 ), aig.get_node( f2 ) } );
  }

  {
    std::set<node<aig_network>> nodes;
    fanout_aig.foreach_fanout( aig.get_node( b ), [&]( const auto& p ){ nodes.insert( p ); } );
    CHECK( nodes == std::set<node<aig_network>>{ aig.get_node( f1 ), aig.get_node( f3 ) } );
  }

  {
    std::set<node<aig_network>> nodes;
    fanout_aig.foreach_fanout( aig.get_node( f1 ), [&]( const auto& p ){ nodes.insert( p ); } );
    CHECK( nodes == std::set<node<aig_network>>{ aig.get_node( f2 ), aig.get_node( f3 ) } );
  }

  {
    std::set<node<aig_network>> nodes;
    fanout_aig.foreach_fanout( aig.get_node( f2 ), [&]( const auto& p ){ nodes.insert( p ); } );
    CHECK( nodes == std::set<node<aig_network>>{ aig.get_node( f4 ) } );
  }

  {
    std::set<node<aig_network>> nodes;
    fanout_aig.foreach_fanout( aig.get_node( f3 ), [&]( const auto& p ){ nodes.insert( p ); } );
    CHECK( nodes == std::set<node<aig_network>>{ aig.get_node( f4 ) } );
  }
}
