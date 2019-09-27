#include <catch.hpp>

#include <algorithm>
#include <array>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/generators/majority_n.hpp>
#include <mockturtle/networks/mig.hpp>

using namespace mockturtle;

template<class Ntk, int Count>
inline std::pair<Ntk, std::array<typename Ntk::signal, Count>> init_network()
{
    Ntk ntk;
    std::array<typename Ntk::signal, Count> pis;
    std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
    return {ntk, pis};
}

template<class Ntk>
inline bool implements_majority( Ntk const& ntk )
{
    CHECK( ntk.num_pos() == 1u );
    
    default_simulator<kitty::dynamic_truth_table> sim( ntk.num_pis() );
    kitty::dynamic_truth_table maj( ntk.num_pis() );
    kitty::create_majority( maj );
    return maj == simulate<kitty::dynamic_truth_table>( ntk, sim )[0];
}

TEST_CASE( "build majority-9 using bdd based method", "[majority]")
{
    auto [mig, pis] = init_network<mig_network, 9>();
    mig.create_po( majority_n_bdd( mig, pis ) );
    
    CHECK( 9u == mig.num_pis() );
    CHECK( implements_majority( mig ));
}

TEST_CASE( "build majority-15 using bdd based method", "[majority]")
{
    auto [mig, pis] = init_network<mig_network, 15>();
    mig.create_po( majority_n_bdd( mig, pis ) );

    CHECK( 15u == mig.num_pis() );
    CHECK( implements_majority( mig ));
}

TEST_CASE( "build majority-9 using sorter network based method", "[majority]")
{
    auto [mig, pis] = init_network<mig_network, 9>();
    mig.create_po( majority_n_bubble_sort( mig, pis ) );
    
    CHECK( 9u == mig.num_pis() );
    CHECK( implements_majority( mig ));
}

TEST_CASE( "build majority-15 using sorter network based method", "[majority]")
{
    auto [mig, pis] = init_network<mig_network, 15>();
    mig.create_po( majority_n_bubble_sort( mig, pis ) );
    
    CHECK( 15u == mig.num_pis() );
    CHECK( implements_majority( mig ));
}
