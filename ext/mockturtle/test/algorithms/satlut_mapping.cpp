#include <catch.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/algorithms/satlut_mapping.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/mapping_view.hpp>

using namespace mockturtle;

TEST_CASE( "SAT-LUT mapping of AIG", "[satlut_mapping]" )
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

  satlut_mapping( mapped_aig );
}
