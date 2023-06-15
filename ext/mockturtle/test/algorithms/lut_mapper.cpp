#include <catch.hpp>

#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/lut_mapper.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mapping_view.hpp>

using namespace mockturtle;

struct lut_custom_cost
{
  std::pair<uint32_t, uint32_t> operator()( uint32_t num_leaves ) const
  {
    if ( num_leaves < 2u )
      return { 0u, 0u };
    return { num_leaves, 1u }; /* area, delay */
  }

  std::pair<uint32_t, uint32_t> operator()( kitty::dynamic_truth_table const& tt ) const
  {
    if ( tt.num_vars() < 2u )
      return { 0u, 0u };
    return { tt.num_vars(), 1u }; /* area, delay */
  }
};

TEST_CASE( "LUT map of AIG", "[lut_mapper]" )
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

  lut_map( mapped_aig );

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

TEST_CASE( "LUT map of a sequential AIG", "[lut_mapper]" )
{
  sequential<aig_network> aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_ro(); // f2 <- f1
  const auto f3 = aig.create_xor( f2, c );

  aig.create_po( f3 );
  aig.create_ri( f1 ); // f3 <- f1

  mapping_view mapped_aig{ aig };

  CHECK( has_has_mapping_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_is_cell_root_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_clear_mapping_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_num_cells_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_add_to_mapping_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_remove_from_mapping_v<mapping_view<sequential<aig_network>>> );
  CHECK( has_foreach_cell_fanin_v<mapping_view<sequential<aig_network>>> );

  CHECK( !mapped_aig.has_mapping() );

  lut_map( mapped_aig );

  CHECK( mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 2 );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( c ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( f1 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f2 ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( f3 ) ) );

  mapped_aig.clear_mapping();

  CHECK( !mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 0 );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f1 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f2 ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( f3 ) ) );
}

TEST_CASE( "LUT map of 2-LUT network", "[lut_mapper]" )
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
  lut_map( mapped_aig );

  CHECK( mapped_aig.num_cells() == 3 );
}

TEST_CASE( "LUT map of 2-LUT network area", "[lut_mapper]" )
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

  lut_map_params ps;
  ps.area_oriented_mapping = true;
  lut_map( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 3 );
}

TEST_CASE( "LUT map of 8-LUT network", "[lut_mapper]" )
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
  lut_map_params ps;
  ps.area_oriented_mapping = true;
  lut_map( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 12 );
}

TEST_CASE( "LUT map of 64-LUT network", "[lut_mapper]" )
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
  lut_map( mapped_aig );

  CHECK( mapped_aig.num_cells() == 114 );
}

TEST_CASE( "LUT map of 64-LUT network delay relaxed", "[lut_mapper]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 64 ), b( 64 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.get_constant( false );

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view<aig_network, false> mapped_aig{ aig };

  lut_map_params ps;
  ps.area_oriented_mapping = false;
  ps.relax_required = 1000;
  ps.recompute_cuts = true;
  ps.remove_dominated_cuts = false;
  ps.edge_optimization = false;
  lut_map<decltype( mapped_aig ), false>( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 114 );
}

TEST_CASE( "LUT map of 64-LUT network area", "[lut_mapper]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 64 ), b( 64 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.get_constant( false );

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view<aig_network, true> mapped_aig{ aig };

  lut_map_params ps;
  ps.area_oriented_mapping = true;
  ps.recompute_cuts = false;
  ps.remove_dominated_cuts = false;
  ps.edge_optimization = false;
  lut_map<decltype( mapped_aig ), true>( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 116 );
}

TEST_CASE( "LUT map with functions of full adder", "[lut_mapper]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();

  const auto [sum, carry] = full_adder( aig, a, b, c );
  aig.create_po( sum );
  aig.create_po( carry );

  mapping_view<aig_network, true> mapped_aig{ aig };

  lut_map_params ps;
  ps.recompute_cuts = false;
  ps.edge_optimization = false;
  ps.remove_dominated_cuts = false;
  lut_map<mapping_view<aig_network, true>, true>( mapped_aig );

  CHECK( has_cell_function_v<mapping_view<aig_network, true>> );
  CHECK( has_set_cell_function_v<mapping_view<aig_network, true>> );

  CHECK( mapped_aig.num_cells() == 2 );
  CHECK( mapped_aig.is_cell_root( aig.get_node( sum ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( carry ) ) );
  CHECK( mapped_aig.cell_function( aig.get_node( sum ) )._bits[0] == 0x96 );
  CHECK( mapped_aig.cell_function( aig.get_node( carry ) )._bits[0] == 0x17 );
}

TEST_CASE( "Collapse MFFC of 64-LUT network", "[lut_mapper]" )
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

  lut_map_params ps;
  ps.collapse_mffcs = true;
  lut_map( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 317 );
}

TEST_CASE( "LUT map of 64-LUT network with cost function", "[lut_mapper]" )
{
  aig_network aig;

  std::vector<aig_network::signal> a( 64 ), b( 64 );
  std::generate( a.begin(), a.end(), [&aig]() { return aig.create_pi(); } );
  std::generate( b.begin(), b.end(), [&aig]() { return aig.create_pi(); } );
  auto carry = aig.get_constant( false );

  carry_ripple_adder_inplace( aig, a, b, carry );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { aig.create_po( f ); } );
  aig.create_po( carry );

  mapping_view<aig_network, true> mapped_aig{ aig };

  lut_map_params ps;
  ps.recompute_cuts = false;
  ps.area_oriented_mapping = true;
  ps.remove_dominated_cuts = false;
  ps.cut_enumeration_ps.cut_size = 5;
  ps.cut_enumeration_ps.cut_limit = 8;
  lut_map<decltype( mapped_aig ), true, lut_custom_cost>( mapped_aig, ps );

  CHECK( mapped_aig.num_cells() == 128 );
}

TEST_CASE( "LUT map remapping LUT network", "[lut_mapper]" )
{
  /* Issue #592 */
  klut_network ntk;

  auto const n2 = ntk.create_pi();
  auto const n3 = ntk.create_pi();
  auto const n4 = ntk.create_maj( n2, ntk.get_constant( false ), ntk.get_constant( true ) );
  auto const n5 = ntk.create_xor( n3, n4 );
  ntk.create_po( n5 );

  mapping_view<klut_network, true> mapped_ntk{ ntk };

  lut_map_params ps;
  ps.cut_enumeration_ps.cut_size = 8;
  lut_map<decltype( mapped_ntk ), true>( mapped_ntk, ps );

  auto const res = *collapse_mapped_network<klut_network>( mapped_ntk );
  auto const miter_ntk = *miter<klut_network>( ntk, res );

  CHECK( mapped_ntk.num_cells() == 1 );
  CHECK( *equivalence_checking( miter_ntk ) == true );
}
