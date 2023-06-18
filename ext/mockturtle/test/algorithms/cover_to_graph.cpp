#include <catch.hpp>

#include <algorithm>

#include <mockturtle/views/names_view.hpp>

#include <mockturtle/algorithms/cover_to_graph.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/cover.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

#include <lorina/blif.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/write_blif.hpp>

using namespace mockturtle;

TEST_CASE( "Creation of a simple three node aig network: only and nodes", "[cover_to_graph]" )
{
  cover_network cover;
  aig_network aig;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();

  auto a11 = aig.create_pi();
  auto a12 = aig.create_pi();
  auto a13 = aig.create_pi();
  auto a14 = aig.create_pi();

  auto c2 = cover.create_and( c11, c12 );
  auto a2 = aig.create_and( a11, a12 );

  auto c3 = cover.create_and( c13, c2 );
  auto a3 = aig.create_and( a13, a2 );

  auto c4 = cover.create_and( c3, c14 );
  auto a4 = aig.create_and( a3, a14 );

  cover.create_po( c4 );
  aig.create_po( a4 );

  aig_network aig_cp1;
  convert_cover_to_graph( aig_cp1, cover );

  aig_network aig_cp2;
  aig_cp2 = convert_cover_to_graph<aig_network>( cover );

  names_view<aig_network> aig_cp3;
  convert_cover_to_graph( aig_cp3, cover );

  names_view<aig_network> aig_cp4{ convert_cover_to_graph<aig_network>( cover ) };

  CHECK( aig.num_gates() == 3u );
  CHECK( aig_cp1.num_gates() == 3u );
  CHECK( aig_cp2.num_gates() == 3u );
  CHECK( aig_cp3.num_gates() == 3u );
  CHECK( aig_cp4.num_gates() == 3u );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == 0x8000 );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits == 0x8000 );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp1 )[0]._bits == 0x8000 );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp2 )[0]._bits == 0x8000 );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp3 )[0]._bits == 0x8000 );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp4 )[0]._bits == 0x8000 );
}

TEST_CASE( "Creation of a simple three node aig network: mixed gates", "[cover_to_graph]" )
{
  cover_network cover;
  aig_network aig;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();

  auto a11 = aig.create_pi();
  auto a12 = aig.create_pi();
  auto a13 = aig.create_pi();
  auto a14 = aig.create_pi();

  auto c2 = cover.create_nand( c11, c12 );
  auto a2 = aig.create_nand( a11, a12 );

  auto c3 = cover.create_xor( c13, c2 );
  auto a3 = aig.create_xor( a13, a2 );

  auto c4 = cover.create_or( c3, c14 );
  auto a4 = aig.create_or( a3, a14 );

  cover.create_po( c4 );
  aig.create_po( a4 );

  aig_network aig_cp1;
  convert_cover_to_graph( aig_cp1, cover );

  aig_network aig_cp2;
  aig_cp2 = convert_cover_to_graph<aig_network>( cover );

  names_view<aig_network> aig_cp3;
  convert_cover_to_graph( aig_cp3, cover );

  names_view<aig_network> aig_cp4{ convert_cover_to_graph<aig_network>( cover ) };

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp1 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp2 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp3 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp4 )[0]._bits == sim_reference );
}

TEST_CASE( "Creation of a simple three node xag network: mixed gates", "[cover_to_graph]" )
{
  cover_network cover;
  xag_network xag;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();

  auto a11 = xag.create_pi();
  auto a12 = xag.create_pi();
  auto a13 = xag.create_pi();
  auto a14 = xag.create_pi();

  auto c2 = cover.create_nand( c11, c12 );
  auto a2 = xag.create_nand( a11, a12 );

  auto c3 = cover.create_xor( c13, c2 );
  auto a3 = xag.create_xor( a13, a2 );

  auto c4 = cover.create_or( c3, c14 );
  auto a4 = xag.create_or( a3, a14 );

  cover.create_po( c4 );
  xag.create_po( a4 );

  xag_network xag_cp1;
  convert_cover_to_graph( xag_cp1, cover );

  xag_network xag_cp2;
  xag_cp2 = convert_cover_to_graph<xag_network>( cover );

  names_view<xag_network> xag_cp3;
  convert_cover_to_graph( xag_cp3, cover );

  names_view<xag_network> xag_cp4{ convert_cover_to_graph<xag_network>( cover ) };

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( xag )[0]._bits );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xag )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xag_cp1 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xag_cp2 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xag_cp3 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xag_cp4 )[0]._bits == sim_reference );
}

