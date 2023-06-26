#include <catch.hpp>

#include <set>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/names_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_create_names_view()
{
  Ntk ntk;
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_and( b, c );
  auto const f = ntk.create_and( t1, t2 );
  ntk.create_po( f );

  CHECK( has_get_network_name_v<names_view<Ntk>> );
  CHECK( has_set_network_name_v<names_view<Ntk>> );
  CHECK( has_has_name_v<names_view<Ntk>> );
  CHECK( has_get_name_v<names_view<Ntk>> );
  CHECK( has_set_name_v<names_view<Ntk>> );
  CHECK( has_has_output_name_v<names_view<Ntk>> );
  CHECK( has_get_output_name_v<names_view<Ntk>> );
  CHECK( has_set_output_name_v<names_view<Ntk>> );

  names_view<Ntk> named_ntk{ ntk, "network" };

  CHECK( named_ntk.get_network_name() == "network" );

  named_ntk.set_network_name( "named network" );

  CHECK( named_ntk.get_network_name() == "named network" );

  CHECK( !named_ntk.has_name( a ) );
  CHECK( !named_ntk.has_name( b ) );
  CHECK( !named_ntk.has_name( c ) );
  CHECK( !named_ntk.has_output_name( 0 ) );

  named_ntk.set_name( a, "a" );
  named_ntk.set_name( b, "b" );
  named_ntk.set_name( c, "c" );
  named_ntk.set_output_name( 0, "f" );

  CHECK( named_ntk.has_name( a ) );
  CHECK( named_ntk.has_name( b ) );
  CHECK( named_ntk.has_name( c ) );
  CHECK( named_ntk.has_output_name( 0 ) );

  CHECK( named_ntk.get_name( a ) == "a" );
  CHECK( named_ntk.get_name( b ) == "b" );
  CHECK( named_ntk.get_name( c ) == "c" );
  CHECK( named_ntk.get_output_name( 0 ) == "f" );
}

TEST_CASE( "create names view and test API", "[names_view]" )
{
  test_create_names_view<aig_network>();
  test_create_names_view<mig_network>();
  test_create_names_view<xag_network>();
  test_create_names_view<xmg_network>();
  test_create_names_view<klut_network>();
}

template<typename Ntk>
void test_copy_names_view()
{
  Ntk ntk;
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_and( b, c );
  auto const f = ntk.create_and( t1, t2 );
  ntk.create_po( f );

  names_view<Ntk> named_ntk;
  named_ntk.set_name( a, "a" );
  named_ntk.set_name( b, "b" );
  named_ntk.set_name( c, "c" );
  named_ntk.set_output_name( 0, "f" );

  CHECK( named_ntk.has_name( a ) );
  CHECK( named_ntk.has_name( b ) );
  CHECK( named_ntk.has_name( c ) );
  CHECK( named_ntk.has_output_name( 0 ) );

  names_view<Ntk> new_named_ntk = named_ntk;
  CHECK( new_named_ntk.has_name( a ) );
  CHECK( new_named_ntk.has_name( b ) );
  CHECK( new_named_ntk.has_name( c ) );
  CHECK( new_named_ntk.has_output_name( 0 ) );

  CHECK( named_ntk.get_name( a ) == "a" );
  CHECK( named_ntk.get_name( b ) == "b" );
  CHECK( named_ntk.get_name( c ) == "c" );
  CHECK( named_ntk.get_output_name( 0 ) == "f" );
}

TEST_CASE( "copy names", "[names_view]" )
{
  test_copy_names_view<aig_network>();
  test_copy_names_view<mig_network>();
  test_copy_names_view<xag_network>();
  test_copy_names_view<xmg_network>();
  test_copy_names_view<klut_network>();
}
