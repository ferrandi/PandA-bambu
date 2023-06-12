#include <catch.hpp>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/node_map.hpp>

#include <cstdint>
#include <vector>

using namespace mockturtle;

template<typename Ntk>
void test_vector_node_map()
{
  /* create a full adder in a network */
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  /* create a (vector) node map */
  node_map<uint32_t, Ntk> map{ ntk };

  ntk.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  uint32_t total{ 0 };
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ( ntk.size() * ( ntk.size() - 1 ) ) / 2 );

  /* reset all values to 1 */
  map.reset( 1 );

  total = 0;
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ntk.size() );
}

template<typename Ntk>
void test_hash_node_map()
{
  /* create a full adder in a network */
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  /* create (hash) node map */
  unordered_node_map<uint32_t, Ntk> map{ ntk };
  ntk.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  ntk.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  ntk.foreach_node( [&]( auto n ) {
    CHECK( map.has( n ) );
  } );

  uint32_t total{ 0 };
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ( ntk.size() * ( ntk.size() - 1 ) ) / 2 );

  /* reset all values to 1 */
  map.reset();
  ntk.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  ntk.foreach_node( [&]( auto n ) {
    map[n] = 1;
  } );

  total = 0;
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ntk.size() );
}

template<typename Ntk>
void test_incomplete_node_map()
{
  /* create a full adder in a network */
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  /* create incomplete node map */
  incomplete_node_map<uint32_t, Ntk> map{ ntk };
  ntk.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  ntk.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  ntk.foreach_node( [&]( auto n ) {
    CHECK( map.has( n ) );
  } );

  uint32_t total{ 0 };
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ( ntk.size() * ( ntk.size() - 1 ) ) / 2 );

  /* reset all values to 1 */
  map.reset();
  ntk.foreach_node( [&]( auto n ) {
    CHECK( !map.has( n ) );
  } );

  ntk.foreach_node( [&]( auto n ) {
    map[n] = 1;
  } );

  total = 0;
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );

  CHECK( total == ntk.size() );

  /* test erase */
  map.erase( a );
  CHECK( !map.has( a ) );

  /* test resize */
  const auto d = ntk.create_pi();
  map.resize();
  CHECK( !map.has( d ) );

  map[d] = map[a] = 1;
  total = 0;
  ntk.foreach_node( [&]( auto n ) {
    total += map[n];
  } );
  CHECK( total == ntk.size() );

  /* reset with initial value 10 */
  map.reset( 10 );
  /* create with initial value 10 */
  incomplete_node_map<uint32_t, Ntk> map2( ntk, 10 );

  ntk.foreach_node( [&]( auto n ) {
    CHECK( map[n] == map2[n] );
  } );
}

template<typename Ntk, typename Container>
void test_copy_ctor()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, Container> map( ntk );
  ntk.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  node_map<uint32_t, Ntk, Container> another_map{ map }; /* copy ctor */
  CHECK( map.size() == another_map.size() );
  ntk.foreach_node( [&]( auto n ) {
    CHECK( map[n] == another_map[n] );
  } );
}

template<typename Ntk, typename Container>
void test_move_ctor()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, Container> map( ntk );
  ntk.foreach_node( [&]( auto n, auto i ) {
    map[n] = i;
  } );

  auto const size_before = map.size();
  std::vector<uint32_t> values_before( size_before );
  ntk.foreach_node( [&]( auto n, auto i ) {
    values_before[i] = map[n];
  } );

  node_map<uint32_t, Ntk, Container> another_map{ std::move( map ) }; /* move ctor */
  CHECK( another_map.size() == size_before );
  ntk.foreach_node( [&]( auto n, auto i ) {
    CHECK( values_before[i] == another_map[n] );
  } );
}

template<typename Ntk>
void test_copy_assign_vector()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, std::vector<uint32_t>> map{ ntk };
  CHECK( map.size() == ntk.size() );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    map[n] = 42;
  } );

  node_map<uint32_t, Ntk, std::vector<uint32_t>> another_map{ ntk };
  CHECK( another_map.size() == ntk.size() );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == 0 );
  } );

  another_map = map;

  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == map[n] );
  } );
}

template<typename Ntk>
void test_copy_assign_hash_map()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, std::unordered_map<node<Ntk>, uint32_t>> map{ ntk };
  CHECK( map.size() == 0 );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    map[n] = 42;
  } );

  node_map<uint32_t, Ntk, std::unordered_map<node<Ntk>, uint32_t>> another_map{ ntk };
  CHECK( another_map.size() == 0 );
  another_map = map;

  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == map[n] );
  } );
}

template<typename Ntk>
void test_move_assign_vector()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, std::vector<uint32_t>> map{ ntk };
  CHECK( map.size() == ntk.size() );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    map[n] = 42;
  } );

  node_map<uint32_t, Ntk, std::vector<uint32_t>> another_map{ ntk };
  CHECK( another_map.size() == ntk.size() );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == 0 );
  } );

  another_map = std::move( map );

  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == 42 );
  } );
}

