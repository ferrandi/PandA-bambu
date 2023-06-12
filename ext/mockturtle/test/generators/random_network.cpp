#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/generators/random_network.hpp>
#include <mockturtle/io/write_verilog.hpp>

using namespace mockturtle;

TEST_CASE( "create random aig_network", "[random_network_generator]" )
{
  random_network_generator_params_size ps;
  ps.num_pis = 4u;
  ps.num_gates = 100u;

  auto gen = random_aig_generator( ps );
  auto const aig = gen.generate();

  CHECK( aig.num_pis() == ps.num_pis );
  CHECK( aig.num_gates() == ps.num_gates );
}

TEST_CASE( "create random mig_network", "[random_network_generator]" )
{
  random_network_generator_params_size ps;
  ps.num_pis = 4u;
  ps.num_gates = 100u;

  auto gen = random_mig_generator( ps );
  auto const aig = gen.generate();

  CHECK( aig.num_pis() == ps.num_pis );
  CHECK( aig.num_gates() == ps.num_gates );
}

TEST_CASE( "create random mig_network with un-real majority nodes", "[random_network_generator]" )
{
  random_network_generator_params_size ps;
  ps.num_pis = 4u;
  ps.num_gates = 100u;

  auto gen = mixed_random_mig_generator( ps );
  auto const aig = gen.generate();

  CHECK( aig.num_pis() == ps.num_pis );
  CHECK( aig.num_gates() == ps.num_gates );
}
