#include <catch.hpp>

#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mapping_view.hpp>

using namespace mockturtle;

TEST_CASE( "LUT mapping of AIG", "[lut_mapping]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  mapping_view mapped_aig{ aig };

  CHECK( has_has_mapping_v<mapping_view<aig_network>> );
  CHECK( has_is_cell_root_v<mapping_view<aig_network>> );
  CHECK( has_clear_mapping_v<mapping_view<aig_network>> );
  CHECK( has_num_cells_v<mapping_view<aig_network>> );
  CHECK( has_add_to_mapping_v<mapping_view<aig_network>> );
  CHECK( has_remove_from_mapping_v<mapping_view<aig_network>> );
  CHECK( has_foreach_cell_fanin_v<mapping_view<aig_network>> );

  CHECK( !mapped_aig.has_mapping() );

  lut_mapping( mapped_aig );

  CHECK( mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 1 );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f1 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f2 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f3 ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( f4 ) ) );

  mapped_aig.clear_mapping();

  CHECK( !mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 0 );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f1 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f2 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f3 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f4 ) ) );
}

TEST_CASE( "LUT mapping of 2-LUT network", "[lut_mapping]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 2 ), b( 2 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.create_pi();

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view mapped_aig{ aig };
  lut_mapping( mapped_aig );

  CHECK( mapped_aig.num_cells() == 3 );
}

TEST_CASE( "LUT mapping of 8-LUT network", "[lut_mapping]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 8 ), b( 8 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.get_constant( false );

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view mapped_aig{ aig };
  lut_mapping( mapped_aig );

  CHECK( mapped_aig.num_cells() == 12 );
}

TEST_CASE( "LUT mapping of 64-LUT network", "[lut_mapping]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 64 ), b( 64 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.get_constant( false );

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view mapped_aig{ aig };
  lut_mapping( mapped_aig );

  CHECK( mapped_aig.num_cells() == 96 );
}

TEST_CASE( "LUT mapping with functions of full adder", "[lut_mapping]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );
  aig.create_po( sum );
  aig.create_po( carry );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  CHECK( has_cell_function_v<mapping_view<aig_network, true>> );
  CHECK( has_set_cell_function_v<mapping_view<aig_network, true>> );

  CHECK( mapped_aig.num_cells() == 2 );
  CHECK( mapped_aig.is_cell_root( aig.get_node( sum ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( carry ) ) );
  CHECK( mapped_aig.cell_function( aig.get_node( sum ) )._bits[0] == 0x96 );
  CHECK( mapped_aig.cell_function( aig.get_node( carry ) )._bits[0] == 0x17 );
}


TEST_CASE( "3-LUT mapping of a sequential k-LUT", "[sequential_lut_mapping]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto d = klut.create_pi();
  const auto e = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  const auto f2 = klut.create_or( f1, c );
  const auto f3 = klut.create_or( f2, d );
  const auto f4 = klut.create_or( f3, e ); // f4 = a+b+c+d+e

  const auto f5 = klut.create_ro(); // f5 <- f4
  const auto fo = klut.create_or( f5, d ); // fo = f5+d
  klut.create_po( fo );

  klut.create_ri( f4 ); // f5 <- f4

  /* check sequential network interfaces */
  using Ntk = decltype( klut );
  CHECK( has_foreach_po_v<Ntk> );
  CHECK( has_create_po_v<Ntk> );
  CHECK( has_create_pi_v<Ntk> );
  CHECK( has_create_ro_v<Ntk> );
  CHECK( has_create_ri_v<Ntk> );

  CHECK( klut.num_gates() == 5 );
  CHECK( klut.num_registers() == 1 );

  /* sequential mapping */
  mapping_view<Ntk, true> viewed{ klut };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 3;
  lut_mapping<decltype(viewed), true>( viewed, ps );
  auto mapped_klut = *collapse_mapped_network<Ntk>( viewed );

  CHECK( mapped_klut.num_gates() == 3 );
  CHECK( mapped_klut.num_registers() == 1 );
}

TEST_CASE( "6-LUT mapping of a sequential k-LUT", "[sequential_lut_mapping]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto d = klut.create_pi();
  const auto e = klut.create_pi();

  const auto f1 = klut.create_or( a, b );
  const auto f2 = klut.create_or( f1, c );
  const auto f3 = klut.create_or( f2, d );
  const auto f4 = klut.create_or( f3, e ); // f4 = a+b+c+d+e

  const auto f5 = klut.create_ro(); // f5 <- f4
  const auto fo = klut.create_or( f5, d ); // fo = f5+d
  klut.create_po( fo );

  klut.create_ri( f4 ); // f5 <- f4

  /* check sequential network interfaces */
  using Ntk = decltype( klut );
  CHECK( has_foreach_po_v<Ntk> );
  CHECK( has_create_po_v<Ntk> );
  CHECK( has_create_pi_v<Ntk> );
  CHECK( has_create_ro_v<Ntk> );
  CHECK( has_create_ri_v<Ntk> );

  CHECK( klut.num_gates() == 5 );
  CHECK( klut.num_registers() == 1 );

  /* sequential mapping */
  mapping_view<Ntk, true> viewed{ klut };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 6;
  lut_mapping<decltype(viewed), true>( viewed, ps );
  auto mapped_klut = *collapse_mapped_network<Ntk>( viewed );

  CHECK( mapped_klut.num_gates() == 2 );
  CHECK( mapped_klut.num_registers() == 1 );
}