#include <catch.hpp>

#include <optional>
#include <set>

#include <fmt/format.h>

#include <mockturtle/algorithms/aqfp/aqfp_db.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_fanout_resyn.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_node_resyn.hpp>
#include <mockturtle/algorithms/aqfp/aqfp_resynthesis.hpp>
#include <mockturtle/networks/aqfp.hpp>
#include <mockturtle/networks/klut.hpp>

#include <kitty/kitty.hpp>

using namespace mockturtle;

auto get_test_klut()
{
  mockturtle::klut_network klut;

  auto x1 = klut.create_pi();
  auto x2 = klut.create_pi();
  auto x3 = klut.create_pi();
  auto x4 = klut.create_pi();

  kitty::dynamic_truth_table tt_f1{ 4 };
  tt_f1._bits[0] = 0x1837;
  kitty::dynamic_truth_table tt_f2{ 4 };
  tt_f2._bits[0] = 0x1600;

  auto n1 = klut.create_node( { x1, x2, x3, x4 }, tt_f1 );
  auto n2 = klut.create_node( { x1, x2, x3, n1 }, tt_f2 );
  auto n3 = klut.create_node( { x1, x2, n1, x4 }, tt_f2 );
  auto n4 = klut.create_node( { x1, n1, x3, x4 }, tt_f2 );
  auto n5 = klut.create_node( { n1, x2, x3, x4 }, tt_f2 );

  klut.create_po( n2 );
  klut.create_po( n3 );
  klut.create_po( n4 );
  klut.create_po( n5 );

  return klut;
}

std::string get_database()
{
  return "2\n"
         "06b3\n"
         "2\n"
         "03030303\n"
         "40.0\n"
         "4 5 5 3 1 2 3 5 4 4 5 6 7 5 4 5 6 6 8 3 6 7 8\n"
         "3 1 2 0\n"
         "04040404\n"
         "46.0\n"
         "6 5 6 3 1 2 6 3 6 7 8 3 3 4 5 3 6 7 9 3 7 8 10 3 8 9 10\n"
         "3 1 2 0\n"
         "0016\n"
         "2\n"
         "01030303\n"
         "32.0\n"
         "3 5 4 5 1 2 3 4 4 3 5 6 7 5 4 4 5 6 7\n"
         "3 2 1 0\n"
         "02030303\n"
         "28.0\n"
         "3 5 3 3 1 2 3 3 4 5 6 5 3 4 5 6 7\n"
         "0 1 2 3\n";
}

TEST_CASE( "AQFP area-oriented resynthesis", "[aqfp_resyn]" )
{
  auto klut = get_test_klut();

  mockturtle::aqfp_assumptions assume = { false, false, true, 4u };
  std::unordered_map<uint32_t, double> gate_costs = { { 3u, 6.0 }, { 5u, 10.0 } };
  std::unordered_map<uint32_t, double> splitters = { { 1u, 2.0 }, { assume.splitter_capacity, 2.0 } };

  mockturtle::aqfp_db<> db( gate_costs, splitters );
  std::stringstream ss( get_database() );
  db.load_db( ss );

  mockturtle::aqfp_node_resyn_param ps{ assume, splitters, mockturtle::aqfp_node_resyn_strategy::area };
  mockturtle::aqfp_fanout_resyn fanout_resyn( assume );
  mockturtle::aqfp_node_resyn node_resyn( db, ps );

  mockturtle::aqfp_network aqfp;
  auto res = mockturtle::aqfp_resynthesis( aqfp, klut, node_resyn, fanout_resyn );

  mockturtle::aqfp_network_cost cost_fn( assume, gate_costs, splitters );

  decltype( res.node_level ) expected_node_level = {
      { 0, 0 },
      { 1, 0 },
      { 2, 0 },
      { 3, 0 },
      { 4, 0 },
      { 5, 2 },
      { 6, 2 },
      { 7, 2 },
      { 8, 3 },
      { 9, 5 },
      { 10, 2 },
      { 11, 6 },
      { 12, 6 },
      { 13, 6 },
      { 14, 7 },
      { 15, 6 },
      { 16, 6 },
      { 17, 7 },
      { 18, 6 },
      { 19, 6 },
      { 20, 7 },
  };
  CHECK( res.node_level == expected_node_level );

  decltype( res.po_level ) expected_po_level = {
      { 11, 6 },
      { 14, 7 },
      { 17, 7 },
      { 20, 7 },
  };
  CHECK( res.po_level == expected_po_level );

  auto actual_cost = cost_fn( aqfp, res.node_level, res.po_level );
  CHECK( 134u == actual_cost );
}

TEST_CASE( "AQFP delay-oriented resynthesis", "[aqfp_resyn]" )
{
  auto klut = get_test_klut();

  mockturtle::aqfp_assumptions assume = { false, false, true, 4u };
  std::unordered_map<uint32_t, double> gate_costs = { { 3u, 6.0 }, { 5u, 10.0 } };
  std::unordered_map<uint32_t, double> splitters = { { 1u, 2.0 }, { assume.splitter_capacity, 2.0 } };

  mockturtle::aqfp_db<> db( gate_costs, splitters );
  std::stringstream ss( get_database() );
  db.load_db( ss );

  mockturtle::aqfp_node_resyn_param ps{ assume, splitters, mockturtle::aqfp_node_resyn_strategy::delay };
  mockturtle::aqfp_node_resyn node_resyn( db, ps );
  mockturtle::aqfp_fanout_resyn fanout_resyn( assume );

  mockturtle::aqfp_network_cost cost_fn( assume, gate_costs, splitters );

  mockturtle::aqfp_network aqfp;
  auto res = mockturtle::aqfp_resynthesis( aqfp, klut, node_resyn, fanout_resyn );

  decltype( res.node_level ) expected_node_level = {
      { 0, 0 },
      { 1, 0 },
      { 2, 0 },
      { 3, 0 },
      { 4, 0 },
      { 5, 2 },
      { 6, 2 },
      { 7, 2 },
      { 8, 3 },
      { 9, 2 },
      { 10, 2 },
      { 11, 5 },
      { 12, 6 },
      { 13, 6 },
      { 14, 7 },
      { 15, 6 },
      { 16, 6 },
      { 17, 7 },
      { 18, 6 },
      { 19, 6 },
      { 20, 7 },
  };
  CHECK( res.node_level == expected_node_level );

  decltype( res.po_level ) expected_po_level = {
      { 11, 5 },
      { 14, 7 },
      { 17, 7 },
      { 20, 7 },
  };
  CHECK( res.po_level == expected_po_level );

  auto actual_cost = cost_fn( aqfp, res.node_level, res.po_level );
  CHECK( 142u == actual_cost );
}
