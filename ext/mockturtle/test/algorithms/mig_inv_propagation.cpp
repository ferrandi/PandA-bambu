#include <catch.hpp>

#include <mockturtle/algorithms/mig_inv_propagation.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

namespace mockturtle
{
namespace inv_prop_test
{
template<typename Ntk>
int number_of_inverted( Ntk const& ntk )
{
  int num_inverted{ 0 };
  ntk.foreach_gate( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_dead( ntk.get_node( f ) ) )
      {
        std::cerr << "dead node " << ntk.get_node( f ) << std::endl;
        return;
      }
      if ( ntk.is_constant( ntk.get_node( f ) ) || ntk.is_pi( ntk.get_node( f ) ) )
      {
        return;
      }
      if ( ntk.is_complemented( f ) )
      {
        num_inverted++;
      }
    } );
  } );
  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_complemented( f ) )
    {
      num_inverted++;
    }
  } );
  return num_inverted;
}
} // namespace inv_prop_test
} // namespace mockturtle

TEST_CASE( "MIG inverter propagation basic", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();

  const auto f1 = mig.create_maj( !a, b, c );
  const auto f2 = mig.create_maj( !a, b, d );
  const auto f3 = mig.create_maj( a, !f1, f2 );
  const auto f4 = mig.create_maj( a, !f1, b );

  mig.create_po( f3 );
  mig.create_po( f4 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}

TEST_CASE( "MIG inverter propagation constant input 0", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f1 = mig.create_maj( !a, b, mig.get_constant( 0 ) );
  const auto f2 = mig.create_maj( !a, b, c );
  const auto f3 = mig.create_maj( a, !f1, f2 );

  mig.create_po( f3 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}

TEST_CASE( "MIG inverter propagation constant input 1", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();

  const auto f1 = mig.create_maj( a, b, mig.get_constant( 1 ) );
  const auto f2 = mig.create_maj( !a, b, c );
  const auto f3 = mig.create_maj( a, !f1, f2 );
  const auto f4 = mig.create_maj( a, !f1, c );

  mig.create_po( f3 );
  mig.create_po( f4 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}

TEST_CASE( "MIG inverter propagation output", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();

  const auto f1 = mig.create_maj( !a, b, c );
  const auto f2 = mig.create_maj( !a, b, d );
  const auto f3 = mig.create_maj( a, !f1, f2 );

  mig.create_po( f3 );
  mig.create_po( !f1 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}

TEST_CASE( "MIG inverter propagation complex", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto zero = mig.get_constant( false );

  const auto x1 = mig.create_pi();
  const auto x2 = mig.create_pi();
  const auto x3 = mig.create_pi();

  const auto y1 = mig.create_maj( x1, !x2, x3 );
  const auto y2 = mig.create_maj( zero, !x2, x3 );

  const auto z1 = mig.create_maj( y1, y2, !x3 );
  const auto z2 = mig.create_maj( x2, x3, !y2 );
  const auto z3 = mig.create_maj( zero, x1, !y2 );
  const auto z4 = mig.create_maj( x2, !y1, zero );
  const auto z5 = mig.create_maj( x1, !y1, zero );

  const auto t1 = mig.create_maj( z1, z2, !z3 );
  const auto t2 = mig.create_maj( z1, !x1, zero );

  mig.create_po( !t1 );
  mig.create_po( !t2 );
  mig.create_po( z4 );
  mig.create_po( z5 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}

TEST_CASE( "MIG inverter propagation two level", "[mig_inv_propagation]" )
{
  mig_network mig;
  mig_inv_propagation_stats st;

  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  const auto d = mig.create_pi();

  const auto f1 = mig.create_maj( !a, b, c );
  const auto f2 = mig.create_maj( !a, b, d );
  const auto f3 = mig.create_maj( !a, f1, f2 );
  const auto f4 = mig.create_maj( a, !f1, f2 );
  const auto f5 = mig.create_maj( a, !f1, f4 );

  mig.create_po( !f3 );
  mig.create_po( f5 );

  mig_inv_propagation( mig, &st );
  auto inv_count = inv_prop_test::number_of_inverted( mig );
  CHECK( inv_count == 0 );
}