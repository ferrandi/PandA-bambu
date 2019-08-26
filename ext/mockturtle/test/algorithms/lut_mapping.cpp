#include <catch.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
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
