#include <catch.hpp>

#include <optional>
#include <set>

#include <fmt/format.h>

#include <mockturtle/algorithms/aqfp/detail/dag.hpp>
#include <mockturtle/algorithms/aqfp/detail/dag_cost.hpp>
#include <mockturtle/algorithms/aqfp/detail/dag_gen.hpp>
#include <mockturtle/algorithms/aqfp/detail/dag_util.hpp>

using namespace mockturtle;

TEST_CASE( "Partition generation", "[aqfp_resyn]" )
{
  using part = std::multiset<int>;
  using partition = std::multiset<part>;
  using partition_set = std::set<partition>;

  detail::partition_generator<int> partition_gen;

  /* General partitioning with distinct elements and unit max counts with unlimited part count and unlimited part size */
  const auto t1a = partition_gen( { 0, 1, 2, 3 }, { 1u, 1u, 1u, 1u } );
  const auto t1b = partition_gen( { 0, 1, 2, 3 }, { 1u, 1u, 1u, 1u }, 0u, 0u );
  const partition_set s1 = {
      { { 0 }, { 1 }, { 2 }, { 3 } },
      { { 0 }, { 1 }, { 2, 3 } },
      { { 0 }, { 1, 2 }, { 3 } },
      { { 0 }, { 1, 2, 3 } },
      { { 0 }, { 1, 3 }, { 2 } },
      { { 0, 1 }, { 2 }, { 3 } },
      { { 0, 1 }, { 2, 3 } },
      { { 0, 1, 2 }, { 3 } },
      { { 0, 1, 2, 3 } },
      { { 0, 1, 3 }, { 2 } },
      { { 0, 2 }, { 1 }, { 3 } },
      { { 0, 2 }, { 1, 3 } },
      { { 0, 2, 3 }, { 1 } },
      { { 0, 3 }, { 1 }, { 2 } },
      { { 0, 3 }, { 1, 2 } } };
  CHECK( t1a == t1b );
  CHECK( t1a == s1 );

  /* Duplicate element and unit max counts */
  const auto t2 = partition_gen( { 0, 1, 1, 3 }, { 1u, 1u, 1u, 1u }, 0u );
  const partition_set s2 = {
      { { 0 }, { 1 }, { 1 }, { 3 } },
      { { 0 }, { 1 }, { 1, 3 } },
      { { 0, 1 }, { 1 }, { 3 } },
      { { 0, 1 }, { 1, 3 } },
      { { 0, 1, 3 }, { 1 } },
      { { 0, 3 }, { 1 }, { 1 } } };
  CHECK( t2 == s2 );

  /* Duplicate element and unit max counts with limited part count */
  const auto t3 = partition_gen( { 0, 1, 1, 3 }, { 1u, 1u, 1u, 1u }, 2u );
  const partition_set s3 = {
      { { 0, 1 }, { 1, 3 } },
      { { 0, 1, 3 }, { 1 } } };
  CHECK( t3 == s3 );

  /* Duplicate element and unit max counts with limited part size */
  const auto t4 = partition_gen( { 0, 1, 1, 3 }, { 1u, 1u, 1u, 1u }, 0u, 2u );
  const partition_set s4 = {
      { { 0 }, { 1 }, { 1 }, { 3 } },
      { { 0 }, { 1 }, { 1, 3 } },
      { { 0, 1 }, { 1 }, { 3 } },
      { { 0, 1 }, { 1, 3 } },
      { { 0, 3 }, { 1 }, { 1 } } };
  CHECK( t4 == s4 );

  /* Duplicate element and unit max counts with limited part count and limited part size */
  const auto t5 = partition_gen( { 0, 1, 1, 3 }, { 1u, 1u, 1u, 1u }, 2u, 2u );
  const partition_set s5 = {
      { { 0, 1 }, { 1, 3 } } };
  CHECK( t5 == s5 );

  /* Duplicate element and max count of 2 for that element */
  const auto t6 = partition_gen( { 0, 1, 1, 3 }, { 1u, 2u, 1u, 1u } );
  const partition_set s6 = {
      { { 0 }, { 1 }, { 1 }, { 3 } },
      { { 0 }, { 1 }, { 1, 3 } },
      { { 0 }, { 1, 1 }, { 3 } },
      { { 0 }, { 1, 1, 3 } },
      { { 0, 1 }, { 1 }, { 3 } },
      { { 0, 1 }, { 1, 3 } },
      { { 0, 1, 1 }, { 3 } },
      { { 0, 1, 1, 3 } },
      { { 0, 1, 3 }, { 1 } },
      { { 0, 3 }, { 1 }, { 1 } },
      { { 0, 3 }, { 1, 1 } } };
  CHECK( t6 == s6 );

  /* Duplicate element and max count of 2 for that element with limited part size */
  const auto t7 = partition_gen( { 0, 1, 1, 3 }, { 1u, 2u, 1u, 1u }, 0u, 2u );
  const partition_set s7 = {
      { { 0 }, { 1 }, { 1 }, { 3 } },
      { { 0 }, { 1 }, { 1, 3 } },
      { { 0 }, { 1, 1 }, { 3 } },
      { { 0, 1 }, { 1 }, { 3 } },
      { { 0, 1 }, { 1, 3 } },
      { { 0, 3 }, { 1 }, { 1 } },
      { { 0, 3 }, { 1, 1 } } };
  CHECK( t7 == s7 );
}

