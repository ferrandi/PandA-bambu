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
  \file abc_resub.hpp
  \brief Interface to `abc_resub`.

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "index_list.hpp"

#include <kitty/kitty.hpp>
#include <abcresub/abcresub.hpp>

namespace mockturtle
{

class abc_resub
{
public:
  explicit abc_resub( uint64_t num_divisors, uint64_t num_blocks_per_truth_table, uint64_t max_num_divisors = 50ul )
    : num_divisors( num_divisors )
    , num_blocks_per_truth_table( num_blocks_per_truth_table )
    , max_num_divisors( max_num_divisors )
    , counter(0)
  {
    alloc();
  }

  virtual ~abc_resub()
  {
    release();
  }

  template<class truth_table_type>
  void add_root( truth_table_type const& tt, truth_table_type const& care )
  {
    add_divisor( ~tt & care ); /* off-set */
    add_divisor( tt & care ); /* on-set */
  }

  template<class truth_table_type>
  void add_divisor( truth_table_type const& tt )
  {
    assert( abc_tts != nullptr && "assume that memory for truth tables has been allocated" );
    assert( abc_divs != nullptr && "assume that memory for divisors has been allocated" );

    assert( tt.num_blocks() == num_blocks_per_truth_table );
    for ( uint64_t i = 0ul; i < num_blocks_per_truth_table; ++i )
    {
      Vec_WrdPush( abc_tts, tt._bits[i] );
    }
    Vec_PtrPush( abc_divs, Vec_WrdEntryP( abc_tts, counter * num_blocks_per_truth_table ) );
    ++counter;
  }

  template<class iterator_type, class truth_table_storage_type>
  void add_divisors( iterator_type begin, iterator_type end, truth_table_storage_type const& tts )
  {
    assert( abc_tts != nullptr && "assume that memory for truth tables has been allocated" );
    assert( abc_divs != nullptr && "assume that memory for divisors has been allocated" );

    while ( begin != end )
    {
      add_divisor( tts[*begin] );
      ++begin;
    }
  }

  std::optional<xag_index_list> compute_function( uint32_t num_inserts, bool useXOR = false )
  {
    int * raw_list;
    int size = abcresub::Abc_ResubComputeFunction( (void **)Vec_PtrArray( abc_divs ), Vec_PtrSize( abc_divs ), num_blocks_per_truth_table, num_inserts, /* nDivsMax */max_num_divisors, /* nChoice = */0, int(useXOR), /* debug = */0, /* verbose = */0, &raw_list );

    if ( size )
    {
      xag_index_list xag_list;
      xag_list.add_inputs( num_divisors - 2 );
      for ( int i = 0; i < size - 1; i += 2 )
      {
        if ( raw_list[i] < raw_list[i+1] )
          xag_list.add_and( raw_list[i] - 2, raw_list[i+1] - 2 );
        else
          xag_list.add_xor( raw_list[i] - 2, raw_list[i+1] - 2 );
      }
      xag_list.add_output( raw_list[size - 1] < 2 ? raw_list[size - 1] : raw_list[size - 1] - 2 );
      return xag_list;
    }

    return std::nullopt;
  }

  void dump( std::string const file = "dump.txt" ) const
  {
    abcresub::Abc_ResubDumpProblem( file.c_str(), (void **)Vec_PtrArray( abc_divs ),  Vec_PtrSize( abc_divs ), num_blocks_per_truth_table );
  }

protected:
  void alloc()
  {
    assert( abc_tts == nullptr );
    assert( abc_divs == nullptr );
    abc_tts = abcresub::Vec_WrdAlloc( num_divisors * num_blocks_per_truth_table );
    abc_divs = abcresub::Vec_PtrAlloc( num_divisors );
  }

  void release()
  {
    assert( abc_divs != nullptr );
    assert( abc_tts != nullptr );
    Vec_PtrFree( abc_divs );
    Vec_WrdFree( abc_tts );
    abc_divs = nullptr;
    abc_tts = nullptr;
  }

protected:
  uint64_t num_divisors;
  uint64_t num_blocks_per_truth_table;
  uint64_t max_num_divisors;
  uint64_t counter;

  abcresub::Vec_Wrd_t * abc_tts{nullptr};
  abcresub::Vec_Ptr_t * abc_divs{nullptr};
}; /* abc_resub */

} /* namespace mockturtle */