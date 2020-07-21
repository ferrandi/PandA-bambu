#include <catch.hpp>

#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/views/cnf_view.hpp>

using namespace mockturtle;

TEST_CASE( "create a simple miter of equivalent functions with cnf_view", "[cnf_view]" )
{
  cnf_view<xag_network> xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f = xag.create_xor( a, b );
  const auto g = xag.create_or( xag.create_and( !a, b ), xag.create_and( a, !b ) );
  xag.create_po( xag.create_xor( f, g ) );

  const auto result = xag.solve();
  CHECK( result );   /* has result */
  CHECK( !*result ); /* result is UNSAT */
}

TEST_CASE( "create a simple miter of non-equivalent functions with cnf_view", "[cnf_view]" )
{
  cnf_view<xag_network> xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f = xag.create_or( a, b );
  const auto g = xag.create_xor( a, b );
  xag.create_po( xag.create_xor( f, g ) );

  const auto result = xag.solve();
  CHECK( result );  /* has result */
  CHECK( *result ); /* result is SAT */
  CHECK( xag.pi_model_values() == std::vector<bool>{{true, true}} );
}

TEST_CASE( "cnf_view with custom clauses", "[cnf_view]" )
{
  cnf_view<mig_network> mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  mig.create_po( mig.create_maj( a, b, c ) );

  mig.add_clause( ~mig.lit( a ) );
  mig.add_clause( mig.lit( b ) );
  const auto result = mig.solve();
  CHECK( result );
  CHECK( *result );
  CHECK( !mig.model_value( mig.get_node( a ) ) );
  CHECK( mig.model_value( mig.get_node( b ) ) );
  CHECK( mig.model_value( mig.get_node( c ) ) );
}

TEST_CASE( "find multiple solutions with cnf_view", "[cnf_view]" )
{
  cnf_view<mig_network> mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  mig.create_po( mig.create_maj( a, b, c ) );

  auto ctr = 0u;
  while ( *mig.solve() )
  {
    ++ctr;
    const auto solution = mig.pi_model_values();
    CHECK( std::count( solution.begin(), solution.end(), true ) >= 2 );
    mig.block();
  }
  CHECK( ctr == 4u );
}

/* cnf_view with AllowModify option */

TEST_CASE( "modify network", "[cnf_view]" )
{
  cnf_view<xag_network, true> xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();

  const auto f = xag.create_and( a, b );
  xag.create_po( f );
  const auto g = xag.create_xor( a, b );
  xag.substitute_node( xag.get_node( f ), g );

  const auto result = xag.solve();
  CHECK( result );
  CHECK( *result );
  CHECK( xag.model_value( xag.get_node( a ) ) != xag.model_value( xag.get_node( b ) ) );
}

TEST_CASE( "deactivate node", "[cnf_view]" )
{
  cnf_view<xag_network, true> xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto f = xag.create_xor( a, b );
  xag.create_po( f );

  /* virtually replacing XOR with AND */
  xag.deactivate( xag.get_node( f ) );
  CHECK( !xag.is_activated( xag.get_node( f ) ) );
  xag.add_clause( xag.lit( a ), ~xag.lit( f ) );
  xag.add_clause( xag.lit( b ), ~xag.lit( f ) );
  xag.add_clause( ~xag.lit( a ), ~xag.lit( b ), xag.lit( f ) );

  const auto result = xag.solve();
  CHECK( result );
  CHECK( *result );
  CHECK( xag.model_value( xag.get_node( a ) ) );
  CHECK( xag.model_value( xag.get_node( b ) ) );
}

TEST_CASE( "build cnf_view on top of existing network and create_pi afterwards", "[cnf_view]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  mig.create_po( mig.create_maj( a, b, c ) );

  cnf_view view( mig );
  const auto d = view.create_pi();
  view.create_po( view.create_maj( a, b, d ) );
  view.create_po( !a );

  const auto result = view.solve();
  CHECK( result );
  CHECK( *result );
  CHECK( !view.model_value( view.get_node( a ) ) );
  CHECK( view.model_value( view.get_node( b ) ) );
  CHECK( view.model_value( view.get_node( c ) ) );
  CHECK( view.model_value( view.get_node( d ) ) );
}

TEST_CASE( "build cnf_view for k-LUT network", "[cnf_view]" )
{
  cnf_view<klut_network> ntk;
  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();
  ntk.create_po( ntk.create_not( ntk.create_xor( ntk.create_maj( a, b, c ), ntk.create_xor3( a, b, c ) ) ) );

  auto result = ntk.solve();
  CHECK( result );
  CHECK( *result );
  auto v = ntk.pi_model_values();
  auto check = ( v == std::vector<bool>{{true, true, true}} ) || ( v == std::vector<bool>{{false, false, false}} );
  CHECK( check );
  ntk.block();
  result = ntk.solve();
  CHECK( result );
  CHECK( *result );
  v = ntk.pi_model_values();
  check = ( v == std::vector<bool>{{true, true, true}} ) || ( v == std::vector<bool>{{false, false, false}} );
  CHECK( check );
  ntk.block();
  result = ntk.solve();
  CHECK( result );
  CHECK( !*result );
}

TEST_CASE( "destructor", "[cnf_view]" )
{
  mig_network mig;
  const auto a = mig.create_pi();
  const auto b = mig.create_pi();
  const auto c = mig.create_pi();
  mig.create_po( mig.create_maj( a, b, c ) );

  {
    cnf_view view( mig );
    mig.events().on_add.push_back( []( auto const& n ) { (void)n; } );
  }
  
  CHECK( mig.events().on_add.size() == 1 );
  CHECK( mig.events().on_delete.size() == 0 );
}