TEST_CASE( "Partition extension", "[aqfp_resyn]" )
{
  using part = std::multiset<int>;
  using partition = std::multiset<part>;
  using partition_set = std::set<partition>;

  detail::partition_extender<int> partition_ext;

  auto t1 = partition_ext( { 0, 1, 2, 3, 4 }, {}, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s1 = {};
  CHECK( t1 == s1 );

  auto t2 = partition_ext( {}, { { 1, 2 }, { 3, 4 } }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s2 = {
      { { 1, 2 }, { 3, 4 } } };
  CHECK( t2 == s2 );

  auto t3 = partition_ext( { 0, 1, 2, 3, 4 }, { {} }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s3 = {
      { { 0, 1, 2, 3, 4 } } };
  CHECK( t3 == s3 );

  auto t4 = partition_ext( { 4 }, { { 1 }, { 2 }, { 3 }, { 4 } }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s4 = {
      { { 1 }, { 2 }, { 3, 4 }, { 4 } },
      { { 1 }, { 2, 4 }, { 3 }, { 4 } },
      { { 1, 4 }, { 2 }, { 3 }, { 4 } } };
  CHECK( t4 == s4 );

  auto t5 = partition_ext( { 4 }, { { 1 }, { 2 }, { 3 }, { 4 } }, { 1u, 1u, 1u, 1u, 2u } );
  partition_set s5 = {
      { { 1 }, { 2 }, { 3 }, { 4, 4 } },
      { { 1 }, { 2 }, { 3, 4 }, { 4 } },
      { { 1 }, { 2, 4 }, { 3 }, { 4 } },
      { { 1, 4 }, { 2 }, { 3 }, { 4 } } };
  CHECK( t5 == s5 );

  auto t6 = partition_ext( { 3, 4 }, { { 0 }, { 1 }, { 2 } }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s6 = {
      { { 0 }, { 1 }, { 2, 3, 4 } },
      { { 0 }, { 1, 3 }, { 2, 4 } },
      { { 0 }, { 1, 3, 4 }, { 2 } },
      { { 0 }, { 1, 4 }, { 2, 3 } },
      { { 0, 3 }, { 1 }, { 2, 4 } },
      { { 0, 3 }, { 1, 4 }, { 2 } },
      { { 0, 3, 4 }, { 1 }, { 2 } },
      { { 0, 4 }, { 1 }, { 2, 3 } },
      { { 0, 4 }, { 1, 3 }, { 2 } } };
  CHECK( t6 == s6 );

  auto t7 = partition_ext( { 0, 1, 2 }, { { 3 }, { 4 } }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s7 = {
      { { 0, 1, 2, 3 }, { 4 } },
      { { 0, 1, 2, 4 }, { 3 } },
      { { 0, 1, 3 }, { 2, 4 } },
      { { 0, 1, 4 }, { 2, 3 } },
      { { 0, 2, 3 }, { 1, 4 } },
      { { 0, 2, 4 }, { 1, 3 } },
      { { 0, 3 }, { 1, 2, 4 } },
      { { 0, 4 }, { 1, 2, 3 } } };
  CHECK( t7 == s7 );

  auto t8 = partition_ext( { 3, 4, 4, 4 }, { { 0 }, { 1 }, { 2 } }, { 1u, 1u, 1u, 1u, 1u } );
  partition_set s8 = {
      { { 0, 3, 4 }, { 1, 4 }, { 2, 4 } },
      { { 0, 4 }, { 1, 3, 4 }, { 2, 4 } },
      { { 0, 4 }, { 1, 4 }, { 2, 3, 4 } } };
  CHECK( t8 == s8 );
}

TEST_CASE( "Sublist generation", "[aqfp_resyn]" )
{
  mockturtle::detail::sublist_generator<int> sublist_gen;

  auto t1 = sublist_gen( { 0 } );
  std::set<std::vector<int>> s1 = { {}, { 0 } };
  CHECK( t1 == s1 );

  auto t2 = sublist_gen( { 0, 0, 0, 0 } );
  std::set<std::vector<int>> s2 = { {}, { 0 }, { 0, 0 }, { 0, 0, 0 }, { 0, 0, 0, 0 } };
  CHECK( t2 == s2 );

  auto t3 = sublist_gen( { 0, 1, 2, 3 } );
  std::set<std::vector<int>> s3 = {
      {}, { 0 }, { 0, 1 }, { 0, 1, 2 }, { 0, 1, 2, 3 }, { 0, 1, 3 }, { 0, 2 }, { 0, 2, 3 }, { 0, 3 }, { 1 }, { 1, 2 }, { 1, 2, 3 }, { 1, 3 }, { 2 }, { 2, 3 }, { 3 } };
  CHECK( t3 == s3 );

  auto t4 = sublist_gen( { 0, 2, 2, 2 } );
  std::set<std::vector<int>> s4 = { {}, { 0 }, { 0, 2 }, { 0, 2, 2 }, { 0, 2, 2, 2 }, { 2 }, { 2, 2 }, { 2, 2, 2 } };
  CHECK( t4 == s4 );

  auto t5 = sublist_gen( { 0, 0, 2, 2, 2 } );
  std::set<std::vector<int>> s5 = { {}, { 0 }, { 0, 0 }, { 0, 0, 2 }, { 0, 0, 2, 2 }, { 0, 0, 2, 2, 2 }, { 0, 2 }, { 0, 2, 2 }, { 0, 2, 2, 2 }, { 2 }, { 2, 2 }, { 2, 2, 2 } };
  CHECK( t5 == s5 );
}

#if !__clang__ || __clang_major__ > 10
TEST_CASE( "DAG generation", "[aqfp_resyn]" )
{
  using Ntk = mockturtle::aqfp_dag<>;

  mockturtle::dag_generator_params params;

  params.max_gates = 3u;         // allow at most 3 gates in total
  params.max_num_fanout = 1000u; // limit the maximum fanout of a gate
  params.max_width = 1000u;      // maximum number of gates at any level
  params.max_num_in = 4u;        // maximum number of inputs slots (need extra one for the constant)
  params.max_levels = 3u;        // maximum number of gate levels in a DAG

  params.allowed_num_fanins = { 3u, 5u };
  params.max_gates_of_fanin = { { 3u, 3u }, { 5u, 1u } };

  std::vector<Ntk> generated_dags;

  mockturtle::dag_generator<> gen( params, 1u );
  gen.for_each_dag( [&]( const auto& dag, auto thread_id ) { (void)thread_id; generated_dags.push_back( dag ); } );

  CHECK( generated_dags.size() == 3018u );
}
#endif

TEST_CASE( "Computing gate cost", "[aqfp_resyn]" )
{
  mockturtle::dag_gate_cost<mockturtle::aqfp_dag<>> gate_cc( { { 3u, 6.0 }, { 5u, 10.0 } } );

  mockturtle::aqfp_dag<> net1 = { { { 1, 4, 5 }, { 2, 4, 5 }, { 3, 5, 6 }, { 5, 7, 8 }, {}, {}, {}, {}, {} }, { 4, 5, 6, 7, 8 }, 5u };
  mockturtle::aqfp_dag<> net2 = { { { 1, 2, 6 }, { 4, 5, 6 }, { 3, 6, 7 }, { 6, 8, 9 }, { 6, 7, 10 }, { 6, 8, 9 }, {}, {}, {}, {}, {} }, { 6, 7, 8, 9, 10 }, 0u };

  CHECK( gate_cc( net1 ) == 24.0 );
  CHECK( gate_cc( net2 ) == 36.0 );
}

TEST_CASE( "Computing AQFP cost", "[aqfp_resyn]" )
{
  mockturtle::dag_aqfp_cost<mockturtle::aqfp_dag<>> aqfp_cc( { { 3u, 3.0 }, { 5u, 5.0 } }, { { 1u, 1.0 }, { 3u, 3.0 } } );

  mockturtle::aqfp_dag<> net1 = { { { 1, 4, 5 }, { 2, 4, 5 }, { 3, 5, 6 }, { 5, 7, 8 }, {}, {}, {}, {}, {} }, { 4, 5, 6, 7, 8 }, 5u };
  mockturtle::aqfp_dag<> net2 = { { { 1, 2, 6 }, { 4, 5, 6 }, { 3, 6, 7 }, { 6, 8, 9 }, { 6, 7, 10 }, { 6, 8, 9 }, {}, {}, {}, {}, {} }, { 6, 7, 8, 9, 10 }, 0u };

  CHECK( aqfp_cc( net1 ) == 18.0 );
  CHECK( aqfp_cc( net2 ) == 42.0 );
}
