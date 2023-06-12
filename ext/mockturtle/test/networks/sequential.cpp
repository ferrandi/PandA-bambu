#include <catch.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/aqfp.hpp>
#include <mockturtle/networks/cover.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

TEST_CASE( "create and use register in an AIG", "[aig]" )
{
  sequential<aig_network> aig;

  CHECK( has_foreach_po_v<sequential<aig_network>> );
  CHECK( has_create_po_v<sequential<aig_network>> );
  CHECK( has_create_pi_v<sequential<aig_network>> );
  CHECK( has_create_ro_v<sequential<aig_network>> );
  CHECK( has_create_ri_v<sequential<aig_network>> );
  CHECK( has_create_and_v<sequential<aig_network>> );

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();

  CHECK( aig.size() == 4 );
  CHECK( aig.num_registers() == 0 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 0 );

  const auto f1 = aig.create_and( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( !f1 );

  const auto f2 = aig.create_and( f1, x3 );
  aig.create_ri( f2 );

  const auto ro = aig.create_ro();
  aig.create_po( ro );

  CHECK( aig.num_pos() == 3 );
  CHECK( aig.num_registers() == 1 );

  aig.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == f1 );
      break;
    case 1:
      CHECK( s == !f1 );
      break;
    case 2:
      // Check if the output (connected to the register) data is the same as the node data being registered.
      CHECK( f2.data == aig.po_at( i ).data );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
}

