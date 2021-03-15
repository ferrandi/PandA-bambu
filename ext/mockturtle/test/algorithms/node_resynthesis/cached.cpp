#include <catch.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <mockturtle/algorithms/node_resynthesis/cached.hpp>
#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

TEST_CASE( "Exact XAG for MAJ cached", "[cached]" )
{
#if __GNUC__ == 7
  namespace fs = std::experimental::filesystem::v1;
#else
  namespace fs = std::filesystem;
#endif

  kitty::dynamic_truth_table maj( 3u );
  kitty::create_majority( maj );

  xag_network xag;
  const auto a = xag.create_pi();
  const auto b = xag.create_pi();
  const auto c = xag.create_pi();

  std::vector<xag_network::signal> pis = {a, b, c};

  {
    exact_aig_resynthesis<xag_network> exact_resyn;
    cached_resynthesis<xag_network, decltype( exact_resyn )> resyn( exact_resyn, 6u, "mockturtle-test-cache.db" );

    resyn( xag, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
      xag.create_po( f );
    } );
    resyn( xag, maj, pis.begin(), pis.end(), [&]( auto const& f ) {
      xag.create_po( f );
    } );

    default_simulator<kitty::dynamic_truth_table> sim( 3u );
    CHECK( xag.num_pos() == 2u );
    CHECK( xag.num_gates() == 4u );
    const auto tts = simulate<kitty::dynamic_truth_table>( xag, sim );
    CHECK( tts[0] == maj );
    CHECK( tts[1] == maj );
  }

  CHECK( fs::exists( "mockturtle-test-cache.db" ) );
  CHECK( !fs::exists( "mockturtle-test-cache.db.bak" ) );
  fs::remove( "mockturtle-test-cache.db" );
}
