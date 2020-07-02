#include <catch.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_depth_view()
{
  CHECK( is_network_type_v<Ntk> );
  CHECK( !has_depth_v<Ntk> );
  CHECK( !has_level_v<Ntk> );

  using depth_ntk = depth_view<Ntk>;

  CHECK( is_network_type_v<depth_ntk> );
  CHECK( has_depth_v<depth_ntk> );
  CHECK( has_level_v<depth_ntk> );

  using depth_depth_ntk = depth_view<depth_ntk>;

  CHECK( is_network_type_v<depth_depth_ntk> );
  CHECK( has_depth_v<depth_depth_ntk> );
  CHECK( has_level_v<depth_depth_ntk> );
};

TEST_CASE( "create different depth views", "[depth_view]" )
{
  test_depth_view<aig_network>();
  test_depth_view<mig_network>();
  test_depth_view<klut_network>();
}

TEST_CASE( "compute depth and levels for AIG", "[depth_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  depth_view depth_aig{aig};
  CHECK( depth_aig.depth() == 3 );
  CHECK( depth_aig.level( aig.get_node( a ) ) == 0 );
  CHECK( depth_aig.level( aig.get_node( b ) ) == 0 );
  CHECK( depth_aig.level( aig.get_node( f1 ) ) == 1 );
  CHECK( depth_aig.level( aig.get_node( f2 ) ) == 2 );
  CHECK( depth_aig.level( aig.get_node( f3 ) ) == 2 );
  CHECK( depth_aig.level( aig.get_node( f4 ) ) == 3 );
}

TEST_CASE( "compute depth and levels for AIG with inverter costs", "[depth_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  depth_view_params ps;
  ps.count_complements = true;
  depth_view depth_aig{aig, {}, ps};
  CHECK( depth_aig.depth() == 6 );
  CHECK( depth_aig.level( aig.get_node( a ) ) == 0 );
  CHECK( depth_aig.level( aig.get_node( b ) ) == 0 );
  CHECK( depth_aig.level( aig.get_node( f1 ) ) == 1 );
  CHECK( depth_aig.level( aig.get_node( f2 ) ) == 3 );
  CHECK( depth_aig.level( aig.get_node( f3 ) ) == 3 );
  CHECK( depth_aig.level( aig.get_node( f4 ) ) == 5 );
}

TEST_CASE( "compute critical path information", "[depth_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();
  const auto e = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_and( c, f1 );
  const auto f3 = aig.create_and( d, e );
  const auto f = aig.create_and( f2, f3 );
  aig.create_po( f );

  depth_view depth_aig{aig};
  CHECK( !has_is_on_critical_path_v<decltype(aig)> );
  CHECK( has_is_on_critical_path_v<decltype(depth_aig)> );
  CHECK( depth_aig.is_on_critical_path( aig.get_node( a ) ) );
  CHECK( depth_aig.is_on_critical_path( aig.get_node( b ) ) );
  CHECK( !depth_aig.is_on_critical_path( aig.get_node( c ) ) );
  CHECK( !depth_aig.is_on_critical_path( aig.get_node( d ) ) );
  CHECK( !depth_aig.is_on_critical_path( aig.get_node( e ) ) );
  CHECK( depth_aig.is_on_critical_path( aig.get_node( f1 ) ) );
  CHECK( depth_aig.is_on_critical_path( aig.get_node( f2 ) ) );
  CHECK( !depth_aig.is_on_critical_path( aig.get_node( f3 ) ) );
  CHECK( depth_aig.is_on_critical_path( aig.get_node( f ) ) );
}

TEST_CASE( "compute levels during node construction", "[depth_view]" )
{
  depth_view<xag_network> dxag;

  const auto a = dxag.create_pi();
  const auto b = dxag.create_pi();
  const auto c = dxag.create_pi();

  dxag.create_po( dxag.create_xor( b, dxag.create_and( dxag.create_xor( a, b ), dxag.create_xor( b, c ) ) ) );

  CHECK( dxag.depth() == 3u );
}

TEST_CASE( "compute levels during node construction with cost function", "[depth_view]" )
{
  xag_network xag;
  depth_view<xag_network, mc_cost<xag_network>> dxag{xag};

  const auto a = dxag.create_pi();
  const auto b = dxag.create_pi();
  const auto c = dxag.create_pi();

  dxag.create_po( dxag.create_xor( b, dxag.create_and( dxag.create_xor( a, b ), dxag.create_xor( b, c ) ) ) );

  CHECK( dxag.depth() == 1u );
}

