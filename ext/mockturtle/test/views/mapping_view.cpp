#include <catch.hpp>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mapping_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_mapping_view()
{
  CHECK( !has_has_mapping_v<Ntk> );
  CHECK( !has_is_cell_root_v<Ntk> );
  CHECK( !has_clear_mapping_v<Ntk> );
  CHECK( !has_num_cells_v<Ntk> );
  CHECK( !has_add_to_mapping_v<Ntk> );
  CHECK( !has_remove_from_mapping_v<Ntk> );
  CHECK( !has_cell_function_v<Ntk> );
  CHECK( !has_set_cell_function_v<Ntk> );
  CHECK( !has_foreach_cell_fanin_v<Ntk> );

  using mapped_ntk = mapping_view<Ntk>;

  CHECK( has_has_mapping_v<mapped_ntk> );
  CHECK( has_is_cell_root_v<mapped_ntk> );
  CHECK( has_clear_mapping_v<mapped_ntk> );
  CHECK( has_num_cells_v<mapped_ntk> );
  CHECK( has_add_to_mapping_v<mapped_ntk> );
  CHECK( has_remove_from_mapping_v<mapped_ntk> );
  CHECK( !has_cell_function_v<mapped_ntk> );
  CHECK( !has_set_cell_function_v<mapped_ntk> );
  CHECK( has_foreach_cell_fanin_v<mapped_ntk> );

  using mapped_ntk_f = mapping_view<Ntk, true>;

  CHECK( has_has_mapping_v<mapped_ntk_f> );
  CHECK( has_is_cell_root_v<mapped_ntk_f> );
  CHECK( has_clear_mapping_v<mapped_ntk_f> );
  CHECK( has_num_cells_v<mapped_ntk_f> );
  CHECK( has_add_to_mapping_v<mapped_ntk_f> );
  CHECK( has_remove_from_mapping_v<mapped_ntk_f> );
  CHECK( has_cell_function_v<mapped_ntk_f> );
  CHECK( has_set_cell_function_v<mapped_ntk_f> );
  CHECK( has_foreach_cell_fanin_v<mapped_ntk_f> );
};

TEST_CASE( "create different mapping views", "[mapping_view]" )
{
  test_mapping_view<aig_network>();
  test_mapping_view<mig_network>();
  test_mapping_view<klut_network>();
}

TEST_CASE( "create a mapping in an AIG", "[mapping_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  auto [sum, carry] = full_adder( aig, a, b, c );

  mapping_view mapped_aig{ aig };

  CHECK( !mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 0u );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( c ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( sum ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( carry ) ) );

  std::vector<aig_network::node> leafs{ { aig.get_node( a ), aig.get_node( b ), aig.get_node( c ) } };
  mapped_aig.add_to_mapping( aig.get_node( sum ), leafs.begin(), leafs.end() );
  mapped_aig.add_to_mapping( aig.get_node( carry ), leafs.begin(), leafs.end() );

  CHECK( mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 2u );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( c ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( sum ) ) );
  CHECK( mapped_aig.is_cell_root( aig.get_node( carry ) ) );

  mapped_aig.remove_from_mapping( aig.get_node( sum ) );
  mapped_aig.remove_from_mapping( aig.get_node( carry ) );

  CHECK( !mapped_aig.has_mapping() );
  CHECK( mapped_aig.num_cells() == 0u );

  CHECK( !mapped_aig.is_cell_root( aig.get_node( a ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( b ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( c ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( sum ) ) );
  CHECK( !mapped_aig.is_cell_root( aig.get_node( carry ) ) );
}
