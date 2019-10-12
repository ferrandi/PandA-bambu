#include <catch.hpp>

#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/io/write_dot.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>

using namespace mockturtle;

TEST_CASE( "write k-LUT network to DOT file", "[write_dot]" )
{
  klut_network klut;

  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto c = klut.create_pi();
  const auto [sum, carry] = full_adder( klut, a, b, c );
  klut.create_po( sum );
  klut.create_po( carry );

  std::ostringstream out;
  write_dot( klut, out );

  CHECK( out.str() == "digraph {\n"
                      "rankdir=BT;\n"
                      "0 [label=\"0\",shape=box,style=filled,fillcolor=snow2]\n"
                      "1 [label=\"1\",shape=box,style=filled,fillcolor=snow2]\n"
                      "2 [label=\"2\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "3 [label=\"3\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "4 [label=\"4\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "5 [label=\"5\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "6 [label=\"6\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "po0 [shape=invtriangle,style=filled,fillcolor=snow2]\n"
                      "po1 [shape=invtriangle,style=filled,fillcolor=snow2]\n"
                      "2 -> 5 [style=solid]\n"
                      "3 -> 5 [style=solid]\n"
                      "4 -> 5 [style=solid]\n"
                      "2 -> 6 [style=solid]\n"
                      "3 -> 6 [style=solid]\n"
                      "4 -> 6 [style=solid]\n"
                      "5 -> po0 [style=solid]\n"
                      "6 -> po1 [style=solid]\n"
                      "{rank = same; 0; 1; 2; 3; 4; }\n"
                      "{rank = same; 5; 6; }\n"
                      "{rank = same; po0; po1; }\n"
                      "}\n" );
}

TEST_CASE( "write single-gate AIG into DOT file", "[write_dot]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_or( a, b );
  aig.create_po( f1 );

  std::ostringstream out;
  write_dot( aig, out );

  CHECK( out.str() == "digraph {\n"
                      "rankdir=BT;\n"
                      "0 [label=\"0\",shape=box,style=filled,fillcolor=snow2]\n"
                      "1 [label=\"1\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "2 [label=\"2\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "3 [label=\"3\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "po0 [shape=invtriangle,style=filled,fillcolor=snow2]\n"
                      "1 -> 3 [style=dashed]\n"
                      "2 -> 3 [style=dashed]\n"
                      "3 -> po0 [style=dashed]\n"
                      "{rank = same; 0; 1; 2; }\n"
                      "{rank = same; 3; }\n"
                      "{rank = same; po0; }\n"
                      "}\n" );
}

TEST_CASE( "write AIG for XOR into DOT file", "[write_dot]" )
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
  write_dot( aig, out );

  CHECK( out.str() == "digraph {\n"
                      "rankdir=BT;\n"
                      "0 [label=\"0\",shape=box,style=filled,fillcolor=snow2]\n"
                      "1 [label=\"1\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "2 [label=\"2\",shape=triangle,style=filled,fillcolor=snow2]\n"
                      "3 [label=\"3\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "4 [label=\"4\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "5 [label=\"5\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "6 [label=\"6\",shape=ellipse,style=filled,fillcolor=white]\n"
                      "po0 [shape=invtriangle,style=filled,fillcolor=snow2]\n"
                      "1 -> 3 [style=solid]\n"
                      "2 -> 3 [style=solid]\n"
                      "1 -> 4 [style=solid]\n"
                      "3 -> 4 [style=dashed]\n"
                      "2 -> 5 [style=solid]\n"
                      "3 -> 5 [style=dashed]\n"
                      "4 -> 6 [style=dashed]\n"
                      "5 -> 6 [style=dashed]\n"
                      "6 -> po0 [style=dashed]\n"
                      "{rank = same; 0; 1; 2; }\n"
                      "{rank = same; 3; }\n"
                      "{rank = same; 4; 5; }\n"
                      "{rank = same; 6; }\n"
                      "{rank = same; po0; }\n"
                      "}\n" );
}