TEST_CASE( "Creation of a simple three node mig network: mixed gates", "[cover_to_graph]" )
{
  cover_network cover;
  mig_network mig;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();

  auto a11 = mig.create_pi();
  auto a12 = mig.create_pi();
  auto a13 = mig.create_pi();
  auto a14 = mig.create_pi();

  auto c2 = cover.create_nand( c11, c12 );
  auto a2 = mig.create_nand( a11, a12 );

  auto c3 = cover.create_xor( c13, c2 );
  auto a3 = mig.create_xor( a13, a2 );

  auto c4 = cover.create_or( c3, c14 );
  auto a4 = mig.create_or( a3, a14 );

  cover.create_po( c4 );
  mig.create_po( a4 );

  mig_network mig_cp1;
  convert_cover_to_graph( mig_cp1, cover );

  mig_network mig_cp2;
  mig_cp2 = convert_cover_to_graph<mig_network>( cover );

  names_view<mig_network> mig_cp3;
  convert_cover_to_graph( mig_cp3, cover );

  names_view<mig_network> mig_cp4{ convert_cover_to_graph<mig_network>( cover ) };

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( mig )[0]._bits );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( mig )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( mig_cp1 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( mig_cp2 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( mig_cp3 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( mig_cp4 )[0]._bits == sim_reference );
}

TEST_CASE( "Creation of a simple three node xmg network: mixed gates", "[cover_to_graph]" )
{
  cover_network cover;
  xmg_network xmg;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();

  auto a11 = xmg.create_pi();
  auto a12 = xmg.create_pi();
  auto a13 = xmg.create_pi();
  auto a14 = xmg.create_pi();

  auto c2 = cover.create_nand( c11, c12 );
  auto a2 = xmg.create_nand( a11, a12 );

  auto c3 = cover.create_xor( c13, c2 );
  auto a3 = xmg.create_xor( a13, a2 );

  auto c4 = cover.create_or( c3, c14 );
  auto a4 = xmg.create_or( a3, a14 );

  cover.create_po( c4 );
  xmg.create_po( a4 );

  xmg_network xmg_cp1;
  convert_cover_to_graph( xmg_cp1, cover );

  xmg_network xmg_cp2;
  xmg_cp2 = convert_cover_to_graph<xmg_network>( cover );

  names_view<xmg_network> xmg_cp3;
  convert_cover_to_graph( xmg_cp3, cover );

  names_view<xmg_network> xmg_cp4{ convert_cover_to_graph<xmg_network>( cover ) };

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( xmg )[0]._bits );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xmg )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xmg_cp1 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xmg_cp2 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xmg_cp3 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( xmg_cp4 )[0]._bits == sim_reference );
}

