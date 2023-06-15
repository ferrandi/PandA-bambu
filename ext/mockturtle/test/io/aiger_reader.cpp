#include <catch.hpp>

#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/views/names_view.hpp>

#include <lorina/aiger.hpp>

#include <sstream>
#include <string>

using namespace mockturtle;

TEST_CASE( "read and write names", "[aiger_reader]" )
{
  sequential<aig_network> aig;
  names_view<sequential<aig_network>> named_aig{ aig };

  std::string file{ "aag 7 2 1 2 4\n"
                    "2\n"
                    "4\n"
                    "6 8\n"
                    "6\n"
                    "7\n"
                    "8 2 6\n"
                    "10 3 7\n"
                    "12 9 11\n"
                    "14 4 12\n"
                    "i0 x0\n"
                    "i1 x1\n"
                    "l0 s0\n"
                    "o0 y0\n"
                    "o1 y1\n" };

  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( named_aig ) );
  CHECK( result == lorina::return_code::success );

  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 0 ) ) ) == "x0" );
  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 1 ) ) ) == "x1" );
  CHECK( named_aig.get_name( aig.make_signal( aig.ro_at( 0 ) ) ) == "s0" );
  CHECK( named_aig.get_name( aig.ri_at( 0 ) ) == "s0_next" );
  CHECK( named_aig.get_output_name( 0 ) == "y0" );
  CHECK( named_aig.get_output_name( 1 ) == "y1" );
}

TEST_CASE( "read an ASCII Aiger file into an AIG network and store input-output names", "[aiger_reader]" )
{
  aig_network aig;
  names_view<aig_network> named_aig{ aig };

  std::string file{ "aag 6 2 0 1 4\n"
                    "2\n"
                    "4\n"
                    "13\n"
                    "6 2 4\n"
                    "8 2 7\n"
                    "10 4 7\n"
                    "12 9 11\n"
                    "i0 foo\n"
                    "i1 bar\n"
                    "o0 foobar\n" };

  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( named_aig ) );
  CHECK( result == lorina::return_code::success );
  CHECK( named_aig.size() == 7 );
  CHECK( named_aig.num_pis() == 2 );
  CHECK( named_aig.num_pos() == 1 );
  CHECK( named_aig.num_gates() == 4 );

  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 0 ) ) ) == "foo" );
  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 1 ) ) ) == "bar" );
  CHECK( named_aig.get_output_name( 0 ) == "foobar" );
}

TEST_CASE( "read a sequential ASCII Aiger file into an AIG network", "[aiger_reader]" )
{
  sequential<aig_network> aig;
  names_view<sequential<aig_network>> named_aig{ aig };

  std::string file{ "aag 7 2 1 2 4\n"
                    "2\n"
                    "4\n"
                    "6 8\n"
                    "6\n"
                    "7\n"
                    "8 2 6\n"
                    "10 3 7\n"
                    "12 9 11\n"
                    "14 4 12\n"
                    "i0 foo\n"
                    "i1 bar\n"
                    "l0 barfoo\n"
                    "o0 foobar\n"
                    "o1 barbar\n" };

  lorina::text_diagnostics consumer;
  lorina::diagnostic_engine diag( &consumer );
  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( named_aig ), &diag );
  CHECK( result == lorina::return_code::success );
  CHECK( named_aig.size() == 8 );
  CHECK( named_aig.num_cis() == 3 );
  CHECK( named_aig.num_cos() == 3 );
  CHECK( named_aig.num_pis() == 2 );
  CHECK( named_aig.num_pos() == 2 );
  CHECK( named_aig.num_gates() == 4 );
  CHECK( named_aig.num_registers() == 1 );

  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 0 ) ) ) == "foo" );
  CHECK( named_aig.get_name( aig.make_signal( aig.pi_at( 1 ) ) ) == "bar" );
  CHECK( named_aig.get_name( aig.make_signal( aig.ro_at( 0 ) ) ) == "barfoo" );
  CHECK( named_aig.get_name( aig.ri_at( 0 ) ) == "barfoo_next" );
  CHECK( named_aig.get_output_name( 0 ) == "foobar" );
  CHECK( named_aig.get_output_name( 1 ) == "barbar" );
}
