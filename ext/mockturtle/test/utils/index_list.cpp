#include <catch.hpp>

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/muxig.hpp>
#include <mockturtle/utils/index_list.hpp>

using namespace mockturtle;

TEST_CASE( "decode mig_index_list into mig_network", "[index_list]" )
{
  std::vector<uint32_t> const raw_list{ 4 | ( 1 << 8 ) | ( 2 << 16 ), 2, 4, 6, 10, 4, 8, 12 };
  mig_index_list mig_il{ raw_list };

  mig_network mig;
  decode( mig, mig_il );

  CHECK( mig.num_gates() == 2u );
  CHECK( mig.num_pis() == 4u );
  CHECK( mig.num_pos() == 1u );

  const auto tt = simulate<kitty::static_truth_table<4u>>( mig )[0];
  CHECK( tt._bits == 0xecc8 );
}

TEST_CASE( "encode mig_network into mig_index_list", "[index_list]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const t0 = mig.create_maj( a, b, c );
  auto const t1 = mig.create_maj( t0, b, d );
  mig.create_po( t1 );

  mig_index_list mig_il;
  encode( mig_il, mig );

  CHECK( mig_il.num_pis() == 4u );
  CHECK( mig_il.num_pos() == 1u );
  CHECK( mig_il.num_gates() == 2u );
  CHECK( mig_il.size() == 8u );
  CHECK( mig_il.raw() == std::vector<uint32_t>{ 4 | ( 1 << 8 ) | ( 2 << 16 ), 2, 4, 6, 4, 8, 10, 12 } );
  CHECK( to_index_list_string( mig_il ) == "{4 | 1 << 8 | 2 << 16, 2, 4, 6, 4, 8, 10, 12}" );
}

TEST_CASE( "decode abc_index_list into xag_network", "[index_list]" )
{
  std::vector<uint32_t> const raw_list{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 12, 10, 14, 14 };
  abc_index_list xag_il( raw_list, 4u );

  xag_network xag;
  decode( xag, xag_il );

  CHECK( xag.num_gates() == 3u );
  CHECK( xag.num_pis() == 4u );
  CHECK( xag.num_pos() == 1u );

  const auto tt = simulate<kitty::static_truth_table<4u>>( xag )[0];
  CHECK( tt._bits == 0x7888 );
}

TEST_CASE( "encode xag_network into abc_index_list", "[index_list]" )
{
  xag_network xag;
  auto const a = xag.create_pi();
  auto const b = xag.create_pi();
  auto const c = xag.create_pi();
  auto const d = xag.create_pi();
  auto const t0 = xag.create_and( a, b );
  auto const t1 = xag.create_and( c, d );
  auto const t2 = xag.create_xor( t0, t1 );
  xag.create_po( t2 );

  abc_index_list xag_il;
  encode( xag_il, xag );

  CHECK( xag_il.num_pis() == 4u );
  CHECK( xag_il.num_pos() == 1u );
  CHECK( xag_il.num_gates() == 3u );
  CHECK( xag_il.size() == 18u );
  CHECK( xag_il.raw() == std::vector<uint32_t>{ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 12, 10, 14, 14 } );
  CHECK( to_index_list_string( xag_il ) == "{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6, 8, 12, 10, 14, 14}" );
}

TEST_CASE( "decode xag_index_list into xag_network", "[index_list]" )
{
  std::vector<uint32_t> const raw_list{ 4 | ( 1 << 8 ) | ( 3 << 16 ), 2, 4, 6, 8, 12, 10, 14 };
  xag_index_list xag_il{ raw_list };

  xag_network xag;
  decode( xag, xag_il );

  CHECK( xag.num_gates() == 3u );
  CHECK( xag.num_pis() == 4u );
  CHECK( xag.num_pos() == 1u );

  const auto tt = simulate<kitty::static_truth_table<4u>>( xag )[0];
  CHECK( tt._bits == 0x7888 );
}

TEST_CASE( "encode xag_network into xag_index_list", "[index_list]" )
{
  xag_network xag;
  auto const a = xag.create_pi();
  auto const b = xag.create_pi();
  auto const c = xag.create_pi();
  auto const d = xag.create_pi();
  auto const t0 = xag.create_and( a, b );
  auto const t1 = xag.create_and( c, d );
  auto const t2 = xag.create_xor( t0, t1 );
  xag.create_po( t2 );

  xag_index_list xag_il;
  encode( xag_il, xag );

  CHECK( xag_il.num_pis() == 4u );
  CHECK( xag_il.num_pos() == 1u );
  CHECK( xag_il.num_gates() == 3u );
  CHECK( xag_il.size() == 8u );
  CHECK( xag_il.raw() == std::vector<uint32_t>{ 4 | ( 1 << 8 ) | ( 3 << 16 ), 2, 4, 6, 8, 12, 10, 14 } );
  CHECK( to_index_list_string( xag_il ) == "{4 | 1 << 8 | 3 << 16, 2, 4, 6, 8, 12, 10, 14}" );
}

TEST_CASE( "decode muxig_index_list into muxig_network", "[index_list]" )
{
  std::vector<uint32_t> const raw_list{ 4 | ( 1 << 8 ) | ( 2 << 16 ), 2, 4, 6, 10, 4, 8, 12 };
  muxig_index_list il{ raw_list };

  muxig_network ntk;
  decode( ntk, il );

  CHECK( ntk.num_gates() == 2u );
  CHECK( ntk.num_pis() == 4u );
  CHECK( ntk.num_pos() == 1u );

  const auto tt = simulate<kitty::static_truth_table<4u>>( ntk )[0];
  CHECK( tt._bits == 0xefc8 );
}
