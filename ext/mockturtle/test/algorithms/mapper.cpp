#include <catch.hpp>

#include <cstdint>
#include <vector>

#include <lorina/genlib.hpp>
#include <lorina/super.hpp>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/super_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/binding_view.hpp>

using namespace mockturtle;

std::string const test_library = "GATE   inv1    1 O=!a;            PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                 "GATE   inv2    2 O=!a;            PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                 "GATE   nand2   2 O=!(a*b);        PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
                                 "GATE   xor2    5 O=a^b;           PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "GATE   maj3    3 O=a*b+a*c+b*c;   PIN * INV 1 999 2.0 0.2 2.0 0.2\n"
                                 "GATE   buf     2 O=a;             PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                 "GATE   zero    0 O=CONST0;\n"
                                 "GATE   one     0 O=CONST1;";

std::string const super_library = "test.genlib\n"
                                  "3\n"
                                  "2\n"
                                  "6\n"
                                  "* nand2 1 0\n"
                                  "inv1 3\n"
                                  "* nand2 2 4\n"
                                  "\0";

TEST_CASE( "Map of MAJ3", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f = aig.create_maj( a, b, c );
  aig.create_po( f );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  CHECK( luts.size() == 6u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 1u );
  CHECK( luts.num_gates() == 1u );
  CHECK( st.area == 3.0f );
  CHECK( st.delay == 2.0f );
}

TEST_CASE( "Map of bad MAJ3 and constant output", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f = aig.create_maj( a, aig.create_maj( a, b, c ), c );
  aig.create_po( f );
  aig.create_po( aig.get_constant( true ) );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  CHECK( luts.size() == 6u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 2u );
  CHECK( luts.num_gates() == 1u );
  CHECK( st.area == 3.0f );
  CHECK( st.delay == 2.0f );
}

TEST_CASE( "Map of full adder 1", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );
  aig.create_po( sum );
  aig.create_po( carry );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 8u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 2u );
  CHECK( luts.num_gates() == 3u );
  CHECK( st.area > 13.0f - eps );
  CHECK( st.area < 13.0f + eps );
  CHECK( st.delay > 3.8f - eps );
  CHECK( st.delay < 3.8f + eps );
}

TEST_CASE( "Map of full adder 2", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3, classification_type::p_configurations> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );
  aig.create_po( sum );
  aig.create_po( carry );

  map_params ps;
  ps.cut_enumeration_ps.minimize_truth_table = false;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 8u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 2u );
  CHECK( luts.num_gates() == 3u );
  CHECK( st.area > 13.0f - eps );
  CHECK( st.area < 13.0f + eps );
  CHECK( st.delay > 3.8f - eps );
  CHECK( st.delay < 3.8f + eps );
}

TEST_CASE( "Map with inverters", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_and( !a, b );
  const auto f2 = aig.create_and( f1, !c );

  aig.create_po( f2 );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 11u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 1u );
  CHECK( luts.num_gates() == 6u );
  CHECK( st.area > 8.0f - eps );
  CHECK( st.area < 8.0f + eps );
  CHECK( st.delay > 4.7f - eps );
  CHECK( st.delay < 4.7f + eps );
}

TEST_CASE( "Map for inverters minimization", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f = aig.create_maj( !a, !b, !c );
  aig.create_po( f );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 7u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 1u );
  CHECK( luts.num_gates() == 2u );
  CHECK( st.area > 4.0f - eps );
  CHECK( st.area < 4.0f + eps );
  CHECK( st.delay > 2.9f - eps );
  CHECK( st.delay < 2.9f + eps );
}

TEST_CASE( "Map of buffer and constant outputs", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3, classification_type::np_configurations> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();

  const auto n5 = aig.create_and( a, d );
  const auto n6 = aig.create_and( a, !c );
  const auto n7 = aig.create_and( !c, n5 );
  const auto n8 = aig.create_and( c, n6 );
  const auto n9 = aig.create_and( !n6, n7 );
  const auto n10 = aig.create_and( n7, n8 );
  const auto n11 = aig.create_and( a, n10 );
  const auto n12 = aig.create_and( !d, n11 );
  const auto n13 = aig.create_and( !d, !n7 );
  const auto n14 = aig.create_and( !n6, !n7 );

  aig.create_po( aig.get_constant( true ) );
  aig.create_po( b );
  aig.create_po( n9 );
  aig.create_po( n12 );
  aig.create_po( !n13 );
  aig.create_po( n14 );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 10u );
  CHECK( luts.num_pis() == 4u );
  CHECK( luts.num_pos() == 6u );
  CHECK( luts.num_gates() == 4u );
  CHECK( st.area > 7.0f - eps );
  CHECK( st.area < 7.0f + eps );
  CHECK( st.delay > 1.9f - eps );
  CHECK( st.delay < 1.9f + eps );
}

