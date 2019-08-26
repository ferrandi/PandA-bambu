#include <catch.hpp>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>

using namespace mockturtle;

TEST_CASE( "write k-LUT network to BENCH file", "[write_bench]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto [sum, carry] = full_adder( klut, a, b, c );
  klut.create_po( sum );
  klut.create_po( carry );

  std::ostringstream out;
  write_bench( klut, out );

  CHECK( out.str() == "INPUT(n2)\n"
                      "INPUT(n3)\n"
                      "INPUT(n4)\n"
                      "OUTPUT(po0)\n"
                      "OUTPUT(po1)\n"
                      "n0 = gnd\n"
                      "n1 = vdd\n"
                      "n5 = LUT 0x96 (n2, n3, n4)\n"
                      "n6 = LUT 0xe8 (n2, n3, n4)\n"
                      "po0 = LUT 0x2 (n5)\n"
                      "po1 = LUT 0x2 (n6)\n" );
}

TEST_CASE( "write single-gate AIG into BENCH file", "[write_bench]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_or( a, b );
  aig.create_po( f1 );

  std::ostringstream out;
  write_bench( aig, out );

  CHECK( out.str() == "INPUT(n1)\n"
                      "INPUT(n2)\n"
                      "OUTPUT(po0)\n"
                      "n0 = gnd\n"
                      "n3 = LUT 0x1 (n1, n2)\n"
                      "po0 = LUT 0x1 (n3)\n" );
}

TEST_CASE( "write AIG for XOR into BENCH file", "[write_bench]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  std::ostringstream out;
  write_bench( aig, out );

  CHECK( out.str() == "INPUT(n1)\n"
                      "INPUT(n2)\n"
                      "OUTPUT(po0)\n"
                      "n0 = gnd\n"
                      "n3 = LUT 0x8 (n1, n2)\n"
                      "n4 = LUT 0x2 (n1, n3)\n"
                      "n5 = LUT 0x2 (n2, n3)\n"
                      "n6 = LUT 0x1 (n4, n5)\n"
                      "po0 = LUT 0x1 (n6)\n" );
}
