/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL
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
  \file algorithm.hpp
  \brief STL-like algorithm extensions

  \author Mathias Soeken
*/

#pragma once

#include <iterator>

namespace mockturtle
{

template<class Iterator, class T, class BinaryOperation>
T tree_reduce( Iterator first, Iterator last, T const& init, BinaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( *first, *( first + 1 ) );
  default:
  {
    const auto m = len / 2;
    return op( tree_reduce( first, first + m, init, op ), tree_reduce( first + m, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class T, class TernaryOperation>
T ternary_tree_reduce( Iterator first, Iterator last, T const& init, TernaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( init, *first, *( first + 1 ) );
  case 3u:
    return op( *first, *( first + 1 ), *( first + 2 ) );
  default:
  {
    const auto m1 = len / 3;
    const auto m2 = ( len - m1 ) / 2;
    return op( ternary_tree_reduce( first, first + m1, init, op ),
               ternary_tree_reduce( first + m1, first + m1 + m2, init, op ),
               ternary_tree_reduce( first + m1 + m2, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class UnaryOperation, class T>
Iterator max_element_unary( Iterator first, Iterator last, UnaryOperation&& fn, T const& init )
{
  auto best = last;
  auto max = init;
  for ( ; first != last; ++first )
  {
    if ( const auto v = fn( *first ) > max )
    {
      max = v;
      best = first;
    }
  }
  return best;
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T begin, T end )
{
  struct iterator
  {
    using value_type = T;

    value_type curr_;
    bool operator!=( iterator const& other ) const { return curr_ != other.curr_; }
    iterator& operator++() { ++curr_; return *this; }
    iterator operator++(int) { auto copy = *this; ++(*this); return copy; }
    value_type operator*() const { return curr_; }
  };
  struct iterable_wrapper
  {
    T begin_;
    T end_;
    auto begin() { return iterator{begin_}; }
    auto end() { return iterator{end_}; }
  };
  return iterable_wrapper{begin, end};
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T end )
{
  return range<T>( {}, end );
}

} /* namespace mockturtle */