TEST_CASE( "Map with supergates", "[mapper]" )
{
  std::vector<gate> gates;
  super_lib super_data;

  std::istringstream in_lib( test_library );
  auto result = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  std::istringstream in_super( super_library );
  result = lorina::read_super( in_super, super_reader( super_data ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3, classification_type::p_configurations> lib( gates, super_data );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto n4 = aig.create_and( a, b );
  const auto n5 = aig.create_and( b, c );
  const auto f = aig.create_and( n4, n5 );
  aig.create_po( f );

  map_params ps;
  map_stats st;
  binding_view<klut_network> luts = map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 9u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 1u );
  CHECK( luts.num_gates() == 4u );
  CHECK( st.area == 6.0f );
  CHECK( st.delay > 3.8f - eps );
  CHECK( st.delay < 3.8f + eps );
}

TEST_CASE( "Map of sequential AIG", "[mapper]" )
{
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  tech_library<3, classification_type::np_configurations> lib( gates );

  sequential<aig_network> aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_ro(); // f2 <- f1
  const auto f3 = aig.create_xor( f2, c );

  aig.create_po( f3 );
  aig.create_ri( f1 ); // f3 <- f1

  CHECK( aig.num_gates() == 4u );
  CHECK( aig.num_registers() == 1u );
  CHECK( aig.num_pis() == 3u );
  CHECK( aig.num_pos() == 1u );

  map_params ps;
  map_stats st;
  binding_view<sequential<klut_network>> luts = seq_map( aig, lib, ps, &st );

  const float eps{ 0.005f };

  CHECK( luts.size() == 8u );
  CHECK( luts.num_pis() == 3u );
  CHECK( luts.num_pos() == 1u );
  CHECK( luts.num_registers() == 1u );
  CHECK( luts.num_gates() == 2u );
  CHECK( st.area == 7.0f );
  CHECK( st.delay > 1.9f - eps );
  CHECK( st.delay < 1.9f + eps );
}

TEST_CASE( "Exact map of bad MAJ3 and constant output", "[mapper]" )
{
  mig_npn_resynthesis resyn{ true };

  exact_library<mig_network, mig_npn_resynthesis> lib( resyn );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f = aig.create_maj( a, aig.create_maj( a, b, c ), c );
  aig.create_po( f );
  aig.create_po( aig.get_constant( true ) );
  aig.create_po( a );

  map_params ps;
  map_stats st;
  mig_network mig = map( aig, lib, ps, &st );

  CHECK( mig.size() == 5u );
  CHECK( mig.num_pis() == 3u );
  CHECK( mig.num_pos() == 3u );
  CHECK( mig.num_gates() == 1u );
  CHECK( st.area == 1.0f );
  CHECK( st.delay == 1.0f );
}

TEST_CASE( "Exact map of full adder", "[mapper]" )
{
  xmg_npn_resynthesis resyn;

  exact_library<xmg_network, xmg_npn_resynthesis> lib( resyn );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );
  aig.create_po( sum );
  aig.create_po( carry );

  map_params ps;
  map_stats st;
  xmg_network xmg = map( aig, lib, ps, &st );

  CHECK( xmg.size() == 7u );
  CHECK( xmg.num_pis() == 3u );
  CHECK( xmg.num_pos() == 2u );
  CHECK( xmg.num_gates() == 3u );
  CHECK( st.area == 3.0f );
  CHECK( st.delay == 2.0f );
}

TEST_CASE( "Exact map should avoid cycles", "[mapper]" )
{
  using resyn_fn = xag_npn_resynthesis<aig_network>;

  resyn_fn resyn;

  exact_library<aig_network, resyn_fn> lib( resyn );

  aig_network aig;
  const auto x0 = aig.create_pi();
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto n0 = aig.create_and( x1, !x2 );
  const auto n1 = aig.create_and( !x0, n0 );
  const auto n2 = aig.create_and( x0, !n0 );
  const auto n3 = aig.create_and( !n1, !n2 );
  const auto n4 = aig.create_and( x1, x2 );
  const auto n5 = aig.create_and( x0, !n4 );
  const auto n6 = aig.create_and( !x0, n4 );
  const auto n7 = aig.create_and( !n5, !n6 );
  aig.create_po( n3 );
  aig.create_po( n7 );

  map_params ps;
  map_stats st;
  aig_network res = map( aig, lib, ps, &st );

  CHECK( res.size() == 12 );
  CHECK( res.num_pis() == 3 );
  CHECK( res.num_pos() == 2 );
  CHECK( res.num_gates() == 8 );
  CHECK( st.area == 8.0f );
  CHECK( st.delay == 3.0f );
}

TEST_CASE( "Exact map with logic sharing", "[mapper]" )
{
  using resyn_fn = xag_npn_resynthesis<aig_network>;

  resyn_fn resyn;

  exact_library<aig_network, resyn_fn> lib( resyn );

  aig_network aig;
  const auto x0 = aig.create_pi();
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();

  const auto n0 = aig.create_and( x0, !x1 );
  const auto n1 = aig.create_and( x2, x3 );
  const auto n2 = aig.create_and( x1, x2 );
  const auto n3 = aig.create_and( n0, n1 );
  const auto n4 = aig.create_and( n2, x3 );
  aig.create_po( !n3 );
  aig.create_po( !n4 );

  map_params ps;
  ps.enable_logic_sharing = true;
  map_stats st;
  aig_network res = map( aig, lib, ps, &st );

  CHECK( res.size() == 9 );
  CHECK( res.num_pis() == 4 );
  CHECK( res.num_pos() == 2 );
  CHECK( res.num_gates() == 4 );
}

TEST_CASE( "exact map of sequential AIG", "[mapper]" )
{
  using resyn_fn = xag_npn_resynthesis<xag_network>;

  resyn_fn resyn;
  exact_library<sequential<xag_network>, resyn_fn> lib( resyn );

  sequential<aig_network> aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_ro(); // f2 <- f1
  const auto f3 = aig.create_xor( f2, c );

  aig.create_po( f3 );
  aig.create_ri( f1 ); // f3 <- f1

  map_params ps;
  map_stats st;
  sequential<xag_network> res = map( aig, lib, ps, &st );

  CHECK( res.size() == 7u );
  CHECK( res.num_pis() == 3u );
  CHECK( res.num_pos() == 1u );
  CHECK( res.num_registers() == 1u );
  CHECK( res.num_gates() == 2u );
}
