#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <kitty/kitty.hpp>
#include <unordered_set>
#pragma GCC diagnostic pop

namespace percy
{
    template<int nr_in>
    static inline std::unordered_set<kitty::static_truth_table<nr_in>, kitty::hash<kitty::static_truth_table<nr_in>>> generate_npn_classes()
    {
        using truth_table = kitty::static_truth_table<nr_in>;
        
        kitty::dynamic_truth_table map(truth_table::NumBits);
        std::transform( map.cbegin(), map.cend(), map.begin(), []( auto word ) { return ~word; } );
        std::unordered_set<truth_table, kitty::hash<truth_table>> classes;

        int64_t index = 0;
        truth_table tt;
        while (index != -1) {
            kitty::create_from_words( tt, &index, &index + 1 );
            const auto res = kitty::exact_npn_canonization( tt, [&map]( const auto& tt ) { kitty::clear_bit( map, *tt.cbegin() ); } );
            classes.insert( std::get<0>( res ) );
            index = find_first_one_bit( map );
        }

        return classes;
    }

}