template<typename Ntk>
void test_move_assign_hash_map()
{
  Ntk ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto [sum, carry] = full_adder( ntk, a, b, c );

  ntk.create_po( sum );
  ntk.create_po( carry );

  node_map<uint32_t, Ntk, std::unordered_map<node<Ntk>, uint32_t>> map{ ntk };
  CHECK( map.size() == 0 );
  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    map[n] = 42;
  } );

  node_map<uint32_t, Ntk, std::unordered_map<node<Ntk>, uint32_t>> another_map{ ntk };
  CHECK( another_map.size() == 0 );
  another_map = std::move( map );

  ntk.foreach_node( [&]( node<Ntk> const& n ) {
    CHECK( another_map[n] == 42 );
  } );
}

TEST_CASE( "create vector node map for full adder", "[node_map]" )
{
  test_vector_node_map<aig_network>();
  test_vector_node_map<mig_network>();
  test_vector_node_map<xag_network>();
  test_vector_node_map<xmg_network>();
  test_vector_node_map<klut_network>();
}

TEST_CASE( "create unordered node map for full adder", "[node_map]" )
{
  test_hash_node_map<aig_network>();
  test_hash_node_map<mig_network>();
  test_hash_node_map<xag_network>();
  test_hash_node_map<xmg_network>();
  test_hash_node_map<klut_network>();
}

TEST_CASE( "create incomplete node map for full adder", "[node_map]" )
{
  test_incomplete_node_map<aig_network>();
  test_incomplete_node_map<mig_network>();
  test_incomplete_node_map<xag_network>();
  test_incomplete_node_map<xmg_network>();
  test_incomplete_node_map<klut_network>();
}

TEST_CASE( "Copy construction", "[node_map]" )
{
  test_copy_ctor<aig_network, std::vector<uint32_t>>();
  test_copy_ctor<mig_network, std::vector<uint32_t>>();
  test_copy_ctor<xag_network, std::vector<uint32_t>>();
  test_copy_ctor<xmg_network, std::vector<uint32_t>>();
  test_copy_ctor<klut_network, std::vector<uint32_t>>();

  test_copy_ctor<aig_network, std::unordered_map<node<aig_network>, uint32_t>>();
  test_copy_ctor<mig_network, std::unordered_map<node<mig_network>, uint32_t>>();
  test_copy_ctor<xag_network, std::unordered_map<node<xag_network>, uint32_t>>();
  test_copy_ctor<xmg_network, std::unordered_map<node<xmg_network>, uint32_t>>();
  test_copy_ctor<klut_network, std::unordered_map<node<klut_network>, uint32_t>>();
}

TEST_CASE( "Move construction", "[node_map]" )
{
  test_move_ctor<aig_network, std::vector<uint32_t>>();
  test_move_ctor<mig_network, std::vector<uint32_t>>();
  test_move_ctor<xag_network, std::vector<uint32_t>>();
  test_move_ctor<xmg_network, std::vector<uint32_t>>();
  test_move_ctor<klut_network, std::vector<uint32_t>>();

  test_move_ctor<aig_network, std::unordered_map<node<aig_network>, uint32_t>>();
  test_move_ctor<mig_network, std::unordered_map<node<mig_network>, uint32_t>>();
  test_move_ctor<xag_network, std::unordered_map<node<xag_network>, uint32_t>>();
  test_move_ctor<xmg_network, std::unordered_map<node<xmg_network>, uint32_t>>();
  test_move_ctor<klut_network, std::unordered_map<node<klut_network>, uint32_t>>();
}

TEST_CASE( "Copy assignment", "[node_map]" )
{
  test_copy_assign_vector<aig_network>();
  test_copy_assign_vector<mig_network>();
  test_copy_assign_vector<xag_network>();
  test_copy_assign_vector<xmg_network>();
  test_copy_assign_vector<klut_network>();

  test_copy_assign_hash_map<aig_network>();
  test_copy_assign_hash_map<mig_network>();
  test_copy_assign_hash_map<xag_network>();
  test_copy_assign_hash_map<xmg_network>();
  test_copy_assign_hash_map<klut_network>();
}

TEST_CASE( "Move assignment", "[node_map]" )
{
  test_move_assign_vector<aig_network>();
  test_move_assign_vector<mig_network>();
  test_move_assign_vector<xag_network>();
  test_move_assign_vector<xmg_network>();
  test_move_assign_vector<klut_network>();

  test_move_assign_hash_map<aig_network>();
  test_move_assign_hash_map<mig_network>();
  test_move_assign_hash_map<xag_network>();
  test_move_assign_hash_map<xmg_network>();
  test_move_assign_hash_map<klut_network>();
}
