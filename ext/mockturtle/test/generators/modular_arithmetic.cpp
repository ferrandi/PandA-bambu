#include <catch.hpp>

#include <numeric>
#include <random>
#include <vector>

#include <fmt/format.h>

#include <mockturtle/traits.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/modular_arithmetic.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>

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
    CHECK( static_cast<bool>( ( ( result >> i ) & 1 ) ) == simm[i] );
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

  simulate_modular_adder<xag_network>( 37, 73 );
  simulate_modular_adder<xag_network>( 0, 255 );
  simulate_modular_adder<xag_network>( 0, 255 );
  simulate_modular_adder<xag_network>( 200, 200 );
  simulate_modular_adder<xag_network>( 120, 250 );
}

template<typename Ntk, typename ArithFn, typename EvaluateFn, typename... Ops>
void simulate_modular_arithmetic( uint32_t k, uint64_t c, ArithFn&& operation, EvaluateFn&& evaluate, Ops... ops )
{
  Ntk ntk;

  constexpr auto NumOps = sizeof...( Ops );
  static_assert( NumOps > 0, "at least one operand must be passed" );

  std::array<uint32_t, NumOps> aops{ops...};

  using signal_arr_t = std::array<std::vector<typename Ntk::signal>, NumOps>;
  signal_arr_t signals;
  for ( auto i = 0u; i < NumOps; ++i )
  {
    signals[i] = std::vector<typename Ntk::signal>( k );
    std::generate( signals[i].begin(), signals[i].end(), [&ntk]() { return ntk.create_pi(); } );
  }

  std::apply( [&]( auto&&... args ) { operation( ntk, args..., c ); }, signals );

  std::for_each( signals[0].begin(), signals[0].end(), [&]( auto f ) { ntk.create_po( f ); } );

  CHECK( ntk.num_pis() == NumOps * k );
  CHECK( ntk.num_pos() == k );

  const auto assign = std::accumulate( aops.rbegin(), aops.rend(), 0u, [&]( auto a, auto v ) { return ( a << k ) + v; } );
  const auto simm = simulate<bool>( ntk, input_word_simulator( assign ) );
  CHECK( simm.size() == k );
  const auto result = std::apply( [&]( auto&&... args ) { return evaluate( args..., c ); }, aops );
  const auto actual = to_int( simm );
  CHECK( actual == result );
}

template<typename Ntk, typename ArithFn, typename EvaluateFn>
void test_unary_modular_arithmetic( ArithFn&& operation, EvaluateFn&& evaluate, int rounds = 1000 )
{
  std::default_random_engine gen( 655321 );

  for ( auto i = 0; i < rounds; ++i )
  {
    auto k = std::uniform_int_distribution<uint32_t>( 5, 16 )( gen );
    auto c = std::uniform_int_distribution<uint64_t>( 2, ( 1 << k ) - 2 )( gen );
    auto a = std::uniform_int_distribution<uint32_t>( 0, static_cast<uint32_t>( c ) - 1 )( gen );

    simulate_modular_arithmetic<Ntk>( k, c, operation, evaluate, a );
  }
}

template<typename Ntk, typename ArithFn, typename EvaluateFn>
void test_binary_modular_arithmetic( ArithFn&& operation, EvaluateFn&& evaluate, int rounds = 1000 )
{
  std::default_random_engine gen( 655321 );

  for ( auto i = 0; i < rounds; ++i )
  {
    auto k = std::uniform_int_distribution<uint32_t>( 5, 16 )( gen );
    auto c = std::uniform_int_distribution<uint64_t>( 2, ( 1 << k ) - 2 )( gen );
    auto a = std::uniform_int_distribution<uint32_t>( 0, static_cast<uint32_t>( c ) - 1 )( gen );
    auto b = std::uniform_int_distribution<uint32_t>( 0, static_cast<uint32_t>( c ) - 1 )( gen );

    simulate_modular_arithmetic<Ntk>( k, c, operation, evaluate, a, b );
  }
}

TEST_CASE( "build default modular adder", "[modular_arithmetic]" )
{
  test_binary_modular_arithmetic<aig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_adder_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a + b ) % c; } );
  test_binary_modular_arithmetic<mig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_adder_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a + b ) % c; } );
  test_binary_modular_arithmetic<xag_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_adder_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a + b ) % c; } );
}

TEST_CASE( "build Hiasat modular adder", "[modular_arithmetic]" )
{
  test_binary_modular_arithmetic<aig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_adder_hiasat_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a + b ) % c; } );
  test_binary_modular_arithmetic<xag_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_adder_hiasat_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a + b ) % c; } );
}

