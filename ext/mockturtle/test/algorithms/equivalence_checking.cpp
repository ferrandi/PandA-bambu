#include <catch.hpp>

#include <vector>

#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Equivalence check on two XAGs", "[equivalence_checking]" )
{
  xag_network xag1, xag2;

  const auto a = xag1.create_pi();
  const auto b = xag1.create_pi();

  const auto f1 = xag1.create_nand( a, b );
  const auto f2 = xag1.create_nand( a, f1 );
  const auto f3 = xag1.create_nand( b, f1 );
  const auto f4 = xag1.create_nand( f2, f3 );

  xag1.create_po( f4 );

  const auto a_ = xag2.create_pi();
  const auto b_ = xag2.create_pi();

  const auto f1_ = xag2.create_xor( a_, b_ );

  xag2.create_po( f1_ );

  const auto miter_ntk = *miter<xag_network>( xag1, xag2 );

  CHECK( miter_ntk.num_pos() == 1u );

  const auto result = equivalence_checking( miter_ntk );

  CHECK( result );
  CHECK( *result );
}

TEST_CASE( "Equivalence check on two non-equivalent AIGs", "[equivalence_checking]" )
{
  aig_network aig1, aig2;

  const auto a = aig1.create_pi();
  const auto b = aig1.create_pi();

  const auto f1 = aig1.create_nand( a, b );
  const auto f2 = aig1.create_nand( a, f1 );
  const auto f3 = aig1.create_nand( b, f1 );
  const auto f4 = aig1.create_nand( f2, f3 );

  aig1.create_po( f4 );

  const auto a_ = aig2.create_pi();
  const auto b_ = aig2.create_pi();

  const auto f1_ = aig2.create_or( a_, b_ );

  aig2.create_po( f1_ );

  const auto miter_ntk = *miter<aig_network>( aig1, aig2 );

  CHECK( miter_ntk.num_pos() == 1u );

  equivalence_checking_stats st;
  const auto result = equivalence_checking( miter_ntk, {}, &st );

  CHECK( result );
  CHECK( !*result );
  CHECK( st.counter_example == std::vector<bool>( { true, true } ) );
}
