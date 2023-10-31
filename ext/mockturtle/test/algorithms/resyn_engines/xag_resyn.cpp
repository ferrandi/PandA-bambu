#include <catch.hpp>

#include <kitty/kitty.hpp>

#include <mockturtle/algorithms/resyn_engines/xag_resyn.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/index_list.hpp>

using namespace mockturtle;

template<class TT>
struct aig_resyn_sparams_copy : public xag_resyn_static_params_default<TT>
{
  static constexpr bool use_xor = false;
  static constexpr bool copy_tts = true;
};

template<class TT>
struct aig_resyn_sparams_no_copy : public xag_resyn_static_params_default<TT>
{
  static constexpr bool use_xor = false;
  static constexpr bool copy_tts = false;
};

template<class TT = kitty::partial_truth_table>
void test_aig_kresub( TT const& target, TT const& care, std::vector<TT> const& tts, uint32_t num_inserts )
{
  xag_resyn_stats st;
  std::vector<uint32_t> divs;
  for ( auto i = 0u; i < tts.size(); ++i )
  {
    divs.emplace_back( i );
  }
  partial_simulator sim( tts );

  xag_resyn_decompose<TT, aig_resyn_sparams_copy<TT>> engine_copy( st );
  const auto res = engine_copy( target, care, divs.begin(), divs.end(), tts, num_inserts );
  CHECK( res );
  CHECK( ( *res ).num_gates() == num_inserts );
  aig_network aig;
  decode( aig, *res );
  const auto ans = simulate<TT, aig_network, partial_simulator>( aig, sim )[0];
  CHECK( kitty::implies( target & care, ans ) );
  CHECK( kitty::implies( ~target & care, ~ans ) );

  xag_resyn_decompose<TT, aig_resyn_sparams_no_copy<TT>> engine_no_copy( st );
  const auto res2 = engine_no_copy( target, care, divs.begin(), divs.end(), tts, num_inserts );
  CHECK( res2 );
  CHECK( ( *res2 ).num_gates() == num_inserts );
  aig_network aig2;
  decode( aig2, *res2 );
  const auto ans2 = simulate<TT, aig_network, partial_simulator>( aig2, sim )[0];
  CHECK( kitty::implies( target & care, ans2 ) );
  CHECK( kitty::implies( ~target & care, ~ans2 ) );
}

TEST_CASE( "AIG/XAG resynthesis -- 0-resub with don't care", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 1, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );
  kitty::partial_truth_table care( 8 );

  /* const */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string( care, "11001100" );
  test_aig_kresub( target, care, tts, 0 );

  /* buffer */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string( care, "00111100" );
  kitty::create_from_binary_string( tts[0], "11110000" );
  test_aig_kresub( target, care, tts, 0 );

  /* inverter */
  kitty::create_from_binary_string( target, "00110011" );
  kitty::create_from_binary_string( care, "00110110" );
  kitty::create_from_binary_string( tts[0], "00000101" );
  test_aig_kresub( target, care, tts, 0 );
}

TEST_CASE( "AIG resynthesis -- 1 <= k <= 3", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 4, kitty::partial_truth_table( 8 ) );
  kitty::partial_truth_table target( 8 );
  kitty::partial_truth_table care = ~target.construct();

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "11000000" );
  kitty::create_from_binary_string( tts[1], "00110000" );
  kitty::create_from_binary_string( tts[2], "01011111" ); // binate
  test_aig_kresub( target, care, tts, 1 );                // 1 | 2

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "11001100" ); // binate
  kitty::create_from_binary_string( tts[1], "11111100" );
  kitty::create_from_binary_string( tts[2], "00001100" );
  test_aig_kresub( target, care, tts, 1 ); // 2 & ~3

  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "01110010" ); // binate
  kitty::create_from_binary_string( tts[1], "11111100" );
  kitty::create_from_binary_string( tts[2], "10000011" ); // binate
  test_aig_kresub( target, care, tts, 2 );                // 2 & (1 | 3)

  tts.emplace_back( 8 );
  kitty::create_from_binary_string( target, "11110000" ); // target
  kitty::create_from_binary_string( tts[0], "01110010" ); // binate
  kitty::create_from_binary_string( tts[1], "00110011" ); // binate
  kitty::create_from_binary_string( tts[2], "10000011" ); // binate
  kitty::create_from_binary_string( tts[3], "11001011" ); // binate
  test_aig_kresub( target, care, tts, 3 );                // ~(2 & 4) & (1 | 3)
}