TEST_CASE( "build default modular subtractor", "[modular_arithmetic]" )
{
  test_binary_modular_arithmetic<aig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_subtractor_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return a >= b ? ( a - b ) : a + ( c - b ); } );
  test_binary_modular_arithmetic<mig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_subtractor_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return a >= b ? ( a - b ) : a + ( c - b ); } );
  test_binary_modular_arithmetic<xag_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_subtractor_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return a >= b ? ( a - b ) : a + ( c - b ); } );
}

TEST_CASE( "build default modular doubling", "[modular_arithmetic]" )
{
  test_unary_modular_arithmetic<aig_network>( []( auto& ntk, auto& a, uint64_t c ) { modular_doubling_inplace( ntk, a, c ); }, []( auto a, auto c ) { return ( a * 2 ) % c; } );
  test_unary_modular_arithmetic<mig_network>( []( auto& ntk, auto& a, uint64_t c ) { modular_doubling_inplace( ntk, a, c ); }, []( auto a, auto c ) { return ( a * 2 ) % c; } );
  test_unary_modular_arithmetic<xag_network>( []( auto& ntk, auto& a, uint64_t c ) { modular_doubling_inplace( ntk, a, c ); }, []( auto a, auto c ) { return ( a * 2 ) % c; } );
}

TEST_CASE( "build default modular halving", "[modular_arithmetic]" )
{
  std::default_random_engine gen( 655321 );

  for ( auto i = 0; i < 1000; ++i )
  {
    auto k = std::uniform_int_distribution<uint32_t>( 5, 16 )( gen );
    auto c = std::uniform_int_distribution<uint64_t>( 2, ( 1 << ( k - 1 ) ) - 2 )( gen ) * 2 + 1;
    auto a = std::uniform_int_distribution<uint32_t>( 0, static_cast<uint32_t>( c ) - 1 )( gen );

    simulate_modular_arithmetic<aig_network>( k, c, []( auto& ntk, auto& a, uint64_t c ) { modular_halving_inplace( ntk, a, c ); }, []( auto a, auto c ) { return a % 2 ? ( a + c ) / 2 : a / 2; }, a );
    simulate_modular_arithmetic<mig_network>( k, c, []( auto& ntk, auto& a, uint64_t c ) { modular_halving_inplace( ntk, a, c ); }, []( auto a, auto c ) { return a % 2 ? ( a + c ) / 2 : a / 2; }, a );
    simulate_modular_arithmetic<xag_network>( k, c, []( auto& ntk, auto& a, uint64_t c ) { modular_halving_inplace( ntk, a, c ); }, []( auto a, auto c ) { return a % 2 ? ( a + c ) / 2 : a / 2; }, a );
  }
}

TEST_CASE( "build default modular multiplier", "[modular_arithmetic]" )
{
  test_binary_modular_arithmetic<aig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_multiplication_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a * b ) % c; }, 100u );
  test_binary_modular_arithmetic<mig_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_multiplication_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a * b ) % c; }, 100u );
  test_binary_modular_arithmetic<xag_network>( []( auto& ntk, auto& a, auto const& b, uint64_t c ) { modular_multiplication_inplace( ntk, a, b, c ); }, []( auto a, auto b, auto c ) { return ( a * b ) % c; }, 100u );
}

TEST_CASE( "create bool vectors from hex strings", "[modular_arithmetic]" )
{

  auto vec_from_hex = []( uint32_t size, std::string const& hex, bool shrink ) {
    std::vector<bool> mod( size );
    bool_vector_from_hex( mod, hex, shrink );
    return mod;
  };

  CHECK( vec_from_hex( 8, "e8", false ) == std::vector<bool>{{false, false, false, true, false, true, true, true}} );
  CHECK( vec_from_hex( 8, "e8", true ) == std::vector<bool>{{false, false, false, true, false, true, true, true}} );

  CHECK( vec_from_hex( 4, "e8", false ) == std::vector<bool>{{false, false, false, true}} );
  CHECK( vec_from_hex( 4, "e8", true ) == std::vector<bool>{{false, false, false, true}} );

  CHECK( vec_from_hex( 4, "e7", false ) == std::vector<bool>{{true, true, true, false}} );
  CHECK( vec_from_hex( 4, "e7", true ) == std::vector<bool>{{true, true, true}} );

  CHECK( vec_from_hex( 3, "0", false ) == std::vector<bool>{{false, false, false}} );
  CHECK( vec_from_hex( 3, "0", true ).size() == 0 );
}

