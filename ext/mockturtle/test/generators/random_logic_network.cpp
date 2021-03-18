#include <catch.hpp>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/generators/random_logic_generator.hpp>
#include <mockturtle/io/write_verilog.hpp>

using namespace mockturtle;

TEST_CASE( "create random aig_network", "[random_logic_generator]" )
{
  uint64_t const num_pis{4u};
  uint64_t const num_gates{100u};

  auto const gen = default_random_aig_generator();
  auto const aig = gen.generate( num_pis, num_gates );

  CHECK( aig.num_pis() == num_pis );
  CHECK( aig.num_gates() == num_gates );
}

TEST_CASE( "create random aig_network2", "[random_logic_generator]" )
{
  uint64_t const num_pis{4u};
  uint64_t const num_pos{3u};

  auto const gen = default_random_aig_generator();
  std::vector<uint32_t> structure{ 16, 16, 16, 8 };
  auto const aig = gen.generate2( num_pis, num_pos, structure );
  CHECK( aig.num_pis() == num_pis );
  CHECK( aig.num_pos() == num_pos );

  auto const aig2 = cleanup_dangling( aig );
  CHECK( aig2.num_pis() == num_pis );
  CHECK( aig2.num_pos() == num_pos );
}

TEST_CASE( "create random mig_network", "[random_logic_generator]" )
{
  uint64_t const num_pis{4u};
  uint64_t const num_gates{100u};

  auto const gen = default_random_mig_generator();
  auto const mig = gen.generate( num_pis, num_gates );

  CHECK( mig.num_pis() == 4u );
  CHECK( mig.num_gates() == 100u );
}

TEST_CASE( "create random mig_network with un-real majority nodes", "[random_logic_generator]" )
{
  uint64_t const num_pis{4u};
  uint64_t const num_gates{100u};

  auto const gen = mixed_random_mig_generator();
  auto const mig = gen.generate( num_pis, num_gates );

  CHECK( mig.num_pis() == num_pis );
  CHECK( mig.num_gates() == num_gates );
}
