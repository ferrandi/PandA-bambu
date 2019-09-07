#include <catch.hpp>

#include <numeric>
#include <random>
#include <vector>

#include <mockturtle/traits.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/modular_arithmetic.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>

using namespace mockturtle;

template<class IntType = uint64_t>
inline IntType to_int( std::vector<bool> const& sim )
{
  return std::accumulate( sim.rbegin(), sim.rend(), IntType( 0 ), []( auto x, auto y ) { return ( x << 1 ) + y; } );
}

template<typename Ntk>
void simulate_modular_adder( uint32_t op1, uint32_t op2 )
{
  Ntk ntk;

  std::vector<typename Ntk::signal> a( 8 ), b( 8 );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_adder_inplace( ntk, a, b );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  CHECK( ntk.num_pis() == 16 );
  CHECK( ntk.num_pos() == 8 );

  const auto simm = simulate<bool>( ntk, input_word_simulator( ( op1 << 8 ) + op2 ) );
  CHECK( simm.size() == 8 );
  const auto result = ( op1 + op2 ) % ( 1 << 8 );
  for ( auto i = 0; i < 8; ++i )
  {
    CHECK( ( ( result >> i ) & 1 ) == simm[i] );
  }
}

TEST_CASE( "build an 8-bit modular adder with different networks", "[modular_arithmetic]" )
{
  simulate_modular_adder<aig_network>( 37, 73 );
  simulate_modular_adder<aig_network>( 0, 255 );
  simulate_modular_adder<aig_network>( 0, 255 );
  simulate_modular_adder<aig_network>( 200, 200 );
  simulate_modular_adder<aig_network>( 120, 250 );

  simulate_modular_adder<mig_network>( 37, 73 );
  simulate_modular_adder<mig_network>( 0, 255 );
  simulate_modular_adder<mig_network>( 0, 255 );
  simulate_modular_adder<mig_network>( 200, 200 );
  simulate_modular_adder<mig_network>( 120, 250 );

  simulate_modular_adder<klut_network>( 37, 73 );
  simulate_modular_adder<klut_network>( 0, 255 );
  simulate_modular_adder<klut_network>( 0, 255 );
  simulate_modular_adder<klut_network>( 200, 200 );
  simulate_modular_adder<klut_network>( 120, 250 );
}

template<typename Ntk>
void simulate_modular_adder2( uint32_t op1, uint32_t op2, uint32_t k, uint64_t c )
{
  Ntk ntk;

  std::vector<typename Ntk::signal> a( k ), b( k );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_adder_inplace( ntk, a, b, c );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  CHECK( ntk.num_pis() == 2u * k );
  CHECK( ntk.num_pos() == k );

  const auto simm = simulate<bool>( ntk, input_word_simulator( ( op1 << k ) + op2 ) );
  CHECK( simm.size() == k );
  const auto result = ( op1 + op2 ) % ( ( 1 << k ) - c );
  CHECK( to_int( simm ) == result );
}

TEST_CASE( "build an k-bit modular adder with constants", "[modular_arithmetic]" )
{
  for ( uint32_t i = 0u; i < 29u; ++i )
  {
    for ( uint32_t j = 0u; j < 29u; ++j )
    {
      simulate_modular_adder2<aig_network>( i, j, 5, 3 );
      simulate_modular_adder2<mig_network>( i, j, 5, 3 );
    }
  }

  std::default_random_engine gen( 655321 );

  for ( auto i = 0; i < 1000; ++i )
  {
    auto k = std::uniform_int_distribution<uint32_t>( 5, 16 )( gen );
    auto c = std::uniform_int_distribution<uint64_t>( 1, 20 )( gen );
    auto a = std::uniform_int_distribution<uint32_t>( 0, ( 1 << k ) - c - 1 )( gen );
    auto b = std::uniform_int_distribution<uint32_t>( 0, ( 1 << k ) - c - 1 )( gen );

    simulate_modular_adder2<aig_network>( a, b, k, c );
    simulate_modular_adder2<mig_network>( a, b, k, c );
  }
}

