#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/window_utils.hpp>
#include <mockturtle/views/color_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/window_view.hpp>

using namespace mockturtle;

TEST_CASE( "expand node set towards TFI without cut-size", "[window_utils]" )
{
  using node = typename aig_network::node;

  aig_network _aig;
  auto const a = _aig.create_pi();
  auto const b = _aig.create_pi();
  auto const c = _aig.create_pi();
  auto const d = _aig.create_pi();
  auto const f1 = _aig.create_and( b, c );
  auto const f2 = _aig.create_and( b, f1 );
  auto const f3 = _aig.create_and( a, f2 );
  auto const f4 = _aig.create_and( d, f2 );
  auto const f5 = _aig.create_and( f3, f4 );
  _aig.create_po( f5 );

  color_view aig{ _aig };
  {
    aig.new_color();

    /* a cut that can be expanded without increasing cut-size */
    std::vector<node> inputs{ aig.get_node( a ), aig.get_node( b ), aig.get_node( f1 ), aig.get_node( d ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    bool const trivial_cut = expand0_towards_tfi( aig, inputs );
    CHECK( trivial_cut );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( a ), aig.get_node( b ), aig.get_node( c ), aig.get_node( d ) } );
  }

  {
    aig.new_color();

    /* a cut that cannot be expanded without increasing cut-size */
    std::vector<node> inputs{ aig.get_node( f3 ), aig.get_node( f4 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    bool const trivial_cut = expand0_towards_tfi( aig, inputs );
    CHECK( !trivial_cut );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( f3 ), aig.get_node( f4 ) } );
  }

  {
    aig.new_color();

    /* a cut that can be moved towards the PIs */
    std::vector<node> inputs{ aig.get_node( f2 ), aig.get_node( f3 ), aig.get_node( f4 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    bool const trivial_cut = expand0_towards_tfi( aig, inputs );
    CHECK( !trivial_cut );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( a ), aig.get_node( d ), aig.get_node( f2 ) } );
  }

  {
    aig.new_color();

    /* the cut { f3, f5 } can be simplified to { f3, f4 } */
    std::vector<node> inputs{ aig.get_node( f3 ), aig.get_node( f5 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    bool const trivial_cut = expand0_towards_tfi( aig, inputs );
    CHECK( !trivial_cut );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( f3 ), aig.get_node( f4 ) } );
  }

  {
    aig.new_color();

    /* the cut { f4, f5 } also can be simplified to { f3, f4 } */
    std::vector<node> inputs{ aig.get_node( f4 ), aig.get_node( f5 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    bool const trivial_cut = expand0_towards_tfi( aig, inputs );
    CHECK( !trivial_cut );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( f3 ), aig.get_node( f4 ) } );
  }
}

TEST_CASE( "expand node set towards TFI", "[window_utils]" )
{
  using node = typename aig_network::node;

  aig_network _aig;
  auto const a = _aig.create_pi();
  auto const b = _aig.create_pi();
  auto const c = _aig.create_pi();
  auto const d = _aig.create_pi();
  auto const f1 = _aig.create_and( b, c );
  auto const f2 = _aig.create_and( b, f1 );
  auto const f3 = _aig.create_and( a, f2 );
  auto const f4 = _aig.create_and( d, f2 );
  auto const f5 = _aig.create_and( f3, f4 );
  _aig.create_po( f5 );

  color_view aig{ _aig };

  {
    aig.new_color();

    /* expand from { f5 } to 4-cut { a, b, c, d } */
    std::vector<node> inputs{ aig.get_node( f5 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    expand_towards_tfi( aig, inputs, 4u );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( a ), aig.get_node( b ), aig.get_node( c ), aig.get_node( d ) } );
  }

  {
    aig.new_color();

    /* expand from { f3, f5 } to 3-cut { a, d, f2 } */
    std::vector<node> inputs{ aig.get_node( f3 ), aig.get_node( f5 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    expand_towards_tfi( aig, inputs, 3u );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( a ), aig.get_node( d ), aig.get_node( f2 ) } );
  }

  {
    aig.new_color();

    /* expand from { f4, f5 } to 3-cut { a, d, f2 } */
    std::vector<node> inputs{ aig.get_node( f4 ), aig.get_node( f5 ) };
    for ( const auto& i : inputs )
    {
      aig.paint( i );
    }

    expand_towards_tfi( aig, inputs, 3u );

    std::sort( std::begin( inputs ), std::end( inputs ) );
    CHECK( inputs == std::vector<node>{ aig.get_node( a ), aig.get_node( d ), aig.get_node( f2 ) } );
  }
}

TEST_CASE( "expand node set towards TFO", "[window_utils]" )
{
  using node = typename aig_network::node;

  aig_network _aig;
  auto const a = _aig.create_pi();
  auto const b = _aig.create_pi();
  auto const c = _aig.create_pi();
  auto const d = _aig.create_pi();
  auto const f1 = _aig.create_and( b, c );
  auto const f2 = _aig.create_and( b, f1 );
  auto const f3 = _aig.create_and( a, f2 );
  auto const f4 = _aig.create_and( d, f2 );
  auto const f5 = _aig.create_and( f3, f4 );
  _aig.create_po( f5 );

  std::vector<node> inputs{ _aig.get_node( a ), _aig.get_node( b ), _aig.get_node( c ), _aig.get_node( d ) };

  fanout_view fanout_aig{ _aig };
  depth_view depth_aig{ fanout_aig };
  color_view aig{ depth_aig };

  {
    std::vector<node> nodes;
    expand_towards_tfo( aig, inputs, nodes );

    std::sort( std::begin( nodes ), std::end( nodes ) );
    CHECK( nodes == std::vector<node>{ aig.get_node( f1 ), aig.get_node( f2 ), aig.get_node( f3 ),
                                       aig.get_node( f4 ), aig.get_node( f5 ) } );
  }

  {
    std::vector<node> nodes;
    levelized_expand_towards_tfo( aig, inputs, nodes );

    std::sort( std::begin( nodes ), std::end( nodes ) );
    CHECK( nodes == std::vector<node>{ aig.get_node( f1 ), aig.get_node( f2 ), aig.get_node( f3 ),
                                       aig.get_node( f4 ), aig.get_node( f5 ) } );
  }
}

TEST_CASE( "create window for pivot", "[window_utils]" )
{
  aig_network _aig;
  auto const a = _aig.create_pi();
  auto const b = _aig.create_pi();
  auto const c = _aig.create_pi();
  auto const d = _aig.create_pi();
  auto const f1 = _aig.create_and( b, c );
  auto const f2 = _aig.create_and( b, f1 );
  auto const f3 = _aig.create_and( a, f2 );
  auto const f4 = _aig.create_and( d, f2 );
  auto const f5 = _aig.create_and( f3, f4 );
  _aig.create_po( f5 );

  fanout_view fanout_aig{ _aig };
  depth_view depth_aig{ fanout_aig };
  color_view aig{ depth_aig };

  create_window_impl windowing( aig );
  if ( auto w = windowing.run( aig.get_node( f5 ), 6u, 5u ) )
  {
    window_view win( aig, w->inputs, w->outputs, w->nodes );
    CHECK( win.num_cis() == 4u );
    CHECK( win.num_cos() == 1u );
    CHECK( win.num_gates() == 5u );
  }
}