TEST_CASE( "create and use register in an xag network", "[xag]" )
{
  sequential<xag_network> xag;

  CHECK( has_foreach_po_v<sequential<xag_network>> );
  CHECK( has_create_po_v<sequential<xag_network>> );
  CHECK( has_create_pi_v<sequential<xag_network>> );
  CHECK( has_create_ro_v<sequential<xag_network>> );
  CHECK( has_create_ri_v<sequential<xag_network>> );
  CHECK( has_create_maj_v<sequential<xag_network>> );

  const auto c0 = xag.get_constant( false );
  const auto x1 = xag.create_pi();
  const auto x2 = xag.create_pi();
  const auto x3 = xag.create_pi();
  const auto x4 = xag.create_pi();

  CHECK( xag.size() == 5 );
  CHECK( xag.num_registers() == 0 );
  CHECK( xag.num_cis() == 4 );
  CHECK( xag.num_cos() == 0 );

  const auto f1 = xag.create_xor3( x1, x2, x3 );
  xag.create_po( f1 );

  CHECK( xag.num_pos() == 1 );

  const auto s1 = xag.create_ro(); // ntk. input
  xag.create_po( s1 );             // po

  const auto f2 = xag.create_xor3( f1, x4, c0 );
  xag.create_ri( f2 ); // ntk. output

  CHECK( xag.num_registers() == 1 );
  CHECK( xag.num_cis() == 4 + 1 );
  CHECK( xag.num_cos() == 2 + 1 );

  xag.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( xag.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xag.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xag.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xag.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xag.make_signal( node ) == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  xag.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( xag.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xag.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xag.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xag.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xag.make_signal( node ) == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( xag.make_signal( node ) == s1 ); /* first state-bit */
      CHECK( xag.is_ci( node ) );
      CHECK( !xag.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  xag.foreach_po( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    default:
      CHECK( false );
    }
  } );

  xag.foreach_co( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    case 2:
      CHECK( node == f2 ); /* first next-state bit */
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "create and use register in an MIG", "[mig]" )
{
  sequential<mig_network> mig;

  CHECK( has_foreach_po_v<sequential<mig_network>> );
  CHECK( has_create_po_v<sequential<mig_network>> );
  CHECK( has_create_pi_v<sequential<mig_network>> );
  CHECK( has_create_ro_v<sequential<mig_network>> );
  CHECK( has_create_ri_v<sequential<mig_network>> );
  CHECK( has_create_maj_v<sequential<mig_network>> );

  const auto c0 = mig.get_constant( false );
  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();
  const auto x4 = mig.create_pi();

  CHECK( mig.size() == 5 );
  CHECK( mig.num_registers() == 0 );
  CHECK( mig.num_pis() == 4 );
  CHECK( mig.num_pos() == 0 );
  CHECK( mig.is_combinational() );

  const auto f1 = mig.create_maj( x1, x2, x3 );
  mig.create_po( f1 );
  mig.create_po( !f1 );

  const auto f2 = mig.create_maj( f1, x4, c0 );
  mig.create_ri( f2 );

  const auto ro = mig.create_ro();
  mig.create_po( ro );

  CHECK( mig.num_pos() == 3 );
  CHECK( mig.num_registers() == 1 );
  CHECK( !mig.is_combinational() );

  mig.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == f1 );
      break;
    case 1:
      CHECK( s == !f1 );
      break;
    case 2:
      // Check if the output (connected to the register) data is the same as the node data being registered.
      CHECK( f2.data == mig.po_at( i ).data );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
}

TEST_CASE( "create and use register in an xmg network", "[xmg]" )
{
  sequential<xmg_network> xmg;

  CHECK( has_foreach_po_v<sequential<xmg_network>> );
  CHECK( has_create_po_v<sequential<xmg_network>> );
  CHECK( has_create_pi_v<sequential<xmg_network>> );
  CHECK( has_create_ro_v<sequential<xmg_network>> );
  CHECK( has_create_ri_v<sequential<xmg_network>> );
  CHECK( has_create_maj_v<sequential<xmg_network>> );

  const auto c0 = xmg.get_constant( false );
  const auto x1 = xmg.create_pi();
  const auto x2 = xmg.create_pi();
  const auto x3 = xmg.create_pi();
  const auto x4 = xmg.create_pi();

  CHECK( xmg.size() == 5 );
  CHECK( xmg.num_registers() == 0 );
  CHECK( xmg.num_cis() == 4 );
  CHECK( xmg.num_cos() == 0 );

  const auto f1 = xmg.create_maj( x1, x2, x3 );
  xmg.create_po( f1 );

  CHECK( xmg.num_pos() == 1 );

  const auto s1 = xmg.create_ro(); // ntk. input
  xmg.create_po( s1 );             // po

  const auto f2 = xmg.create_maj( f1, x4, c0 );
  xmg.create_ri( f2 ); // ntk. output

  CHECK( xmg.num_registers() == 1 );
  CHECK( xmg.num_cis() == 4 + 1 );
  CHECK( xmg.num_cos() == 2 + 1 );

  xmg.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( xmg.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xmg.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xmg.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xmg.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xmg.make_signal( node ) == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( xmg.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( xmg.make_signal( node ) == x1 ); /* first pi */
      break;
    case 1:
      CHECK( xmg.make_signal( node ) == x2 ); /* second pi */
      break;
    case 2:
      CHECK( xmg.make_signal( node ) == x3 ); /* third pi */
      break;
    case 3:
      CHECK( xmg.make_signal( node ) == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( xmg.make_signal( node ) == s1 ); /* first state-bit */
      CHECK( xmg.is_ci( node ) );
      CHECK( !xmg.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_po( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    default:
      CHECK( false );
    }
  } );

  xmg.foreach_co( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    case 2:
      CHECK( node == f2 ); /* first next-state bit */
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "create and use register in a k-LUT network", "[klut]" )
{
  sequential<klut_network> klut;

  CHECK( has_foreach_po_v<sequential<klut_network>> );
  CHECK( has_create_po_v<sequential<klut_network>> );
  CHECK( has_create_pi_v<sequential<klut_network>> );
  CHECK( has_create_ro_v<sequential<klut_network>> );
  CHECK( has_create_ri_v<sequential<klut_network>> );
  CHECK( has_create_maj_v<sequential<klut_network>> );

  const auto c0 = klut.get_constant( false );
  const auto x1 = klut.create_pi();
  const auto x2 = klut.create_pi();
  const auto x3 = klut.create_pi();
  const auto x4 = klut.create_pi();

  CHECK( klut.size() == 6 );
  CHECK( klut.num_registers() == 0 );
  CHECK( klut.num_cis() == 4 );
  CHECK( klut.num_cos() == 0 );

  const auto f1 = klut.create_maj( x1, x2, x3 );
  klut.create_po( f1 );

  CHECK( klut.num_pos() == 1 );

  const auto s1 = klut.create_ro(); // ntk. input
  klut.create_po( s1 );             // po

  const auto f2 = klut.create_maj( f1, x4, c0 );
  klut.create_ri( f2 ); // ntk. output

  CHECK( klut.num_registers() == 1 );
  CHECK( klut.num_cis() == 4 + 1 );
  CHECK( klut.num_cos() == 2 + 1 );

  klut.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( klut.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  klut.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( klut.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( node == s1 ); /* first state-bit */
      CHECK( klut.is_ci( node ) );
      CHECK( !klut.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  klut.foreach_po( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    default:
      CHECK( false );
    }
  } );

  klut.foreach_co( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    case 2:
      CHECK( node == f2 ); /* first next-state bit */
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "create and use register in a cover network", "[cover]" )
{
  sequential<cover_network> cover;

  CHECK( has_foreach_po_v<sequential<cover_network>> );
  CHECK( has_create_po_v<sequential<cover_network>> );
  CHECK( has_create_pi_v<sequential<cover_network>> );
  CHECK( has_create_ro_v<sequential<cover_network>> );
  CHECK( has_create_ri_v<sequential<cover_network>> );
  CHECK( has_create_maj_v<sequential<cover_network>> );

  const auto c0 = cover.get_constant( false );
  const auto x1 = cover.create_pi();
  const auto x2 = cover.create_pi();
  const auto x3 = cover.create_pi();
  const auto x4 = cover.create_pi();

  CHECK( cover.size() == 6 );
  CHECK( cover.num_registers() == 0 );
  CHECK( cover.num_cis() == 4 );
  CHECK( cover.num_cos() == 0 );

  const auto f1 = cover.create_maj( x1, x2, x3 );
  cover.create_po( f1 );

  CHECK( cover.num_pos() == 1 );

  const auto s1 = cover.create_ro(); // ntk. input
  cover.create_po( s1 );             // po

  const auto f2 = cover.create_maj( f1, x4, c0 );
  cover.create_ri( f2 ); // ntk. output

  CHECK( cover.num_registers() == 1 );
  CHECK( cover.num_cis() == 4 + 1 );
  CHECK( cover.num_cos() == 2 + 1 );

  cover.foreach_pi( [&]( auto const& node, auto index ) {
    CHECK( cover.is_pi( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    default:
      CHECK( false );
    }
  } );

  cover.foreach_ci( [&]( auto const& node, auto index ) {
    CHECK( cover.is_ci( node ) );
    switch ( index )
    {
    case 0:
      CHECK( node == x1 ); /* first pi */
      break;
    case 1:
      CHECK( node == x2 ); /* second pi */
      break;
    case 2:
      CHECK( node == x3 ); /* third pi */
      break;
    case 3:
      CHECK( node == x4 ); /* fourth pi */
      break;
    case 4:
      CHECK( node == s1 ); /* first state-bit */
      CHECK( cover.is_ci( node ) );
      CHECK( !cover.is_pi( node ) );
      break;
    default:
      CHECK( false );
    }
  } );

  cover.foreach_po( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    default:
      CHECK( false );
    }
  } );

  cover.foreach_co( [&]( auto const& node, auto index ) {
    switch ( index )
    {
    case 0:
      CHECK( node == f1 ); /* first po */
      break;
    case 1:
      CHECK( node == s1 ); /* second po */
      break;
    case 2:
      CHECK( node == f2 ); /* first next-state bit */
      break;
    default:
      CHECK( false );
    }
  } );
}

TEST_CASE( "create and use register in an AQFP", "[aqfp]" )
{
  sequential<aqfp_network> aqfp;

  CHECK( has_foreach_po_v<sequential<aqfp_network>> );
  CHECK( has_create_po_v<sequential<aqfp_network>> );
  CHECK( has_create_pi_v<sequential<aqfp_network>> );
  CHECK( has_create_ro_v<sequential<aqfp_network>> );
  CHECK( has_create_ri_v<sequential<aqfp_network>> );
  CHECK( has_create_maj_v<sequential<aqfp_network>> );

  const auto c0 = aqfp.get_constant( false );
  const auto x1 = aqfp.create_pi();
  const auto x2 = aqfp.create_pi();
  const auto x3 = aqfp.create_pi();
  const auto x4 = aqfp.create_pi();

  CHECK( aqfp.size() == 5 );
  CHECK( aqfp.num_registers() == 0 );
  CHECK( aqfp.num_pis() == 4 );
  CHECK( aqfp.num_pos() == 0 );
  CHECK( aqfp.is_combinational() );

  const auto f1 = aqfp.create_maj( x1, x2, x3 );
  aqfp.create_po( f1 );
  aqfp.create_po( !f1 );

  const auto f2 = aqfp.create_maj( f1, x4, c0 );
  aqfp.create_ri( f2 );

  const auto ro = aqfp.create_ro();
  aqfp.create_po( ro );

  CHECK( aqfp.num_pos() == 3 );
  CHECK( aqfp.num_registers() == 1 );
  CHECK( !aqfp.is_combinational() );

  aqfp.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == f1 );
      break;
    case 1:
      CHECK( s == !f1 );
      break;
    case 2:
      // Check if the output (connected to the register) data is the same as the node data being registered.
      CHECK( f2.data == aqfp.po_at( i ).data );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
}
