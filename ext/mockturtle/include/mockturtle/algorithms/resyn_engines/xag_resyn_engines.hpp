/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file xag_resyn_engines.hpp
  \brief Resynthesis by recursive decomposition for AIGs or XAGs.
  (based on ABC's implementation in `giaResub.c` by Alan Mishchenko)

  \author Siang-Yun Lee
*/

#pragma once

#include "../../utils/index_list.hpp"
#include "../../utils/stopwatch.hpp"

#include <kitty/kitty.hpp>
#include <fmt/format.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

namespace mockturtle
{

struct xag_resyn_engine_params
{
  /*! \brief Maximum size (number of gates) of the dependency circuit. */
  uint32_t max_size{0u};

  /*! \brief Whether to consider XOR gates as having the same cost as AND gates (i.e., using XAGs). */
  bool use_xor{false};

  /*! \brief Maximum number of binate divisors to be considered. */
  uint32_t max_binates{50u};
};

struct xag_resyn_engine_stats
{
  /*! \brief Time for finding 0-resub and collecting unate literals. */
  stopwatch<>::duration time_unate{0};

  /*! \brief Time for finding 1-resub. */
  stopwatch<>::duration time_resub1{0};

  /*! \brief Time for finding 2-resub. */
  stopwatch<>::duration time_resub2{0};

  /*! \brief Time for finding 3-resub. */
  stopwatch<>::duration time_resub3{0};

  /*! \brief Time for sorting unate literals and unate pairs. */
  stopwatch<>::duration time_sort{0};

  /*! \brief Time for collecting unate pairs. */
  stopwatch<>::duration time_collect_pairs{0};

  void report() const
  {
    fmt::print( "[i]         0-resub      :{:>5.2f} secs\n", to_seconds( time_unate ) );
    fmt::print( "[i]         1-resub      :{:>5.2f} secs\n", to_seconds( time_resub1 ) );
    fmt::print( "[i]         2-resub      :{:>5.2f} secs\n", to_seconds( time_resub2 ) );
    fmt::print( "[i]         3-resub      :{:>5.2f} secs\n", to_seconds( time_resub3 ) );
    fmt::print( "[i]         sort         :{:>5.2f} secs\n", to_seconds( time_sort ) );
    fmt::print( "[i]         collect pairs:{:>5.2f} secs\n", to_seconds( time_collect_pairs ) );
  }
};

/*! \brief Logic resynthesis engine for AIGs or XAGs.
 *
 * The algorithm is based on ABC's implementation in `giaResub.c` by Alan Mishchenko.
 *
 * Divisors are classified as positive unate (not overlapping with target offset),
 * negative unate (not overlapping with target onset), or binate (overlapping with
 * both onset and offset). Furthermore, pairs of binate divisors are combined with
 * an AND operation and considering all possible input polarities and again classified
 * as positive unate, negative unate or binate. Simple solutions of zero cost 
 * (one unate divisor), one node (two unate divisors), two nodes (one unate divisor + 
 * one unate pair), and three nodes (two unate pairs) are exhaustively examined.
 * When no simple solutions can be found, the algorithm heuristically chooses an unate
 * divisor or an unate pair to divide the target function with and recursively calls
 * itself to decompose the remainder function.
 */
template<class TT>
class xag_resyn_engine
{
public:
  using stats = xag_resyn_engine_stats;
  using params = xag_resyn_engine_params;
  using index_list_t = xag_index_list;
  using truth_table_t = TT;

private:
  struct and_pair
  {
    and_pair( uint32_t l1, uint32_t l2 )
      : lit1( l1 < l2 ? l1 : l2 ), lit2( l1 < l2 ? l2 : l1 )
    { }
    bool operator==( and_pair const& other ) const
    {
      return lit1 == other.lit1 && lit2 == other.lit2;
    }

    uint32_t lit1, lit2;
  };

  struct pair_hash
  {
    std::size_t operator()( and_pair const& p ) const
    {
      return std::hash<uint64_t>{}( (uint64_t)p.lit1 << 32 | (uint64_t)p.lit2 );
    }
  };

public:
  explicit xag_resyn_engine( TT const& target, TT const& care, stats& st, params const& ps = {} )
    : divisors( { ~target & care, target & care } ), st( st ), ps( ps )
  { }

  void add_divisor( TT const& tt )
  {
    assert( tt.num_bits() == divisors[0].num_bits() );
    divisors.emplace_back( tt );
  }

  template<class node_type, class truth_table_storage_type>
  void add_divisor( node_type const& node, truth_table_storage_type const& tts )
  {
    add_divisor( tts[node] );
  }

  template<class iterator_type, class truth_table_storage_type>
  void add_divisors( iterator_type begin, iterator_type end, truth_table_storage_type const& tts )
  { 
    while ( begin != end )
    {
      add_divisor( tts[*begin] );
      ++begin;
    }
  }

  std::optional<index_list_t> operator()()
  {
    return compute_function();
  }

  template<class iterator_type, class truth_table_storage_type>
  std::optional<index_list_t> operator()( iterator_type begin, iterator_type end, truth_table_storage_type const& tts )
  {
    add_divisors( begin, end, tts );
    return compute_function();
  }

private:
  std::optional<index_list_t> compute_function()
  {
    index_list.add_inputs( divisors.size() - 2 );
    auto const lit = compute_function_rec( ps.max_size );
    if ( lit )
    {
      assert( index_list.num_gates() <= ps.max_size );
      index_list.add_output( *lit );
      return index_list;
    }
    return std::nullopt;
  }

  std::optional<uint32_t> compute_function_rec( uint32_t num_inserts )
  {
    /* try 0-resub and collect unate literals */
    auto const res0 = call_with_stopwatch( st.time_unate, [&]() {
      return find_one_unate();
    });
    if ( res0 )
    {
      return *res0;
    }
    if ( num_inserts == 0u )
    {
      return std::nullopt;
    }

    /* sort unate literals and try 1-resub */
    call_with_stopwatch( st.time_sort, [&]() {
      sort_unate_lits( pos_unate_lits, pos_lit_scores, 1 );
      sort_unate_lits( neg_unate_lits, neg_lit_scores, 0 );
    });
    auto const res1or = call_with_stopwatch( st.time_resub1, [&]() {
      return find_div_div( pos_unate_lits, pos_lit_scores, 1 );
    });
    if ( res1or )
    {
      return *res1or;
    }
    auto const res1and = call_with_stopwatch( st.time_resub1, [&]() {
      return find_div_div( neg_unate_lits, neg_lit_scores, 0 );
    });
    if ( res1and )
    {
      return *res1and;
    }

    if ( binate_divs.size() > ps.max_binates )
    {
      binate_divs.resize( ps.max_binates );
    }

    if ( ps.use_xor )
    {
      auto const res1xor = find_xor();
      if ( res1xor )
      {
        return *res1xor;
      }
    }
    if ( num_inserts == 1u )
    {
      return std::nullopt;
    }

    /* collect and sort unate pairs, then try 2- and 3-resub */
    call_with_stopwatch( st.time_collect_pairs, [&]() {
      collect_unate_pairs();
    });
    call_with_stopwatch( st.time_sort, [&]() {
      sort_unate_pairs( pos_unate_pairs, pos_pair_scores, 1 );
      sort_unate_pairs( neg_unate_pairs, neg_pair_scores, 0 );
    });
    auto const res2or = call_with_stopwatch( st.time_resub2, [&]() {
      return find_div_pair( pos_unate_lits, pos_unate_pairs, pos_lit_scores, pos_pair_scores, 1 );
    });
    if ( res2or )
    {
      return *res2or;
    }
    auto const res2and = call_with_stopwatch( st.time_resub2, [&]() {
      return find_div_pair( neg_unate_lits, neg_unate_pairs, neg_lit_scores, neg_pair_scores, 0 );
    });
    if ( res2and )
    {
      return *res2and;
    }

    if ( num_inserts >= 3u )
    {
      auto const res3or = call_with_stopwatch( st.time_resub3, [&]() {
        return find_pair_pair( pos_unate_pairs, pos_pair_scores, 1 );
      });
      if ( res3or )
      {
        return *res3or;
      }
      auto const res3and = call_with_stopwatch( st.time_resub3, [&]() {
        return find_pair_pair( neg_unate_pairs, neg_pair_scores, 0 );
      });
      if ( res3and )
      {
        return *res3and;
      }
    }

    /* choose something to divide and recursive call on the remainder */
    uint32_t on_off_div, on_off_pair;
    uint32_t score_div = 0, score_pair = 0;
    if ( pos_unate_lits.size() > 0 )
    {
      on_off_div = 1; /* use pos_lit */
      score_div = pos_lit_scores[pos_unate_lits[0]];
      if ( neg_unate_lits.size() > 0 && neg_lit_scores[neg_unate_lits[0]] > pos_lit_scores[pos_unate_lits[0]] )
      {
        on_off_div = 0; /* use neg_lit */
        score_div = neg_lit_scores[neg_unate_lits[0]];
      }
    }
    else if ( neg_unate_lits.size() > 0 )
    {
      on_off_div = 0; /* use neg_lit */
      score_div = neg_lit_scores[neg_unate_lits[0]];
    }

    if ( num_inserts >= 3u )
    {
      if ( pos_unate_pairs.size() > 0 )
      {
        on_off_pair = 1; /* use pos_pair */
        score_pair = pos_pair_scores[pos_unate_pairs[0]];
        if ( neg_unate_pairs.size() > 0 && neg_pair_scores[neg_unate_pairs[0]] > pos_pair_scores[pos_unate_pairs[0]] )
        {
          on_off_pair = 0; /* use neg_pair */
          score_pair = neg_pair_scores[neg_unate_pairs[0]];
        }
      }
      else if ( neg_unate_pairs.size() > 0 )
      {
        on_off_pair = 0; /* use neg_pair */
        score_pair = neg_pair_scores[neg_unate_pairs[0]];
      }
    }

    if ( score_div > score_pair / 2 ) /* divide with a divisor */
    {
      /* if using pos_lit (on_off_div = 1), modify on-set and use an OR gate on top;
         if using neg_lit (on_off_div = 0), modify off-set and use an AND gate on top
       */
      uint32_t const lit = on_off_div ? pos_unate_lits[0] : neg_unate_lits[0];
      divisors[on_off_div] &= lit & 0x1 ? divisors[lit >> 1] : ~divisors[lit >> 1];

      auto const res_remain_div = compute_function_rec( num_inserts - 1 );
      if ( res_remain_div )
      {
        auto const new_lit = index_list.add_and( ( lit ^ 0x1 ) - 2, *res_remain_div ^ on_off_div );
        return new_lit + on_off_div;
      }
    }
    else if ( score_pair > 0 ) /* divide with a pair */
    {
      and_pair const pair = on_off_pair ? pos_unate_pairs[0] : neg_unate_pairs[0];
      divisors[on_off_pair] &= ( pair.lit1 & 0x1 ? divisors[pair.lit1 >> 1] : ~divisors[pair.lit1 >> 1] )
                            | ( pair.lit2 & 0x1 ? divisors[pair.lit2 >> 1] : ~divisors[pair.lit2 >> 1] );

      auto const res_remain_pair = compute_function_rec( num_inserts - 2 );
      if ( res_remain_pair )
      {
        auto const new_lit1 = index_list.add_and( pair.lit1 - 2, pair.lit2 - 2 );
        auto const new_lit2 = index_list.add_and( new_lit1 ^ 0x1, *res_remain_pair ^ on_off_pair );
        return new_lit2 + on_off_pair;
      }
    }

    return std::nullopt;
  }

  /* See if there is a constant or divisor covering all on-set bits or all off-set bits.
     1. Check constant-resub
     2. Collect unate literals
     3. Find 0-resub (both positive unate and negative unate) and collect binate (neither pos nor neg unate) divisors
   */
  std::optional<uint32_t> find_one_unate()
  {
    num_bits[0] = kitty::count_ones( divisors[0] ); /* off-set */
    num_bits[1] = kitty::count_ones( divisors[1] ); /* on-set */
    if ( num_bits[0] == 0 )
    {
      return 1;
    }
    if ( num_bits[1] == 0 )
    {
      return 0;
    }

    pos_unate_lits.clear();
    neg_unate_lits.clear();
    binate_divs.clear();
    for ( auto v = 2u; v < divisors.size(); ++v )
    {
      bool unateness[4] = {false, false, false, false};
      /* check intersection with off-set */
      if ( intersection_is_empty( divisors[v], divisors[0] ) )
      {
        pos_unate_lits.emplace_back( v << 1 );
        unateness[0] = true;
      }
      else if ( intersection_is_empty_neg( divisors[v], divisors[0] ) )
      {
        pos_unate_lits.emplace_back( v << 1 | 0x1 );
        unateness[1] = true;
      }

      /* check intersection with on-set */
      if ( intersection_is_empty( divisors[v], divisors[1] ) )
      {
        neg_unate_lits.emplace_back( v << 1 );
        unateness[2] = true;
      }
      else if ( intersection_is_empty_neg( divisors[v], divisors[1] ) )
      {
        neg_unate_lits.emplace_back( v << 1 | 0x1 );
        unateness[3] = true;
      }

      /* 0-resub */
      if ( unateness[0] && unateness[3] )
      {
        return ( v << 1 ) - 2;
      }
      if ( unateness[1] && unateness[2] )
      {
        return ( v << 1 ) - 1;
      }
      /* useless unate literal */
      if ( ( unateness[0] && unateness[2] ) || ( unateness[1] && unateness[3] ) )
      {
        pos_unate_lits.pop_back();
        neg_unate_lits.pop_back();
      }
      /* binate divisor */
      else if ( !unateness[0] && !unateness[1] && !unateness[2] && !unateness[3] )
      {
        binate_divs.emplace_back( v );
      }
    }
    return std::nullopt;
  }

  /* Sort the unate literals by the number of minterms in the intersection.
     - For `pos_unate_lits`, `on_off` = 1, sort by intersection with on-set;
     - For `neg_unate_lits`, `on_off` = 0, sort by intersection with off-set
   */
  void sort_unate_lits( std::vector<uint32_t>& unate_lits, std::unordered_map<uint32_t, uint32_t>& scores, uint32_t on_off )
  {
    scores.clear();
    for ( auto const& lit : unate_lits )
    {
      scores[lit] = kitty::count_ones( ( lit & 0x1 ? ~divisors[lit >> 1] : divisors[lit >> 1] ) & divisors[on_off] );
    }
    std::sort( unate_lits.begin(), unate_lits.end(), [&]( uint32_t lit1, uint32_t lit2 ) {
        return scores[lit1] > scores[lit2]; // descending order
    });
  }
  void sort_unate_pairs( std::vector<and_pair>& unate_pairs, std::unordered_map<and_pair, uint32_t, pair_hash>& scores, uint32_t on_off )
  {
    scores.clear();
    for ( auto const& p : unate_pairs )
    {
      scores[p] = kitty::count_ones( ( p.lit1 & 0x1 ? ~divisors[p.lit1 >> 1] : divisors[p.lit1 >> 1] )
                                   & ( p.lit2 & 0x1 ? ~divisors[p.lit2 >> 1] : divisors[p.lit2 >> 1] )
                                   & divisors[on_off] );
    }
    std::sort( unate_pairs.begin(), unate_pairs.end(), [&]( and_pair const& p1, and_pair const& p2 ) {
        return scores[p1] > scores[p2]; // descending order
    });
  }

  /* See if there are two unate divisors covering all on-set bits or all off-set bits.
     - For `pos_unate_lits`, `on_off` = 1, try covering all on-set bits by combining two with an OR gate;
     - For `neg_unate_lits`, `on_off` = 0, try covering all off-set bits by combining two with an AND gate
   */
  std::optional<uint32_t> find_div_div( std::vector<uint32_t>& unate_lits, std::unordered_map<uint32_t, uint32_t>& scores, uint32_t on_off )
  {
    for ( auto i = 0u; i < unate_lits.size(); ++i )
    {
      uint32_t const& lit1 = unate_lits[i];
      if ( scores[lit1] * 2 < num_bits[on_off] )
      {
        break;
      }
      for ( auto j = i + 1; j < unate_lits.size(); ++j )
      {
        uint32_t const& lit2 = unate_lits[j];
        if ( scores[lit1] + scores[lit2] < num_bits[on_off] )
        {
          break;
        }
        auto const ntt1 = lit1 & 0x1 ? divisors[lit1 >> 1] : ~divisors[lit1 >> 1];
        auto const ntt2 = lit2 & 0x1 ? divisors[lit2 >> 1] : ~divisors[lit2 >> 1];
        if ( intersection_is_empty( ntt1, ntt2, divisors[on_off] ) )
        {
          auto const new_lit = index_list.add_and( ( lit1 ^ 0x1 ) - 2, ( lit2 ^ 0x1 ) - 2 );
          return new_lit + on_off;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<uint32_t> find_div_pair( std::vector<uint32_t>& unate_lits, std::vector<and_pair>& unate_pairs, std::unordered_map<uint32_t, uint32_t>& lit_scores, std::unordered_map<and_pair, uint32_t, pair_hash>& pair_scores, uint32_t on_off )
  {
    for ( auto i = 0u; i < unate_lits.size(); ++i )
    {
      uint32_t const& lit1 = unate_lits[i];
      for ( auto j = 0u; j < unate_pairs.size(); ++j )
      {
        and_pair const& pair2 = unate_pairs[j];
        if ( lit_scores[lit1] + pair_scores[pair2] < num_bits[on_off] )
        {
          break;
        }
        auto const ntt1 = lit1 & 0x1 ? divisors[lit1 >> 1] : ~divisors[lit1 >> 1];
        auto const ntt2 = ( pair2.lit1 & 0x1 ? divisors[pair2.lit1 >> 1] : ~divisors[pair2.lit1 >> 1] )
                        | ( pair2.lit2 & 0x1 ? divisors[pair2.lit2 >> 1] : ~divisors[pair2.lit2 >> 1] );
        if ( intersection_is_empty( ntt1, ntt2, divisors[on_off] ) )
        {
          auto const new_lit1 = index_list.add_and( pair2.lit1 - 2, pair2.lit2 - 2 );
          auto const new_lit2 = index_list.add_and( ( lit1 ^ 0x1 ) - 2, new_lit1 ^ 0x1 );
          return new_lit2 + on_off;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<uint32_t> find_pair_pair( std::vector<and_pair>& unate_pairs, std::unordered_map<and_pair, uint32_t, pair_hash>& scores, uint32_t on_off )
  {
    for ( auto i = 0u; i < unate_pairs.size(); ++i )
    {
      and_pair const& pair1 = unate_pairs[i];
      if ( scores[pair1] * 2 < num_bits[on_off] )
      {
        break;
      }
      for ( auto j = i + 1; j < unate_pairs.size(); ++j )
      {
        and_pair const& pair2 = unate_pairs[j];
        if ( scores[pair1] + scores[pair2] < num_bits[on_off] )
        {
          break;
        }
        auto const ntt1 = ( pair1.lit1 & 0x1 ? divisors[pair1.lit1 >> 1] : ~divisors[pair1.lit1 >> 1] )
                        | ( pair1.lit2 & 0x1 ? divisors[pair1.lit2 >> 1] : ~divisors[pair1.lit2 >> 1] );
        auto const ntt2 = ( pair2.lit1 & 0x1 ? divisors[pair2.lit1 >> 1] : ~divisors[pair2.lit1 >> 1] )
                        | ( pair2.lit2 & 0x1 ? divisors[pair2.lit2 >> 1] : ~divisors[pair2.lit2 >> 1] );
        if ( intersection_is_empty( ntt1, ntt2, divisors[on_off] ) )
        {
          uint32_t const fanin_lit1 = index_list.add_and( pair1.lit1 - 2, pair1.lit2 - 2 );
          uint32_t const fanin_lit2 = index_list.add_and( pair2.lit1 - 2, pair2.lit2 - 2 );
          uint32_t const output_lit = index_list.add_and( fanin_lit1 ^ 0x1, fanin_lit2 ^ 0x1 );
          return output_lit + on_off;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<uint32_t> find_xor()
  {
    /* collect xor_pairs (d1 ^ d2) & off = 0 and ~(d1 ^ d2) & on = 0, selecting d1, d2 from binate_divs */
    return std::nullopt;
  }

  /* collect and_pairs (d1 & d2) & off = 0 and ~(d1 & d2) & on = 0, selecting d1, d2 from binate_divs */
  void collect_unate_pairs()
  {
    for ( auto i = 0u; i < binate_divs.size(); ++i )
    {
      for ( auto j = i + 1; j < binate_divs.size(); ++j )
      {
        collect_unate_pairs_detail( binate_divs[i], 0, binate_divs[j], 0 );
        collect_unate_pairs_detail( binate_divs[i], 0, binate_divs[j], 1 );
        collect_unate_pairs_detail( binate_divs[i], 1, binate_divs[j], 0 );
        collect_unate_pairs_detail( binate_divs[i], 1, binate_divs[j], 1 );
      }
    }
  }

  void collect_unate_pairs_detail( uint32_t div1, uint32_t neg1, uint32_t div2, uint32_t neg2 )
  {
    auto const tt1 = neg1 ? ~divisors[div1] : divisors[div1];
    auto const tt2 = neg2 ? ~divisors[div2] : divisors[div2];

    /* check intersection with off-set; additionally check intersection with on-set is not empty (otherwise it's useless) */
    if ( intersection_is_empty( tt1, tt2, divisors[0] ) && !intersection_is_empty( tt1, tt2, divisors[1] ) )
    {
      pos_unate_pairs.emplace_back( ( div1 << 1 ) + neg1, ( div2 << 1 ) + neg2 );
    }
    /* check intersection with on-set; additionally check intersection with off-set is not empty (otherwise it's useless) */
    else if ( intersection_is_empty( tt1, tt2, divisors[1] ) && !intersection_is_empty( tt1, tt2, divisors[0] ) )
    {
      neg_unate_pairs.emplace_back( ( div1 << 1 ) + neg1, ( div2 << 1 ) + neg2 );
    }
  }

private:
  /* equivalent to kitty::is_const0( tt1 & tt2 ), but faster when num_blocks is a lot */
  bool intersection_is_empty( TT const& tt1, TT const& tt2 )
  {
    for ( auto i = 0u; i < tt1.num_blocks(); ++i )
    {
      if ( ( tt1._bits[i] & tt2._bits[i] ) != 0 )
      {
        return false;
      }
    }
    return true;
  }
  /* equivalent to kitty::is_const0( ~tt1 & tt2 ), but faster when num_blocks is a lot */
  bool intersection_is_empty_neg( TT const& tt1, TT const& tt2 )
  {
    for ( auto i = 0u; i < tt1.num_blocks(); ++i )
    {
      if ( ( ~(tt1._bits[i]) & tt2._bits[i] ) != 0 )
      {
        return false;
      }
    }
    return true;
  }
  /* equivalent to kitty::is_const0( tt1 & tt2 & tt3 ), but faster when num_blocks is a lot */
  bool intersection_is_empty( TT const& tt1, TT const& tt2, TT const& tt3 )
  {
    for ( auto i = 0u; i < tt1.num_blocks(); ++i )
    {
      if ( ( tt1._bits[i] & tt2._bits[i] & tt3._bits[i] ) != 0 )
      {
        return false;
      }
    }
    return true;
  }

private:
  std::vector<TT> divisors;
  index_list_t index_list;

  uint32_t num_bits[2]; /* number of bits in on-set and off-set */

  /* positive unate: not overlapping with off-set
     negative unate: not overlapping with on-set */
  std::vector<uint32_t> pos_unate_lits, neg_unate_lits, binate_divs;
  std::unordered_map<uint32_t, uint32_t> pos_lit_scores, neg_lit_scores;
  std::vector<and_pair> pos_unate_pairs, neg_unate_pairs;
  std::unordered_map<and_pair, uint32_t, pair_hash> pos_pair_scores, neg_pair_scores;

  stats& st;
  params const& ps;
}; /* xag_resyn_engine */

} /* namespace mockturtle */
