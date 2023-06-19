#include <catch.hpp>

#include <set>
#include <vector>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
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

  topo_view aig2{ aig };
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
  CHECK( aig.size() == 6 );
  CHECK( aig.num_gates() == 3 );

  topo_view aig2{ aig };
  nodes.clear();
  aig2.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 3 );
  CHECK( aig2.size() == 3 );
  CHECK( aig2.num_gates() == 0 );
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

  /* test topological order of nodes */
  std::vector<node<aig_network>> nodes;
  aig.foreach_node( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{ { 0, 1, 2, 3, 4, 5 } } );

  topo_view aig2{ aig };
  nodes.clear();
  aig2.foreach_node( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{ { 0, 1, 2, 3, 5, 4 } } );

  /* test topological order of gates */
  std::vector<node<aig_network>> gates;
  aig.foreach_gate( [&gates]( auto node ) { gates.push_back( node ); } );
  CHECK( gates == std::vector<node<aig_network>>{ { 4, 5 } } );

  gates.clear();
  aig2.foreach_gate( [&gates]( auto node ) { gates.push_back( node ); } );
  CHECK( gates == std::vector<node<aig_network>>{ { 5, 4 } } );

  /* test normalized index order */
  uint32_t counter = 0;
  aig2.foreach_node( [&aig2, &counter]( auto node ) {
    CHECK( aig2.node_to_index( node ) == counter++ );
  } );
}

TEST_CASE( "test reverse topo order", "[topo_view]" )
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

  /* test topological order of nodes */
  std::vector<node<aig_network>> nodes;
  aig.foreach_node( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{ { 0, 1, 2, 3, 4, 5 } } );

  topo_view aig2{ aig };
  nodes.clear();
  aig2.foreach_node_reverse( [&nodes]( auto node ) { nodes.push_back( node ); } );
  CHECK( nodes == std::vector<node<aig_network>>{ { 4, 5, 3, 2, 1, 0 } } );

  /* test topological order of gates */
  std::vector<node<aig_network>> gates;
  aig.foreach_gate( [&gates]( auto node ) { gates.push_back( node ); } );
  CHECK( gates == std::vector<node<aig_network>>{ { 4, 5 } } );

  gates.clear();
  aig2.foreach_gate_reverse( [&gates]( auto node ) { gates.push_back( node ); } );
  CHECK( gates == std::vector<node<aig_network>>{ { 4, 5 } } );
}