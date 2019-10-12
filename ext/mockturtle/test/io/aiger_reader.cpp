#include <catch.hpp>

#include <sstream>
#include <string>

#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/networks/aig.hpp>

#include <lorina/aiger.hpp>

using namespace mockturtle;

TEST_CASE( "read and write names", "[aiger_reader]" )
{
  aig_network aig;

  std::string file{"aag 7 2 1 2 4\n"
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
  "o1 y1\n"};

  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( aig ) );
  CHECK( result == lorina::return_code::success );

  NameMap<aig_network,std::map<aig_network::signal,std::vector<std::string>>> names;
  names.insert( aig.make_signal( aig.pi_at( 0 ) ), "x0" );
  names.insert( aig.make_signal( aig.pi_at( 1 ) ), "x1" );
  names.insert( aig.make_signal( aig.ro_at( 0 ) ), "s0" );
  names.insert( aig.ri_at( 0 ), "s0_next" );
  names.insert( aig.po_at( 0 ), "y0" );
  names.insert( aig.po_at( 1 ), "y1" );

  CHECK( names.has_name( aig.make_signal( aig.pi_at( 0 ) ), "x0" ) );
  CHECK( names.has_name( aig.make_signal( aig.pi_at( 1 ) ), "x1" ) );
  CHECK( names.has_name( aig.po_at( 0 ), "y0" ) );
  CHECK( names.has_name( aig.po_at( 1 ), "y1" ) );
  CHECK( names.has_name( aig.make_signal( aig.ro_at( 0 ) ), "s0" ) );
  CHECK( names.has_name( aig.ri_at( 0 ), "s0_next" ) );
}

TEST_CASE( "read an ASCII Aiger file into an AIG network and store input-output names", "[aiger_reader]" )
{
  aig_network aig;

  std::string file{"aag 6 2 0 1 4\n"
  "2\n"
  "4\n"
  "13\n"
  "6 2 4\n"
  "8 2 7\n"
  "10 4 7\n"
  "12 9 11\n"
  "i0 foo\n"
  "i1 bar\n"
  "o0 foobar\n"};

  NameMap<aig_network> names;
  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( aig, &names ) );
  CHECK( result == lorina::return_code::success );
  CHECK( aig.size() == 7 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 1 );
  CHECK( aig.num_gates() == 4 );

  CHECK( names.has_name( aig.make_signal( aig.pi_at( 0 ) ), "foo" ) );
  CHECK( names.has_name( aig.make_signal( aig.pi_at( 1 ) ), "bar" ) );
  CHECK( names.has_name( aig.po_at( 0 ), "foobar" ) );
}

TEST_CASE( "read a sequential ASCII Aiger file into an AIG network", "[aiger_reader]" )
{
  aig_network aig;

  std::string file{"aag 7 2 1 2 4\n"
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
  "o1 barbar\n"};

  NameMap<aig_network> names;
  lorina::diagnostic_engine diag;
  std::istringstream in( file );
  auto const result = lorina::read_ascii_aiger( in, aiger_reader( aig, &names ), &diag );
  CHECK( result == lorina::return_code::success );
  CHECK( aig.size() == 8 );
  CHECK( aig.num_cis() == 3 );
  CHECK( aig.num_cos() == 3 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 4 );
  CHECK( aig.num_registers() == 1 );

  CHECK( names.has_name( aig.make_signal( aig.pi_at( 0 ) ), "foo" ) );
  CHECK( names.has_name( aig.make_signal( aig.pi_at( 1 ) ), "bar" ) );
  CHECK( names.has_name( aig.make_signal( aig.ro_at( 0 ) ), "barfoo" ) );
  CHECK( names.has_name( aig.ri_at( 0 ), "barfoo_next" ) );
  CHECK( names.has_name( aig.po_at( 0 ), "foobar" ) );
  CHECK( names.has_name( aig.po_at( 1 ), "barbar" ) );
}