template<typename Ntk>
void simulate_modular_subtractor( uint32_t op1, uint32_t op2 )
{
  Ntk ntk;

  std::vector<typename Ntk::signal> a( 8 ), b( 8 );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_subtractor_inplace( ntk, a, b );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  CHECK( ntk.num_pis() == 16 );
  CHECK( ntk.num_pos() == 8 );

  const auto simm = simulate<bool>( ntk, input_word_simulator( ( op1 << 8 ) + op2 ) );
  CHECK( simm.size() == 8 );
  const auto result = ( op2 - op1 ) % ( 1 << 8 );
  for ( auto i = 0; i < 8; ++i )
  {
    CHECK( ( ( result >> i ) & 1 ) == simm[i] );
  }
}

TEST_CASE( "build an 8-bit modular subtractor with different networks", "[modular_arithmetic]" )
{
  simulate_modular_subtractor<aig_network>( 37, 73 );
  simulate_modular_subtractor<aig_network>( 0, 255 );
  simulate_modular_subtractor<aig_network>( 0, 255 );
  simulate_modular_subtractor<aig_network>( 200, 200 );
  simulate_modular_subtractor<aig_network>( 120, 250 );

  simulate_modular_subtractor<mig_network>( 37, 73 );
  simulate_modular_subtractor<mig_network>( 0, 255 );
  simulate_modular_subtractor<mig_network>( 0, 255 );
  simulate_modular_subtractor<mig_network>( 200, 200 );
  simulate_modular_subtractor<mig_network>( 120, 250 );

  simulate_modular_subtractor<klut_network>( 37, 73 );
  simulate_modular_subtractor<klut_network>( 0, 255 );
  simulate_modular_subtractor<klut_network>( 0, 255 );
  simulate_modular_subtractor<klut_network>( 200, 200 );
  simulate_modular_subtractor<klut_network>( 120, 250 );
}

template<typename Ntk>
void simulate_modular_subtractor2( uint32_t op1, uint32_t op2, uint32_t k, uint64_t c )
{
  Ntk ntk;

  std::vector<typename Ntk::signal> a( k ), b( k );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_subtractor_inplace( ntk, a, b, c );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  CHECK( ntk.num_pis() == 2u * k );
  CHECK( ntk.num_pos() == k );

  const auto simm = simulate<bool>( ntk, input_word_simulator( ( op1 << k ) + op2 ) );
  CHECK( simm.size() == k );

  // C++ behaves different in computing mod with negative numbers (a % b) => (b + (b % a)) % b
  const int32_t ma = static_cast<int32_t>( op2 ) - static_cast<int32_t>( op1 );
  const int32_t mb = ( 1 << k ) - c;
  const unsigned long long result = ( mb + ( ma % mb ) ) % mb;
  CHECK( to_int( simm ) == result );
}

TEST_CASE( "build an k-bit modular subtractor with constants", "[modular_arithmetic]" )
{
  for ( uint32_t i = 0u; i < 29u; ++i )
  {
    for ( uint32_t j = 0u; j < 29u; ++j )
    {
      simulate_modular_subtractor2<aig_network>( i, j, 5, 3 );
      simulate_modular_subtractor2<mig_network>( i, j, 5, 3 );
    }
  }

  std::default_random_engine gen( 655321 );

  for ( auto i = 0; i < 1000; ++i )
  {
    auto k = std::uniform_int_distribution<uint32_t>( 5, 16 )( gen );
    auto c = std::uniform_int_distribution<uint64_t>( 1, 20 )( gen );
    auto a = std::uniform_int_distribution<uint32_t>( 0, ( 1 << k ) - c - 1 )( gen );
    auto b = std::uniform_int_distribution<uint32_t>( 0, ( 1 << k ) - c - 1 )( gen );

    simulate_modular_subtractor2<aig_network>( a, b, k, c );
    simulate_modular_subtractor2<mig_network>( a, b, k, c );
  }
}

