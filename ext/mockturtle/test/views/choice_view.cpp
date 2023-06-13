#include <catch.hpp>

#include <sstream>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/choice_view.hpp>

using namespace mockturtle;

TEST_CASE( "Create choice view", "[choice_view]" )
{
  aig_network ntk;

  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();

  auto const c0 = ntk.get_constant( false );
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_or( c, d );
  auto const t3 = ntk.create_and( t1, c );
  auto const t4 = ntk.create_and( t1, d );
  auto const f = ntk.create_and( t1, t2 );
  auto const g = ntk.create_not( a );
  auto const h = ntk.create_or( t3, t4 );
  auto const l = ntk.create_and( g, h );

  ntk.create_po( f );
  ntk.create_po( g );
  ntk.create_po( h );
  ntk.create_po( l );
  ntk.create_po( c0 );

  choice_view<aig_network> choice_ntk{ ntk };

  choice_ntk.add_choice( choice_ntk.get_node( f ), h );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( h ) ) == choice_ntk.get_node( f ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( h ) ) ) == true );

  choice_ntk.add_choice( choice_ntk.get_node( t1 ), h );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( t1 ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( f ) ) == choice_ntk.get_node( t1 ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( f ) ) ) == false );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( h ) ) == choice_ntk.get_node( t1 ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( h ) ) ) == true );

  choice_ntk.remove_choice( choice_ntk.get_node( t1 ) );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( h ) ) == choice_ntk.get_node( f ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( h ) ) ) == true );

  choice_ntk.remove_choice( choice_ntk.get_node( h ) );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == true );

  choice_ntk.add_choice( choice_ntk.get_node( f ), h );
  choice_ntk.add_choice( choice_ntk.get_node( f ), l );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == false );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( l ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( h ) ) == choice_ntk.get_node( f ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( h ) ) ) == true );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( l ) ) == choice_ntk.get_node( f ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( l ) ) ) == false );

  choice_ntk.remove_choice( choice_ntk.get_node( l ) );

  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( f ) ) == true );
  CHECK( choice_ntk.is_choice_representative( choice_ntk.get_node( h ) ) == false );
  CHECK( choice_ntk.get_choice_representative( choice_ntk.get_node( h ) ) == choice_ntk.get_node( f ) );
  CHECK( choice_ntk.is_complemented( choice_ntk.get_choice_representative_signal( choice_ntk.get_node( h ) ) ) == true );

  CHECK( choice_ntk.fanout_size( choice_ntk.get_node( h ) ) == 2 );

  choice_ntk.substitute_node( choice_ntk.get_node( h ), f );

  CHECK( choice_ntk.is_choice( choice_ntk.get_node( h ) ) == true );
  CHECK( choice_ntk.fanout_size( choice_ntk.get_node( h ) ) == 0 );
  CHECK( choice_ntk.is_choice( choice_ntk.get_node( t3 ) ) == true );
  CHECK( choice_ntk.is_choice( choice_ntk.get_node( t4 ) ) == true );
  CHECK( choice_ntk.fanout_size( choice_ntk.get_node( f ) ) == 3 );
}
