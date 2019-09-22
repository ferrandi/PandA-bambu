#include <catch.hpp>

#include <set>
#include <vector>

#include <mockturtle/traits.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/topo_view.hpp>

using namespace mockturtle;

TEST_CASE( "create a topo_view on an AIG", "[topo_view]" )
{
  aig_network aig;

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f = aig.create_xor( x1, x2 );
  aig.create_po( f );

  std::set<node<aig_network>> nodes;
  aig.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 6 );

  topo_view aig2{aig};
  nodes.clear();
  aig2.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 6 );
}

TEST_CASE( "create a topo_view on an AIG without output", "[topo_view]" )
{
  aig_network aig;

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  aig.create_xor( x1, x2 );

  std::set<node<aig_network>> nodes;
  aig.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 6 );

  topo_view aig2{aig};
  nodes.clear();
  aig2.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 3 );
}

TEST_CASE( "create a topo_view on an AIG without topo order", "[topo_view]" )
{
  aig_network aig;

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();
  const auto gate1 = aig.create_and( x1, x2 );
  const auto gate2 = aig.create_and( x3, gate1 );

  /* switch gate order on storage */
  aig._storage->nodes[aig.get_node( gate2 )].children[0].index = aig.get_node( x1 );
  aig._storage->nodes[aig.get_node( gate2 )].children[1].index = aig.get_node( x2 );

  aig._storage->nodes[aig.get_node( gate1 )].children[0].index = aig.get_node( x3 );
  aig._storage->nodes[aig.get_node( gate1 )].children[1].index = aig.get_node( gate2 );

  aig.create_po( gate1 );

  std::vector<node<aig_network>> nodes;
  aig.foreach_node( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{{0, 1, 2, 3, 4, 5}} );

  topo_view aig2{aig};
  nodes.clear();
  aig2.foreach_node( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{{0, 1, 2, 3, 5, 4}} );
}
