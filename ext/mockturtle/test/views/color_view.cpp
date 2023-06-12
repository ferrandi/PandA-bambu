#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/color_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void test_color_view()
{
  CHECK( is_network_type_v<Ntk> );
  CHECK( !has_new_color_v<Ntk> );
  CHECK( !has_current_color_v<Ntk> );
  CHECK( !has_clear_colors_v<Ntk> );
  CHECK( !has_color_v<Ntk> );
  CHECK( !has_paint_v<Ntk> );
  CHECK( !has_eval_color_v<Ntk> );
  CHECK( !has_eval_fanins_color_v<Ntk> );

  using color_ntk = color_view<Ntk>;
  CHECK( is_network_type_v<color_ntk> );
  CHECK( has_new_color_v<color_ntk> );
  CHECK( has_current_color_v<color_ntk> );
  CHECK( has_clear_colors_v<color_ntk> );
  CHECK( has_color_v<color_ntk> );
  CHECK( has_paint_v<color_ntk> );
  CHECK( has_eval_color_v<color_ntk> );
  CHECK( has_eval_fanins_color_v<color_ntk> );

  using color_color_ntk = color_view<color_ntk>;
  CHECK( is_network_type_v<color_color_ntk> );
  CHECK( has_new_color_v<color_color_ntk> );
  CHECK( has_current_color_v<color_color_ntk> );
  CHECK( has_clear_colors_v<color_color_ntk> );
  CHECK( has_color_v<color_color_ntk> );
  CHECK( has_paint_v<color_color_ntk> );
  CHECK( has_eval_color_v<color_color_ntk> );
  CHECK( has_eval_fanins_color_v<color_color_ntk> );

  using out_of_place_color_ntk = out_of_place_color_view<Ntk>;
  CHECK( is_network_type_v<out_of_place_color_ntk> );
  CHECK( has_new_color_v<out_of_place_color_ntk> );
  CHECK( has_current_color_v<out_of_place_color_ntk> );
  CHECK( has_clear_colors_v<out_of_place_color_ntk> );
  CHECK( has_color_v<out_of_place_color_ntk> );
  CHECK( has_paint_v<out_of_place_color_ntk> );
  CHECK( has_eval_color_v<out_of_place_color_ntk> );
  CHECK( has_eval_fanins_color_v<out_of_place_color_ntk> );
};

TEST_CASE( "create different color views", "[color_view]" )
{
  test_color_view<aig_network>();
  test_color_view<mig_network>();
  test_color_view<xag_network>();
  test_color_view<xmg_network>();
  test_color_view<klut_network>();
}

TEST_CASE( "in-place color view", "[color_view]" )
{
  aig_network _aig;
  const auto a = _aig.create_pi();
  const auto b = _aig.create_pi();
  const auto c = _aig.create_pi();
  const auto d = _aig.create_pi();
  const auto e = _aig.create_pi();

  const auto f1 = _aig.create_and( a, b );
  const auto f2 = _aig.create_and( c, d );
  const auto f3 = _aig.create_and( f1, f2 );
  const auto f4 = _aig.create_and( e, f2 );
  const auto f5 = _aig.create_and( f1, f3 );
  const auto f6 = _aig.create_and( f2, f3 );
  const auto f7 = _aig.create_and( f5, f6 );
  const auto f8 = _aig.create_and( f4, f7 );
  _aig.create_po( f8 );

  color_view aig{ _aig };

  auto const white = aig.new_color();
  auto const yellow = aig.new_color();
  CHECK( yellow > white );
  auto const red = aig.new_color();
  CHECK( red > white );

  /* assign some colors: f5 is white, f1 is yellow, and f3 is in the color of f1 */
  aig.paint( aig.get_node( f5 ), white );
  aig.paint( aig.get_node( f1 ), yellow );
  aig.paint( aig.get_node( f3 ), aig.get_node( f1 ) );

  /* f1 and f3 have the same color */
  CHECK( aig.eval_color( aig.get_node( f1 ), aig.get_node( f3 ),
                         [&]( auto c0, auto c1 ) { return c0 == c1; } ) );

  /* f1 and f5 have different colors */
  CHECK( aig.eval_color( aig.get_node( f1 ), aig.get_node( f5 ),
                         [&]( auto c0, auto c1 ) { return c0 != c1; } ) );

  /* f5 is at least white */
  CHECK( aig.eval_color( aig.get_node( f5 ),
                         [&]( auto color ) { return color >= white; } ) );

  /* f5 is not yellow */
  CHECK( aig.eval_color( aig.get_node( f5 ),
                         [&]( auto color ) { return color != yellow; } ) );

  /* the fanin's of f5 are at least white */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color >= white; } ) );

  /* the fanins of f5 are yellow */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color == yellow; } ) );

  /* at least one fanin of f5 is not red */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color != red; } ) );

  /* colors are stored in the visited flags */
  CHECK( aig.visited( aig.get_node( f5 ) ) == white );
  CHECK( aig.visited( aig.get_node( f1 ) ) == yellow );
  CHECK( aig.visited( aig.get_node( f3 ) ) == yellow );
}

TEST_CASE( "out-of-place color view", "[color_view]" )
{
  aig_network _aig;
  const auto a = _aig.create_pi();
  const auto b = _aig.create_pi();
  const auto c = _aig.create_pi();
  const auto d = _aig.create_pi();
  const auto e = _aig.create_pi();

  const auto f1 = _aig.create_and( a, b );
  const auto f2 = _aig.create_and( c, d );
  const auto f3 = _aig.create_and( f1, f2 );
  const auto f4 = _aig.create_and( e, f2 );
  const auto f5 = _aig.create_and( f1, f3 );
  const auto f6 = _aig.create_and( f2, f3 );
  const auto f7 = _aig.create_and( f5, f6 );
  const auto f8 = _aig.create_and( f4, f7 );
  _aig.create_po( f8 );

  out_of_place_color_view aig{ _aig };

  auto const white = aig.new_color();
  auto const yellow = aig.new_color();
  CHECK( yellow > white );
  auto const red = aig.new_color();
  CHECK( red > white );

  /* assign some colors: f5 is white, f1 is yellow, and f3 is in the color of f1 */
  aig.paint( aig.get_node( f5 ), white );
  aig.paint( aig.get_node( f1 ), yellow );
  aig.paint( aig.get_node( f3 ), aig.get_node( f1 ) );

  /* f1 and f3 have the same color */
  CHECK( aig.eval_color( aig.get_node( f1 ), aig.get_node( f3 ),
                         [&]( auto c0, auto c1 ) { return c0 == c1; } ) );

  /* f1 and f5 have different colors */
  CHECK( aig.eval_color( aig.get_node( f1 ), aig.get_node( f5 ),
                         [&]( auto c0, auto c1 ) { return c0 != c1; } ) );

  /* f5 is at least white */
  CHECK( aig.eval_color( aig.get_node( f5 ),
                         [&]( auto color ) { return color >= white; } ) );

  /* f5 is not yellow */
  CHECK( aig.eval_color( aig.get_node( f5 ),
                         [&]( auto color ) { return color != yellow; } ) );

  /* the fanin's of f5 are at least white */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color >= white; } ) );

  /* the fanins of f5 are yellow */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color == yellow; } ) );

  /* at least one fanin of f5 is not red */
  CHECK( aig.eval_fanins_color( aig.get_node( f5 ),
                                [&]( auto color ) { return color != red; } ) );

  /* visited flags have not been affected by assigning colors */
  CHECK( aig.visited( aig.get_node( f5 ) ) == 0u );
  CHECK( aig.visited( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.visited( aig.get_node( f3 ) ) == 0u );
}
