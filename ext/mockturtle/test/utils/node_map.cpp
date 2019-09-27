#include <catch.hpp>

#include <cstdint>
#include <vector>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/utils/node_map.hpp>

using namespace mockturtle;

TEST_CASE( "create vector node map for full adder", "[node_map]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );

  aig.create_po( sum );
  aig.create_po( carry );

  node_map<uint32_t, aig_network> map( aig );

  aig.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  uint32_t total{0};
  aig.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ( aig.size() * ( aig.size() - 1 ) ) / 2 );

  map.reset( 1 );
  total = 0;
  aig.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == aig.size() );
}

TEST_CASE( "create unordered node map for full adder", "[node_map]" )
{
  mig_network mig;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto [sum, carry] = full_adder( mig, a, b, c );

  mig.create_po( sum );
  mig.create_po( carry );

  unordered_node_map<uint32_t, mig_network> map( mig );
  mig.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  mig.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  mig.foreach_node( [&]( auto n ) {
    CHECK( map.has( n ) );
  } );

  uint32_t total{0};
  mig.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ( mig.size() * ( mig.size() - 1 ) ) / 2 );

  map.reset();
  mig.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  mig.foreach_node( [&]( auto n ) {
    map[n] = 1;
  } );

  total = 0;
  mig.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == mig.size() );
}
