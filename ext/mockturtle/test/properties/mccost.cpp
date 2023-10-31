#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "count MC in AIG", "[mccost]" )
{
  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( multiplicative_complexity( aig ) == 4u );
  CHECK( multiplicative_complexity_depth( aig ) == 3u );
}

template<class Ntk>
inline void test_ripple_carry_adder( uint32_t width, uint32_t expected_count, uint32_t expected_depth )
{
  Ntk ntk;

  std::vector<typename Ntk::signal> as( width ), bs( width );
  std::generate( as.begin(), as.end(), [&]() { return ntk.create_pi(); } );
  std::generate( bs.begin(), bs.end(), [&]() { return ntk.create_pi(); } );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, as, bs, carry );
  std::for_each( as.begin(), as.end(), [&]( auto const& f ) { ntk.create_po( f ); } );
  ntk.create_po( carry );

  CHECK( multiplicative_complexity( ntk ) == expected_count );
  CHECK( multiplicative_complexity_depth( ntk ) == expected_depth );
}

TEST_CASE( "count MC in ripple carry adder", "[mccost]" )
{
  test_ripple_carry_adder<aig_network>( 16u, 108u, 32u );
  test_ripple_carry_adder<xag_network>( 16u, 16u, 16u );
}
