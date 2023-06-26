#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mffc_view.hpp>

using namespace mockturtle;

template<typename Ntk>
void initialize_refs( Ntk& ntk )
{
  ntk.clear_values();
  ntk.foreach_node( [&]( auto const& n ) {
    ntk.set_value( n, ntk.fanout_size( n ) );
  } );
}

TEST_CASE( "create a MFFC view", "[mffc_view]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();
  const auto e = aig.create_pi();

  const auto f1 = aig.create_and( a, b );
  const auto f2 = aig.create_and( c, d );
  const auto f3 = aig.create_and( f1, f2 );
  const auto f4 = aig.create_and( e, f2 );
  const auto f5 = aig.create_and( f1, f3 );
  const auto f6 = aig.create_and( f2, f3 );
  const auto f7 = aig.create_and( f5, f6 );
  const auto f8 = aig.create_and( f4, f7 );

  aig.create_po( f8 );
  initialize_refs( aig );

  CHECK( aig.size() == 14 );
  CHECK( aig.num_pis() == 5 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 8 );

  CHECK( aig.get_node( f1 ) == 6 );
  CHECK( aig.get_node( f2 ) == 7 );
  CHECK( aig.get_node( f3 ) == 8 );
  CHECK( aig.get_node( f4 ) == 9 );
  CHECK( aig.get_node( f5 ) == 10 );
  CHECK( aig.get_node( f6 ) == 11 );
  CHECK( aig.get_node( f7 ) == 12 );
  CHECK( aig.get_node( f8 ) == 13 );

  mffc_view mffc1{ aig, aig.get_node( f1 ) };

  CHECK( mffc1.size() == 4 );
  CHECK( mffc1.num_pis() == 2 );
  CHECK( mffc1.num_pos() == 1 );
  CHECK( mffc1.num_gates() == 1 );
  mffc1.foreach_pi( [&]( auto const& n, auto i ) {
    CHECK( mffc1.node_to_index( n ) == i + 1 );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( a ) );
      break;
    case 1:
      CHECK( n == aig.get_node( b ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc1.foreach_gate( [&]( auto const& n, auto i ) {
    CHECK( mffc1.node_to_index( n ) == i + 1 + mffc1.num_pis() );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f1 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc1.foreach_node( [&]( auto const& n, auto i ) {
    CHECK( mffc1.node_to_index( n ) == i );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( aig.get_constant( false ) ) );
      break;
    case 1:
      CHECK( n == aig.get_node( a ) );
      break;
    case 2:
      CHECK( n == aig.get_node( b ) );
      break;
    case 3:
      CHECK( n == aig.get_node( f1 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc1.foreach_po( [&]( auto const& f ) { CHECK( mffc1.get_node( f ) == aig.get_node( f1 ) ); } );

  mffc_view mffc2{ aig, aig.get_node( f3 ) };

  CHECK( mffc2.size() == 4 );
  CHECK( mffc2.num_pis() == 2 );
  CHECK( mffc2.num_pos() == 1 );
  CHECK( mffc2.num_gates() == 1 );
  mffc2.foreach_pi( [&]( auto const& n, auto i ) {
    CHECK( mffc2.node_to_index( n ) == i + 1 );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 1:
      CHECK( n == aig.get_node( f2 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc2.foreach_gate( [&]( auto const& n, auto i ) {
    CHECK( mffc2.node_to_index( n ) == i + 1 + mffc2.num_pis() );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f3 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc2.foreach_node( [&]( auto const& n, auto i ) {
    CHECK( mffc2.node_to_index( n ) == i );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( aig.get_constant( false ) ) );
      break;
    case 1:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 2:
      CHECK( n == aig.get_node( f2 ) );
      break;
    case 3:
      CHECK( n == aig.get_node( f3 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc2.foreach_po( [&]( auto const& f ) { CHECK( mffc2.get_node( f ) == aig.get_node( f3 ) ); } );

  mffc_view mffc3{ aig, aig.get_node( f5 ) };

  CHECK( mffc3.size() == 4 );
  CHECK( mffc3.num_pis() == 2 );
  CHECK( mffc3.num_pos() == 1 );
  CHECK( mffc3.num_gates() == 1 );
  mffc3.foreach_pi( [&]( auto const& n, auto i ) {
    CHECK( mffc3.node_to_index( n ) == i + 1 );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 1:
      CHECK( n == aig.get_node( f3 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc3.foreach_gate( [&]( auto const& n, auto i ) {
    CHECK( mffc3.node_to_index( n ) == i + 1 + mffc3.num_pis() );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f5 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc3.foreach_node( [&]( auto const& n, auto i ) {
    CHECK( mffc3.node_to_index( n ) == i );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( aig.get_constant( false ) ) );
      break;
    case 1:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 2:
      CHECK( n == aig.get_node( f3 ) );
      break;
    case 3:
      CHECK( n == aig.get_node( f5 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc3.foreach_po( [&]( auto const& f ) { CHECK( mffc3.get_node( f ) == aig.get_node( f5 ) ); } );

  mffc_view mffc4{ aig, aig.get_node( f7 ) };

  CHECK( mffc4.size() == 9 );
  CHECK( mffc4.num_pis() == 3 );
  CHECK( mffc4.num_pos() == 1 );
  CHECK( mffc4.num_gates() == 5 );
  mffc4.foreach_pi( [&]( auto const& n, auto i ) {
    CHECK( mffc4.node_to_index( n ) == i + 1 );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( a ) );
      break;
    case 1:
      CHECK( n == aig.get_node( b ) );
      break;
    case 2:
      CHECK( n == aig.get_node( f2 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc4.foreach_gate( [&]( auto const& n, auto i ) {
    CHECK( mffc4.node_to_index( n ) == i + 1 + mffc4.num_pis() );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 1:
      CHECK( n == aig.get_node( f3 ) );
      break;
    case 2:
      CHECK( n == aig.get_node( f5 ) );
      break;
    case 3:
      CHECK( n == aig.get_node( f6 ) );
      break;
    case 4:
      CHECK( n == aig.get_node( f7 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc4.foreach_node( [&]( auto const& n, auto i ) {
    CHECK( mffc4.node_to_index( n ) == i );
    switch ( i )
    {
    case 0:
      CHECK( n == aig.get_node( aig.get_constant( false ) ) );
      break;
    case 1:
      CHECK( n == aig.get_node( a ) );
      break;
    case 2:
      CHECK( n == aig.get_node( b ) );
      break;
    case 3:
      CHECK( n == aig.get_node( f2 ) );
      break;
    case 4:
      CHECK( n == aig.get_node( f1 ) );
      break;
    case 5:
      CHECK( n == aig.get_node( f3 ) );
      break;
    case 6:
      CHECK( n == aig.get_node( f5 ) );
      break;
    case 7:
      CHECK( n == aig.get_node( f6 ) );
      break;
    case 8:
      CHECK( n == aig.get_node( f7 ) );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
  mffc4.foreach_po( [&]( auto const& f ) { CHECK( mffc4.get_node( f ) == aig.get_node( f7 ) ); } );

  mffc_view mffc5{ aig, aig.get_node( f8 ) };

  CHECK( mffc5.size() == 14 );
  CHECK( mffc5.num_pis() == 5 );
  CHECK( mffc5.num_pos() == 1 );
  CHECK( mffc5.num_gates() == 8 );
  mffc5.foreach_pi( [&]( auto const& n, auto i ) {
    CHECK( mffc5.node_to_index( n ) == i + 1 );
    CHECK( n == i + 1 );
  } );
  mffc5.foreach_po( [&]( auto const& f ) { CHECK( mffc5.get_node( f ) == aig.get_node( f8 ) ); } );
}
