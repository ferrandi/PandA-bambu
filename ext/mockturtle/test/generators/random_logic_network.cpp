#include <catch.hpp>

#include <mockturtle/generators/random_logic_generator.hpp>

using namespace mockturtle;

TEST_CASE( "create random aig_network", "[random_logic_generator]" )
{
  auto const gen = default_random_aig_generator();
  auto const aig = gen.generate( 4u, 100u );

  CHECK( aig.num_pis() == 4u );
  CHECK( aig.num_gates() == 100u );
}

TEST_CASE( "create random mig_network", "[random_logic_generator]" )
{
  auto const gen = default_random_mig_generator();
  auto const mig = gen.generate( 4u, 100u );

  CHECK( mig.num_pis() == 4u );
  CHECK( mig.num_gates() == 100u );
}

TEST_CASE( "create random mig_network with un-real majority nodes", "[random_logic_generator]" )
{
  auto const gen = mixed_random_mig_generator();
  auto const mig = gen.generate( 4u, 100u );

  CHECK( mig.num_pis() == 4u );
  CHECK( mig.num_gates() == 100u );
}