TEST_CASE( "AIG resynthesis -- recursive", "[xag_resyn]" )
{
  std::vector<kitty::partial_truth_table> tts( 6, kitty::partial_truth_table( 16 ) );
  kitty::partial_truth_table target( 16 );
  kitty::partial_truth_table care = ~target.construct();

  kitty::create_from_binary_string( target, "1111000011111111" ); // target
  kitty::create_from_binary_string( tts[0], "0111001000000000" ); // binate
  kitty::create_from_binary_string( tts[1], "0011001100000000" ); // binate
  kitty::create_from_binary_string( tts[2], "1000001100000000" ); // binate
  kitty::create_from_binary_string( tts[3], "1100101100000000" ); // binate
  kitty::create_from_binary_string( tts[4], "0000000011111111" ); // unate
  test_aig_kresub( target, care, tts, 4 );                        // 5 | ( ~(2 & 4) & (1 | 3) )

  tts.emplace_back( 16 );
  kitty::create_from_binary_string( target, "1111000011111100" ); // target
  kitty::create_from_binary_string( tts[0], "0111001000000000" ); // binate
  kitty::create_from_binary_string( tts[1], "0011001100000000" ); // binate
  kitty::create_from_binary_string( tts[2], "1000001100000000" ); // binate
  kitty::create_from_binary_string( tts[3], "1100101100000000" ); // binate
  kitty::create_from_binary_string( tts[4], "0000000011111110" ); // binate
  kitty::create_from_binary_string( tts[5], "0000000011111101" ); // binate
  test_aig_kresub( target, care, tts, 5 );                        // (5 & 6) | ( ~(2 & 4) & (1 | 3) )
}

template<uint32_t num_vars>
class simulator
{
public:
  explicit simulator( std::vector<kitty::static_truth_table<num_vars>> const& input_values )
      : input_values_( input_values )
  {}

  kitty::static_truth_table<num_vars> compute_constant( bool value ) const
  {
    kitty::static_truth_table<num_vars> tt;
    return value ? ~tt : tt;
  }

  kitty::static_truth_table<num_vars> compute_pi( uint32_t index ) const
  {
    assert( index < input_values_.size() );
    return input_values_[index];
  }

  kitty::static_truth_table<num_vars> compute_not( kitty::static_truth_table<num_vars> const& value ) const
  {
    return ~value;
  }

private:
  std::vector<kitty::static_truth_table<num_vars>> const& input_values_;
}; /* simulator */

template<typename engine_type, uint32_t num_vars>
void test_xag_n_input_functions( uint32_t& success_counter, uint32_t& failed_counter )
{
  using truth_table_type = typename engine_type::truth_table_t;
  typename engine_type::stats st;
  std::vector<truth_table_type> divisor_functions;
  std::vector<uint32_t> divisors;
  truth_table_type x;
  for ( uint32_t i = 0; i < num_vars; ++i )
  {
    kitty::create_nth_var( x, i );
    divisor_functions.emplace_back( x );
    divisors.emplace_back( i );
  }

  truth_table_type target, care;
  care = ~target.construct();

  do
  {
    engine_type engine( st );

    auto const index_list = engine( target, care, divisors.begin(), divisors.end(), divisor_functions, std::numeric_limits<uint32_t>::max() );
    if ( index_list )
    {
      ++success_counter;

      /* verify index_list using simulation */
      aig_network aig;
      decode( aig, *index_list );

      simulator<num_vars> sim( divisor_functions );
      auto const tts = simulate<truth_table_type, aig_network>( aig, sim );
      CHECK( target == tts[0] );
    }
    else
    {
      ++failed_counter;
    }

    kitty::next_inplace( target );
  } while ( !kitty::is_const0( target ) );
}

TEST_CASE( "Synthesize XAGs for all 3-input functions", "[xag_resyn]" )
{
  using truth_table_type = kitty::static_truth_table<3>;
  using engine_t = xag_resyn_decompose<truth_table_type>;
  uint32_t success_counter{ 0 };
  uint32_t failed_counter{ 0 };
  test_xag_n_input_functions<engine_t, 3>( success_counter, failed_counter );

  CHECK( success_counter == 254 );
  CHECK( failed_counter == 2 );

  using engine_abc_t = xag_resyn_abc<truth_table_type>;
  success_counter = 0;
  failed_counter = 0;
  test_xag_n_input_functions<engine_abc_t, 3>( success_counter, failed_counter );

  CHECK( success_counter == 254 );
  CHECK( failed_counter == 2 );
}

TEST_CASE( "Synthesize XAGs for all 4-input functions", "[xag_resyn]" )
{
  using truth_table_type = kitty::static_truth_table<4>;
  using engine_t = xag_resyn_decompose<truth_table_type>;
  uint32_t success_counter{ 0 };
  uint32_t failed_counter{ 0 };
  test_xag_n_input_functions<engine_t, 4>( success_counter, failed_counter );

  CHECK( success_counter == 54622 );
  CHECK( failed_counter == 10914 );
}
