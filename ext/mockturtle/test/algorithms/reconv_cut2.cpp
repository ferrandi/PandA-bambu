#include <catch.hpp>

#include <iostream>
#include <set>

#include <mockturtle/algorithms/reconv_cut2.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/networks/aig.hpp>

using namespace mockturtle;

TEST_CASE( "generate reconvergence-driven cuts for an AIG", "[cut_generation]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  using set_t = std::set<node<aig_network>>;

  fanout_view<aig_network> aig_fanout_view( aig );
  depth_view<fanout_view<aig_network>> aig_view( aig_fanout_view );
  cut_manager<depth_view<fanout_view<aig_network>>> mgr( 6 );

  auto leaves = [&]( const auto& s, uint32_t size = 1000u ){
    mgr.node_size_max = size;
    const auto leaves = reconv_driven_cut( mgr, aig_view, aig.get_node( s ) );
    return set_t( leaves.begin(), leaves.end() );
  };

  CHECK( leaves( a ) == set_t{} );
  CHECK( leaves( b ) == set_t{} );
  CHECK( leaves( f1 ) == set_t{aig.get_node( a ), aig.get_node( b )} );
  CHECK( leaves( f2 ) == set_t{aig.get_node( a ), aig.get_node( b )} );
  CHECK( leaves( f3 ) == set_t{aig.get_node( a ), aig.get_node( b )} );
  CHECK( leaves( f4 ) == set_t{aig.get_node( a ), aig.get_node( b )} );
  CHECK( leaves( f4, 1u ) == set_t{} );
  CHECK( leaves( f4, 2u ) == set_t{aig.get_node( f2 ), aig.get_node( f3 )} );
  CHECK( leaves( f4, 3u ) == set_t{aig.get_node( a ), aig.get_node( b )} );
}