TEST_CASE( "check Montgomery numbers", "[modular_arithmetic]" )
{
  CHECK( detail::compute_montgomery_parameters<int64_t>( 5 ) == std::pair<int64_t, int64_t>{16, 3} );
  CHECK( detail::compute_montgomery_parameters<int64_t>( 21 ) == std::pair<int64_t, int64_t>{64, 3} );
  CHECK( detail::compute_montgomery_parameters<int64_t>( 43 ) == std::pair<int64_t, int64_t>{128, 125} );
  CHECK( detail::compute_montgomery_parameters<int64_t>( 59 ) == std::pair<int64_t, int64_t>{128, 13} );
}

TEST_CASE( "check Montgomery encoding", "[modular_arithmetic]" )
{
  const int64_t n = 11u;
  const auto nbits = static_cast<int64_t>( std::ceil( std::log2( n ) ) );

  auto [r, np] = detail::compute_montgomery_parameters( n );
  const auto rbits = static_cast<int64_t>( std::log2( r ) );

  CHECK( r == 32 );
  CHECK( np == 29 );

  aig_network ntk;
  std::vector<aig_network::signal> pis;
  for ( auto i = 0; i < nbits; ++i )
  {
    pis.push_back( ntk.create_pi() );
  }
  for ( auto i = 0; i < rbits; ++i )
  {
    pis.push_back( ntk.get_constant( false ) );
  }

  const auto MON = detail::to_montgomery_form( ntk, pis, n, rbits, np );

  for ( auto const& m : MON )
  {
    ntk.create_po( m );
  }

  CHECK( MON.size() == nbits );

  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 0u ) ) ) == 0 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 1u ) ) ) == 10 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 2u ) ) ) == 9 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 3u ) ) ) == 8 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 4u ) ) ) == 7 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 5u ) ) ) == 6 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 6u ) ) ) == 5 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 7u ) ) ) == 4 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 8u ) ) ) == 3 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 9u ) ) ) == 2 );
  CHECK( to_int( simulate<bool>( ntk, input_word_simulator( 10u ) ) ) == 1 );
}

TEST_CASE( "check 8-bit modular multiplication", "[modular_arithmetic]" )
{
  aig_network ntk;

  std::vector<aig_network::signal> a( 8 ), b( 8 );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_multiplication_inplace( ntk, a, b, 13 );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  std::default_random_engine gen( 655321 );
  for ( auto i = 0; i < 1000; ++i )
  {
    auto a = std::uniform_int_distribution<uint32_t>( 0, 243 - 1 )( gen );
    auto b = std::uniform_int_distribution<uint64_t>( 0, 243 - 1 )( gen );

    CHECK( to_int( simulate<bool>( ntk, input_word_simulator( ( a << 8 ) + b ) ) ) % 243 == ( a * b ) % 243 );
  }

  write_bench( ntk, "/tmp/montgomery8_243.bench" );
}

TEST_CASE( "check 16-bit modular multiplication", "[modular_arithmetic]" )
{
  aig_network ntk;

  std::vector<aig_network::signal> a( 16 ), b( 16 );
  std::generate( a.begin(), a.end(), [&ntk]() { return ntk.create_pi(); } );
  std::generate( b.begin(), b.end(), [&ntk]() { return ntk.create_pi(); } );

  modular_multiplication_inplace( ntk, a, b, 37 );

  std::for_each( a.begin(), a.end(), [&]( auto f ) { ntk.create_po( f ); } );

  std::default_random_engine gen( 655321 );
  for ( auto i = 0; i < 1000; ++i )
  {
    auto a = std::uniform_int_distribution<uint32_t>( 0, 65499 - 1 )( gen );
    auto b = std::uniform_int_distribution<uint64_t>( 0, 65499 - 1 )( gen );

    CHECK( to_int( simulate<bool>( ntk, input_word_simulator( ( a << 16 ) + b ) ) ) == ( a * b ) % 65499 );
  }
}