TEST_CASE( "build Montgomery multiplier", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 6u ), ys( 6u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  const auto pos = montgomery_multiplication( xag, xs, ys, 17 );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });

  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 14 << 6 ) + 6 ) ) ) == 13 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 6 << 6 ) + 14 ) ) ) == 13 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 3 << 6 ) + 16 ) ) ) == 5 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 16 << 6 ) + 3 ) ) ) == 5 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 0 << 6 ) + 4 ) ) ) == 0 );
}

TEST_CASE( "build Montgomery multiplier 10-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 10u ), ys( 10u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  const auto pos = montgomery_multiplication( xag, xs, ys, 661 );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });

  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 115 << 10 ) + 643 ) ) ) == 106 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 643 << 10 ) + 115 ) ) ) == 106 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 362 << 10 ) + 278 ) ) ) == 374 );
  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 278 << 10 ) + 362 ) ) ) == 374 );
}

TEST_CASE( "build Montgomery multiplier 30-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 30u ), ys( 30u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  const auto pos = montgomery_multiplication( xag, xs, ys, 1027761563 );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });

  CHECK( to_int( simulate<bool>( xag, input_word_simulator( ( 516764288ull << 30ull ) + 411767756ull ) ) ) == 287117401ull );
}

TEST_CASE( "build Montgomery multiplier 192-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 192u ), ys( 192u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> N( 192u ), NN( 192u );
  bool_vector_from_hex( N, "fffffffffffffffffffffffffffffffeffffffffffffffff", false );
  bool_vector_from_hex( NN, "ffffffffffffffff0000000000000001", false );

  CHECK( N.size() == 192u );
  CHECK( NN.size() == 192u );

  const auto pos = montgomery_multiplication( xag, xs, ys, N, NN );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });
}

TEST_CASE( "build Montgomery multiplier 224-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 224u ), ys( 224u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> N( 224u ), NN( 224u );
  bool_vector_from_hex( N, "ffffffffffffffffffffffffffffffff000000000000000000000001", false );
  bool_vector_from_hex( NN, "1000000000000000000000001000000000000000000000001", false );

  CHECK( N.size() == 224u );
  CHECK( NN.size() == 224u );

  const auto pos = montgomery_multiplication( xag, xs, ys, N, NN );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });
}

TEST_CASE( "build Montgomery multiplier 256-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 256u ), ys( 256u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> N( 256u ), NN( 256u );
  bool_vector_from_hex( N, "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff", false );
  bool_vector_from_hex( NN, "fffffffdfffffffffffffffffffffffeffffffffffffffffffffffff", false );

  CHECK( N.size() == 256u );
  CHECK( NN.size() == 256u );

  const auto pos = montgomery_multiplication( xag, xs, ys, N, NN );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });
}

TEST_CASE( "build Montgomery multiplier 384-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 384u ), ys( 384u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> N( 384u ), NN( 384u );
  bool_vector_from_hex( N, "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff", false );
  bool_vector_from_hex( NN, "14000000140000000c00000002fffffffcfffffffafffffffbfffffffe00000000000000010000000100000001", false );

  CHECK( N.size() == 384u );
  CHECK( NN.size() == 384u );

  const auto pos = montgomery_multiplication( xag, xs, ys, N, NN );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });
}

TEST_CASE( "build Montgomery multiplier 521-bit", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 521u ), ys( 521u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );
  std::generate( ys.begin(), ys.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> N( 521u ), NN( 521u );
  bool_vector_from_hex( N, "1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", false );
  bool_vector_from_hex( NN, "1", false );

  CHECK( N.size() == 521u );
  CHECK( NN.size() == 521u );

  const auto pos = montgomery_multiplication( xag, xs, ys, N, NN );
  std::for_each( pos.begin(), pos.end(), [&]( auto const& f) { xag.create_po( f ); });
}

TEST_CASE( "10-bit constant multiplication by 661", "[modular_arithmetic]" )
{
  xag_network xag;

  std::vector<xag_network::signal> xs( 10u );
  std::generate( xs.begin(), xs.end(), [&]() { return xag.create_pi(); } );

  std::vector<bool> constant = {true, false, true, false, true, false, false, true, false, true};
  const auto sum = modular_constant_multiplier( xag, xs, constant );
  std::for_each( sum.begin(), sum.end(), [&]( auto const& f) { xag.create_po( f ); });

  std::default_random_engine gen;
  std::uniform_int_distribution<int> dist( 0, 1023 );

  for ( auto i = 0u; i < 100u; ++i )
  {
    const auto v = dist( gen );
    CHECK( to_int( simulate<bool>( xag, input_word_simulator( v ) ) ) == ( ( 661 * v ) % 1024 ) );
  }
}
