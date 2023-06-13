#include <catch.hpp>

#include <sstream>

#include <lorina/genlib.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/binding_view.hpp>

using namespace mockturtle;

std::string const simple_library = "GATE zero 0 O=CONST0;\n"
                                   "GATE one 0 O=CONST1;\n"
                                   "GATE inverter 1 O=!a; PIN * INV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE buffer 2 O=a; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE and 5 O=a*b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n"
                                   "GATE or 5 O=a+b; PIN * NONINV 1 999 1.0 1.0 1.0 1.0\n";

TEST_CASE( "Create binding view", "[binding_view]" )
{
  std::vector<gate> gates;

  std::istringstream in( simple_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  binding_view<klut_network> ntk( gates );

  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();

  auto const c0 = ntk.get_constant( false );
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_or( c, d );
  auto const f = ntk.create_and( t1, t2 );
  auto const g = ntk.create_not( a );

  ntk.create_po( f );
  ntk.create_po( g );
  ntk.create_po( ntk.get_constant() );

  ntk.add_binding( ntk.get_node( c0 ), 0 );
  ntk.add_binding( ntk.get_node( t1 ), 4 );
  ntk.add_binding( ntk.get_node( t2 ), 5 );
  ntk.add_binding( ntk.get_node( f ), 4 );
  ntk.add_binding( ntk.get_node( g ), 2 );

  CHECK( ntk.has_binding( ntk.get_node( a ) ) == false );
  CHECK( ntk.has_binding( ntk.get_node( b ) ) == false );
  CHECK( ntk.has_binding( ntk.get_node( c ) ) == false );
  CHECK( ntk.has_binding( ntk.get_node( d ) ) == false );
  CHECK( ntk.has_binding( ntk.get_node( c0 ) ) == true );
  CHECK( ntk.has_binding( ntk.get_node( t1 ) ) == true );
  CHECK( ntk.has_binding( ntk.get_node( t2 ) ) == true );
  CHECK( ntk.has_binding( ntk.get_node( f ) ) == true );
  CHECK( ntk.has_binding( ntk.get_node( g ) ) == true );

  CHECK( ntk.get_binding_index( ntk.get_node( c0 ) ) == 0 );
  CHECK( ntk.get_binding_index( ntk.get_node( t1 ) ) == 4 );
  CHECK( ntk.get_binding_index( ntk.get_node( t2 ) ) == 5 );
  CHECK( ntk.get_binding_index( ntk.get_node( f ) ) == 4 );
  CHECK( ntk.get_binding_index( ntk.get_node( g ) ) == 2 );

  CHECK( ntk.get_binding( ntk.get_node( c0 ) ).name == "zero" );
  CHECK( ntk.get_binding( ntk.get_node( t1 ) ).name == "and" );
  CHECK( ntk.get_binding( ntk.get_node( t2 ) ).name == "or" );
  CHECK( ntk.get_binding( ntk.get_node( f ) ).name == "and" );
  CHECK( ntk.get_binding( ntk.get_node( g ) ).name == "inverter" );

  CHECK( ntk.compute_area() == 16 );
  CHECK( ntk.compute_worst_delay() == 2 );

  std::stringstream report_stats;
  ntk.report_stats( report_stats );
  CHECK( report_stats.str() == "[i] Report stats: area = 16.00; delay =  2.00;\n" );

  std::stringstream report_gates;
  ntk.report_gates_usage( report_gates );
  CHECK( report_gates.str() == "[i] Report gates usage:\n"
                               "[i] zero                     \t Instance =          1\t Area =         0.00     0.00 %\n"
                               "[i] inverter                 \t Instance =          1\t Area =         1.00     6.25 %\n"
                               "[i] and                      \t Instance =          2\t Area =        10.00    62.50 %\n"
                               "[i] or                       \t Instance =          1\t Area =         5.00    31.25 %\n"
                               "[i] TOTAL                    \t Instance =          5\t Area =        16.00   100.00 %\n" );
}

TEST_CASE( "Binding view on copy", "[binding_view]" )
{
  std::vector<gate> gates;

  std::istringstream in( simple_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );

  CHECK( result == lorina::return_code::success );

  binding_view<klut_network> ntk( gates );

  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();

  auto const c0 = ntk.get_constant( false );
  auto const t1 = ntk.create_and( a, b );
  auto const t2 = ntk.create_or( c, d );
  auto const f = ntk.create_and( t1, t2 );
  auto const g = ntk.create_not( a );

  ntk.create_po( f );
  ntk.create_po( g );
  ntk.create_po( ntk.get_constant() );

  ntk.add_binding( ntk.get_node( c0 ), 0 );
  ntk.add_binding( ntk.get_node( t1 ), 4 );
  ntk.add_binding( ntk.get_node( t2 ), 5 );
  ntk.add_binding( ntk.get_node( f ), 4 );
  ntk.add_binding( ntk.get_node( g ), 2 );

  binding_view<klut_network> ntk_copy = ntk;

  CHECK( ntk_copy.has_binding( ntk_copy.get_node( a ) ) == false );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( b ) ) == false );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( c ) ) == false );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( d ) ) == false );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( c0 ) ) == true );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( t1 ) ) == true );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( t2 ) ) == true );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( f ) ) == true );
  CHECK( ntk_copy.has_binding( ntk_copy.get_node( g ) ) == true );

  CHECK( ntk_copy.get_binding_index( ntk_copy.get_node( c0 ) ) == 0 );
  CHECK( ntk_copy.get_binding_index( ntk_copy.get_node( t1 ) ) == 4 );
  CHECK( ntk_copy.get_binding_index( ntk_copy.get_node( t2 ) ) == 5 );
  CHECK( ntk_copy.get_binding_index( ntk_copy.get_node( f ) ) == 4 );
  CHECK( ntk_copy.get_binding_index( ntk_copy.get_node( g ) ) == 2 );

  CHECK( ntk_copy.get_binding( ntk_copy.get_node( c0 ) ).name == "zero" );
  CHECK( ntk_copy.get_binding( ntk_copy.get_node( t1 ) ).name == "and" );
  CHECK( ntk_copy.get_binding( ntk_copy.get_node( t2 ) ).name == "or" );
  CHECK( ntk_copy.get_binding( ntk_copy.get_node( f ) ).name == "and" );
  CHECK( ntk_copy.get_binding( ntk_copy.get_node( g ) ).name == "inverter" );

  CHECK( ntk_copy.compute_area() == 16 );
  CHECK( ntk_copy.compute_worst_delay() == 2 );
}