TEST_CASE( "Creation of an aig network containing all the features of the converter", "[cover_to_graph]" )
{
  cover_network cover;
  aig_network aig;

  auto c11 = cover.create_pi();
  auto c12 = cover.create_pi();
  auto c13 = cover.create_pi();
  auto c14 = cover.create_pi();
  auto c15 = cover.create_pi();
  auto c16 = cover.create_pi();

  auto a11 = aig.create_pi();
  auto a12 = aig.create_pi();
  auto a13 = aig.create_pi();
  auto a14 = aig.create_pi();
  auto a15 = aig.create_pi();
  auto a16 = aig.create_pi();

  kitty::cube _X11 = kitty::cube( "-11" );
  kitty::cube _1X1 = kitty::cube( "1-1" );
  kitty::cube _11X = kitty::cube( "11-" );
  kitty::cube _X00 = kitty::cube( "-00" );
  kitty::cube _0X0 = kitty::cube( "0-0" );
  kitty::cube _00X = kitty::cube( "00-" );
  kitty::cube _100 = kitty::cube( "100" );
  kitty::cube _010 = kitty::cube( "010" );
  kitty::cube _001 = kitty::cube( "001" );
  kitty::cube _111 = kitty::cube( "111" );

  std::vector<kitty::cube> cubes_maj1 = { _X11, _1X1, _11X };
  std::pair<std::vector<kitty::cube>, bool> cover_maj1 = std::make_pair( cubes_maj1, true );
  std::vector<kitty::cube> cubes_maj0 = { _X00, _0X0, _00X };
  std::pair<std::vector<kitty::cube>, bool> cover_maj0 = std::make_pair( cubes_maj0, false );

  std::vector<kitty::cube> cubes_xor = { _001, _010, _100, _111 };
  std::pair<std::vector<kitty::cube>, bool> cover_xor = std::make_pair( cubes_xor, true );

  auto c21 = cover.create_cover_node( { c11, c12, c13 }, cover_maj1 );
  auto c22 = cover.create_cover_node( { c11, c12, c13 }, cover_xor );
  auto c23 = cover.create_cover_node( { c14, c15, c16 }, cover_xor );
  auto c24 = cover.create_cover_node( { c14, c15, c16 }, cover_maj0 );

  auto c31 = cover.create_and( c21, c22 );
  auto c32 = cover.create_or( c23, c24 );
  auto c41 = cover.create_and( c31, c14 );
  auto co = cover.create_and( c41, c32 );

  cover.create_po( co );

  auto a21 = aig.create_maj( a11, a12, a13 );
  auto a22 = aig.create_xor3( a11, a12, a13 );
  auto a23 = aig.create_xor3( a14, a15, a16 );
  auto a24 = aig.create_maj( a14, a15, a16 );

  auto a31 = aig.create_and( a21, a22 );
  auto a32 = aig.create_or( a23, a24 );
  auto a41 = aig.create_and( a31, a14 );
  auto ao = aig.create_and( a41, a32 );

  aig.create_po( ao );

  aig_network aig_cp1;
  convert_cover_to_graph( aig_cp1, cover );

  aig_network aig_cp2;
  aig_cp2 = convert_cover_to_graph<aig_network>( cover );

  names_view<aig_network> aig_cp3;
  convert_cover_to_graph( aig_cp3, cover );

  names_view<aig_network> aig_cp4{ convert_cover_to_graph<aig_network>( cover ) };

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits );

  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp1 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp2 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp3 )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp4 )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model top\n"
      ".inputs a b c\n"
      ".outputs y1 y2\n"
      ".names a b n1\n"
      "11 1\n"
      ".names c n1 n2\n"
      "1- 1\n"
      "-1 1\n"
      ".names n2 y1\n"
      "0 1\n"
      ".names n2 y2\n"
      "1 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto c = aig.create_pi();

  auto n1 = aig.create_and( a, b );
  auto n2 = aig.create_or( n1, c );

  auto y1 = aig.create_not( n2 );
  auto y2 = aig.create_buf( n2 );

  aig.create_po( y1 );
  aig.create_po( y2 );

  auto const sim_reference = ( simulate<kitty::static_truth_table<4u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<4u>>( aig_cp )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<4u>>( cover )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: No don't cares", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model top\n"
      ".inputs a1 a2 \n"
      ".outputs y\n"
      ".names a1 a2 i0\n"
      "11 1\n"
      ".names a1 i1\n"
      "0 1\n"
      ".names a2 i2\n"
      "0 1\n"
      ".names i1 i2 i3\n"
      "11 1\n"
      ".names i0 i3 y\n"
      "00 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto b1 = aig.create_pi();
  auto b2 = aig.create_pi();

  auto n0 = aig.create_and( b1, b2 );
  auto n1 = aig.create_not( b1 );
  auto n2 = aig.create_not( b2 );
  auto n3 = aig.create_and( n1, n2 );
  auto z = aig.create_and( !n0, !n3 );

  aig.create_po( z );

  auto const sim_reference = ( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig_cp )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<2u>>( cover )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: No don't cares and some OFF set", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model top\n"
      ".inputs a1 a2 \n"
      ".outputs y\n"
      ".names a1 a2 i0\n"
      "11 1\n"
      ".names a1 i1\n"
      "1 0\n"
      ".names a2 i2\n"
      "1 0\n"
      ".names i1 i2 i3\n"
      "11 1\n"
      ".names i0 i3 y\n"
      "00 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto b1 = aig.create_pi();
  auto b2 = aig.create_pi();

  auto n0 = aig.create_and( b1, b2 );
  auto n1 = aig.create_not( b1 );
  auto n2 = aig.create_not( b2 );
  auto n3 = aig.create_and( n1, n2 );
  auto z = aig.create_and( !n0, !n3 );

  aig.create_po( z );

  auto const sim_reference = ( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig_cp )[0]._bits == sim_reference );
  CHECK( simulate<kitty::static_truth_table<2u>>( cover )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: some don't cares", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model top\n"
      ".inputs a1 a2 \n"
      ".outputs y\n"
      ".names a1 a2 i0\n"
      "11 1\n"
      ".names a1 i1\n"
      "0 1\n"
      ".names a2 i2\n"
      "0 1\n"
      ".names i1 i2 i3\n"
      "11 1\n"
      ".names i0 i3 y\n"
      "1- 1\n"
      "-1 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto b1 = aig.create_pi();
  auto b2 = aig.create_pi();

  auto n0 = aig.create_and( b1, b2 );
  auto n1 = aig.create_not( b1 );
  auto n2 = aig.create_not( b2 );
  auto n3 = aig.create_and( n1, n2 );

  auto y = aig.create_or( n0, n3 );

  aig.create_po( y );

  auto const sim_reference = ( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig_cp )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: some don't cares and some OFF-sets", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model top\n"
      ".inputs a1 a2 \n"
      ".outputs y\n"
      ".names a1 a2 i0\n"
      "11 1\n"
      ".names a1 i1\n"
      "0 1\n"
      ".names a2 i2\n"
      "0 1\n"
      ".names i1 i2 i3\n"
      "11 0\n"
      ".names i0 i3 y\n"
      "1- 0\n"
      "-1 0\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto b1 = aig.create_pi();
  auto b2 = aig.create_pi();

  auto n0 = aig.create_and( b1, b2 );
  auto n1 = aig.create_not( b1 );
  auto n2 = aig.create_not( b2 );
  auto n3 = aig.create_nand( n1, n2 );

  auto y = aig.create_and( !n0, !n3 );

  aig.create_po( y );

  auto const sim_reference = ( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig_cp )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: 3D cubes", "[cover_to_graph]" )
{
  cover_network cover;

  std::string file{
      ".model monitorBLIF\n"
      ".inputs a b c d e\n"
      ".outputs t1\n"
      ".names a b c internal1\n"
      "101 1\n"
      "011 1\n"
      ".names internal1 d internal2\n"
      "11 1\n"
      ".names internal2 e t1\n"
      "01 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto c = aig.create_pi();
  auto d = aig.create_pi();
  auto e = aig.create_pi();

  auto n1 = aig.create_and( a, c );
  auto n2 = aig.create_and( n1, !b );
  auto n3 = aig.create_and( b, c );
  auto n4 = aig.create_and( !a, n3 );
  auto i1 = aig.create_or( n4, n2 );

  auto i2 = aig.create_and( i1, d );
  auto y = aig.create_and( !i2, e );

  aig.create_po( y );

  auto const sim_reference = ( simulate<kitty::static_truth_table<5u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<5u>>( aig_cp )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: constants", "[cover_to_graph]" )
{
  cover_network cover;
  std::string file{
      ".model monitorBLIF\n"
      ".inputs a b \n"
      ".outputs y\n"
      ".names i1\n"
      "1\n"
      ".names a i1 i2\n"
      "11 1\n"
      ".names i2 b y\n"
      "11 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  aig_network aig;
  auto a = aig.create_pi();
  auto b = aig.create_pi();
  auto i1 = aig.get_constant( true );
  auto i2 = aig.create_and( a, i1 );
  auto y = aig.create_and( i2, b );

  aig.create_po( y );

  auto const sim_reference = ( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits );
  CHECK( simulate<kitty::static_truth_table<2u>>( aig_cp )[0]._bits == sim_reference );
}

TEST_CASE( "read a combinational BLIF file into cover network and map it to aig: many don't cares", "[cover_to_graph]" )
{
  cover_network cover;
  std::string file{
      ".model monitorBLIF\n"
      ".inputs a1 a2 a3 a4 a5 \n"
      ".outputs y\n"
      ".names a1 a2 a3 a4 a5 y\n"
      "1---- 1\n"
      "-1--- 1\n"
      "--1-- 1\n"
      "---1- 1\n"
      "----1 1\n"
      ".end\n" };

  std::istringstream in( file );
  auto result = lorina::read_blif( in, blif_reader( cover ) );

  aig_network aig_cp;
  convert_cover_to_graph( aig_cp, cover );

  /* structural checks */
  CHECK( result == lorina::return_code::success );

  CHECK( simulate<kitty::static_truth_table<5u>>( aig_cp )[0]._bits == simulate<kitty::static_truth_table<5u>>( cover )[0]._bits );
}
