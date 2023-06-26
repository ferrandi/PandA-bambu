#include <catch.hpp>

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/resyn_engines/xag_resyn.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/utils/debugging_utils.hpp>
#include <mockturtle/utils/index_list.hpp>
#include <mockturtle/utils/network_utils.hpp>

using namespace mockturtle;

template<class TT>
struct aig_resyn_sparams_node_map : public xag_resyn_static_params
{
  using truth_table_storage_type = node_map<TT, aig_network>;
  using node_type = typename aig_network::node;
  static constexpr bool use_xor = false;
};

TEST_CASE( "clone a window, optimize it, and insert it back", "[network_utils]" )
{
  using node = aig_network::node;
  using signal = aig_network::signal;

  /* original network */
  aig_network aig;
  auto const x0 = aig.create_pi();
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();
  auto const a = aig.create_and( x0, x1 );
  auto const b = aig.create_and( x2, x3 );
  auto const t0 = aig.create_and( !a, b );
  auto const t1 = aig.create_and( a, !b );
  auto const t2 = aig.create_or( t0, t1 ); // a XOR b
  auto const t3 = aig.create_and( a, b );
  aig.create_po( t2 );
  aig.create_po( t3 );

  /* create a window*/
  std::vector<node> inputs{ aig.get_node( a ), aig.get_node( b ) };
  std::vector<signal> outputs{ t2, t3 };
  std::vector<node> gates{ aig.get_node( t0 ), aig.get_node( t1 ), aig.get_node( t2 ), aig.get_node( t3 ) };

  aig_network win;
  clone_subnetwork( aig, inputs, outputs, gates, win );

  CHECK( win.num_pis() == 2u );
  CHECK( win.num_pos() == 2u );
  CHECK( win.num_gates() == 4u );

  /* optimize the window */
  using TT = kitty::static_truth_table<2>;
  using ResynEngine = xag_resyn_decompose<TT, aig_resyn_sparams_node_map<TT>>;
  typename ResynEngine::stats engine_st;

  default_simulator<TT> sim;
  auto tts = simulate_nodes<TT, aig_network>( win, sim );
  win.foreach_po( [&]( auto const& f, auto const i ) {
    if ( i == 0 ) // try only the first PO
    {
      auto const root = win.get_node( f );
      std::vector<node> divs;
      std::vector<signal> div_signals;

      win.incr_trav_id();
      win.set_visited( root, win.trav_id() ); // exclude root
      win.foreach_fanin( root, [&]( auto const& fi ) {
        win.set_visited( win.get_node( fi ), win.trav_id() ); // exclude MFFC
      } );
      win.foreach_node( [&]( auto const& n ) {
        if ( win.visited( n ) != win.trav_id() )
        {
          divs.emplace_back( n );
          div_signals.emplace_back( win.make_signal( n ) );
        }
      } );

      ResynEngine engine( engine_st );
      auto const il = engine( tts[root], ~tts[win.get_constant( false )], divs.begin(), divs.end(), tts, 2 );
      if ( il )
      {
        insert( win, div_signals.begin(), div_signals.end(), *il, [&]( auto const& s ) {
          win.substitute_node( root, s );
        } );
      }
    }
  } );

  CHECK( win.num_gates() == 3u );
  CHECK( check_window_equivalence( aig, inputs, outputs, gates, win ) );

  /* insert the window back */
  std::vector<signal> input_signals{ a, b };
  uint32_t counter = 0u;
  insert_ntk( aig, input_signals.begin(), input_signals.end(), win, [&]( signal const& _new ) {
    auto const _old = outputs.at( counter++ );
    if ( _old != _new )
    {
      aig.substitute_node( aig.get_node( _old ), aig.is_complemented( _old ) ? !_new : _new );
    }
  } );

  CHECK( aig.num_gates() == 5u );
}
