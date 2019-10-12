#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/window_view.hpp>
#include <mockturtle/algorithms/reconv_cut.hpp>

using namespace mockturtle;

TEST_CASE( "create window view on AIG", "[window_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.size() == 7 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 4 );

  fanout_view<aig_network> fanout_ntk( aig );
  fanout_ntk.clear_visited();

  const auto pivot1 = aig.get_node( f3 );
  auto const& leaves1 = reconv_cut( reconv_cut_params{4} )( aig, { pivot1 } );
  window_view<fanout_view<aig_network>> win1( fanout_ntk, leaves1, { pivot1 }, false );

  CHECK( is_network_type_v<decltype( win1 )> );
  CHECK( win1.size() == 5 );
  CHECK( win1.num_pis() == 2 );
  CHECK( win1.num_pos() == 3 ); // b, f1, f2

  const auto pivot2 = aig.get_node( f2 );
  auto const& leaves2 = reconv_cut( reconv_cut_params{4} )( aig, { pivot2 } );
  window_view<fanout_view<aig_network>> win2( fanout_ntk, leaves2, { pivot2 }, false );
  CHECK( win2.size() == 5 );
  CHECK( win2.num_pis() == 2 );
  CHECK( win2.num_pos() == 3 ); // a, f1, f3

  const auto pivot3 = aig.get_node( f1 );
  auto const& leaves3 = reconv_cut( reconv_cut_params{4} )( aig, { pivot3 } );
  window_view<fanout_view<aig_network>> win3( fanout_ntk, leaves3, { pivot3 }, true );
  CHECK( win3.size() == 7 );
  CHECK( win3.num_pis() == 2 );
  CHECK( win3.num_pos() == 1 ); // f4
}
