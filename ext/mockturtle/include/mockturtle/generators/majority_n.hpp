/*!
 \file majority_n.hpp
 \brief Generate majority-n networks using BDD and sorter network based methods
 
 \author Dewmini Sudara
 */

#include <array>
#include <vector>

#include "sorting.hpp"


namespace mockturtle
{
    /**! \brief Generates majority-n network on given input signals using the BDD based method
     *
     *  All majority operations are leafy.
     */
    template <typename Ntk, typename SizeT, SizeT N>
    signal<Ntk> majority_n_bdd( Ntk& ntk, std::array<signal<Ntk>, N> const& xs ) {
        const auto logic1 = ntk.get_constant( true );
        const auto logic0 = ntk.get_constant( false );
        std::array<std::vector<signal<Ntk>>, N> dp;
        dp[0].push_back( xs[0] );
        for( auto r = 1u; r <= xs.size() / 2; r++ ){
            dp[r].push_back( ntk.create_maj( logic0, dp[r-1][0], xs[r] ) );
            for( auto c = 1u; c < r; c++ ){
                dp[r].push_back( ntk.create_maj( dp[r-1][c-1], dp[r-1][c], xs[r] ) );
            }
            dp[r].push_back( ntk.create_maj( logic1, dp[r-1][r-1], xs[r] ) );
        }
        for( auto r = xs.size() / 2 + 1; r < xs.size(); r++ ){
            for( auto c = 0u; c < xs.size() - r; c++ ){
                dp[r].push_back(ntk.create_maj( dp[r-1][c], dp[r-1][c+1], xs[r] ) );
            }
        }
        return dp[xs.size() - 1][0];
    }
    
    /**! \brief Generates majority-n network on given input signals using bubble sort.
     *
     * All majority operations require no inverters and are leafy.
     */
    template <typename Ntk, typename SizeT, SizeT N>
    signal<Ntk> majority_n_bubble_sort( Ntk& ntk, std::array<signal<Ntk>, N> const& xs ) {
        std::vector<signal<Ntk>> sigs( xs.begin(), xs.end() );
        bubble_sorting_network( sigs.size(), [&sigs, &ntk]( auto i, auto j ){
            signal<Ntk> lhs = ntk.create_and( sigs[i], sigs[j] );
            signal<Ntk> rhs = ntk.create_or( sigs[i], sigs[j] );
            sigs[i] = lhs;
            sigs[j] = rhs;
        });
        return sigs[sigs.size() / 2];
    }
    
} // namespace mockturtle
