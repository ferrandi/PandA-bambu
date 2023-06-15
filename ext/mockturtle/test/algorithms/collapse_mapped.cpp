#include <catch.hpp>

#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

using namespace mockturtle;

TEST_CASE( "Mapped AIG into k-LUT network", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut_opt = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut_opt );
  auto const& klut = *klut_opt;
  CHECK( klut.size() == 5 );
  CHECK( klut.num_gates() == 1 );
  CHECK( mapped_aig.num_cells() == 1 );

  kitty::dynamic_truth_table tt_xor( 2 );
  kitty::create_from_hex_string( tt_xor, "6" );
  klut.foreach_node( [&]( auto n ) {
    if ( klut.is_constant( n ) || klut.is_pi( n ) )
      return;
    CHECK( klut.node_function( n ) == tt_xor );
  } );
}

TEST_CASE( "Mapped AIG into k-LUT network with no function", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  mapping_view<aig_network, false> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, false>, false>( mapped_aig );

  const auto klut_opt = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut_opt );
  auto const& klut = *klut_opt;
  CHECK( klut.size() == 5 );
  CHECK( klut.num_gates() == 1 );
  CHECK( mapped_aig.num_cells() == 1 );

  kitty::dynamic_truth_table tt_xor( 2 );
  kitty::create_from_hex_string( tt_xor, "6" );
  klut.foreach_node( [&]( auto n ) {
    if ( klut.is_constant( n ) || klut.is_pi( n ) )
      return;
    CHECK( klut.node_function( n ) == tt_xor );
  } );
}

TEST_CASE( "Mapped AIG with positive output driver", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_and( a, b );
  aig.create_po( f1 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 5 );
  CHECK( klut.num_gates() == 1 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x8u );
}

TEST_CASE( "Mapped AIG with positive output driver with no function", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_and( a, b );
  aig.create_po( f1 );

  mapping_view<aig_network, false> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, false>, false>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 5 );
  CHECK( klut.num_gates() == 1 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x8u );
}

TEST_CASE( "Mapped AIG with negative output driver", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  aig.create_po( f1 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 5 );
  CHECK( klut.num_gates() == 1 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x7u );
}

TEST_CASE( "Mapped AIG with mixed output driver", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_nand( a, b );
  aig.create_po( f1 );
  aig.create_po( f2 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 6 );
  CHECK( klut.num_gates() == 2 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x8u );
  CHECK( klut.node_function( 5 )._bits[0] == 0x7u );
}

TEST_CASE( "Mapped AIG with mixed output driver with no function", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_nand( a, b );
  aig.create_po( f1 );
  aig.create_po( f2 );

  mapping_view<aig_network, false> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, false>, false>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 6 );
  CHECK( klut.num_gates() == 2 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x8u );
  CHECK( klut.node_function( 5 )._bits[0] == 0x7u );
}

TEST_CASE( "Mapped AIG with mixed output driver (opposite)", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_and( a, b );
  aig.create_po( f1 );
  aig.create_po( f2 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 6 );
  CHECK( klut.num_gates() == 2 );
  CHECK( klut.node_function( 4 )._bits[0] == 0x8u );
  CHECK( klut.node_function( 5 )._bits[0] == 0x7u );
}

TEST_CASE( "Mapped AIG with internal output", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_and( c, aig.create_and( a, b ) );
  aig.create_po( f1 );
  aig.create_po( f2 );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut.size() == 7 );
  CHECK( klut.num_gates() == 2 );
  CHECK( klut.node_function( 5 )._bits[0] == 0x7u );
  CHECK( klut.node_function( 6 )._bits[0] == 0x80u );
}

TEST_CASE( "Mapped AIG with PI outputs", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  aig.create_po( a );
  aig.create_po( !b );
  aig.create_po( c );
  aig.create_po( !c );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut );
  CHECK( klut->size() == 7 );
  CHECK( klut->num_gates() == 2 );
}

TEST_CASE( "Mapped AIG with PI outputs with no function", "[collapse_mapped]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  aig.create_po( a );
  aig.create_po( !b );
  aig.create_po( c );
  aig.create_po( !c );

  mapping_view<aig_network, false> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, false>, false>( mapped_aig );

  const auto klut = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut );
  CHECK( klut->size() == 7 );
  CHECK( klut->num_gates() == 2 );
}

TEST_CASE( "Mapped AIG with constant-0 output", "[collapse_mapped]" )
{
  aig_network aig;

  aig.create_po( aig.get_constant( false ) );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut );
  CHECK( klut->size() == 2 );
  CHECK( klut->num_gates() == 0 );
  klut->foreach_po( [&]( auto const& f ) {
    CHECK( klut->get_node( f ) == 0 );
  } );
}

TEST_CASE( "Mapped AIG with constant-0 output with no function", "[collapse_mapped]" )
{
  aig_network aig;

  aig.create_po( aig.get_constant( false ) );

  mapping_view<aig_network, false> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, false>, false>( mapped_aig );

  const auto klut = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut );
  CHECK( klut->size() == 2 );
  CHECK( klut->num_gates() == 0 );
  klut->foreach_po( [&]( auto const& f ) {
    CHECK( klut->get_node( f ) == 0 );
  } );
}

TEST_CASE( "Mapped AIG with constant-1 output", "[collapse_mapped]" )
{
  aig_network aig;

  aig.create_po( aig.get_constant( true ) );

  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig );

  const auto klut = collapse_mapped_network<klut_network>( mapped_aig );

  CHECK( klut );
  CHECK( klut->size() == 2 );
  CHECK( klut->num_gates() == 0 );
  klut->foreach_po( [&]( auto const& f ) {
    CHECK( klut->get_node( f ) == 1 );
  } );
}

TEST_CASE( "Mapped sequential k-LUT network", "[collapse_mapped]" )
{
  sequential<klut_network> klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();

  const auto f1 = klut.create_maj( a, b, c );
  const auto f2 = klut.create_ro(); // f2 <- f1
  const auto f3 = klut.create_ro(); // f3 <- f1
  const auto f4 = klut.create_xor( f2, f3 );

  klut.create_po( f4 );
  klut.create_ri( f1 ); // f2 <- f1
  klut.create_ri( f1 ); // f3 <- f1

  CHECK( klut.num_gates() == 2 );
  CHECK( klut.num_registers() == 2 );
  CHECK( klut.num_pis() == 3 );
  CHECK( klut.num_pos() == 1 );

  mapping_view<decltype(klut), true> mapped_klut{ klut };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 3;
  lut_mapping<decltype(mapped_klut), true>( mapped_klut );

  const auto _klut = collapse_mapped_network<decltype(klut)>( mapped_klut );

  CHECK( _klut );
  CHECK( _klut->num_gates() == 2 );
  CHECK( _klut->num_registers() == 2 );
  CHECK( _klut->num_pis() == 3 );
  CHECK( _klut->num_pos() == 1 );
}
