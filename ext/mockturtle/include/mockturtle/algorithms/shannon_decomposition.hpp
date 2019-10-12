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
  \file shannon_decomposition.hpp
  \brief Shannon decomposition

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../traits.hpp"

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

namespace mockturtle
{

namespace detail
{

template<class Ntk>
class shannon_decomposition_impl
{
public:
  shannon_decomposition_impl( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children )
      : _ntk( ntk ),
        _func( func ),
        pis( children )
  {
    _cache.insert( {func.construct(), _ntk.get_constant( false )} );

    for ( auto i = 0u; i < pis.size(); ++i )
    {
      auto var = func.construct();
      kitty::create_nth_var( var, i );
      _cache.insert( {func.construct(), pis[i] });
    }
  }

  signal<Ntk> run()
  {
    /* -1 will call the cache */
    return decompose( static_cast<int32_t>( pis.size() ) - 1, _func );
  }

private:
  signal<Ntk> decompose( int32_t var, kitty::dynamic_truth_table const& func )
  {
    /* cache lookup... */
    auto it = _cache.find( func );
    if ( it != _cache.end() )
    {
      return it->second;
    }

    /* ...and for the complement */
    it = _cache.find( ~func );
    if ( it != _cache.end() )
    {
      return _ntk.create_not( it->second );
    }

    /* decompose */
    assert( var >= 0 && var < static_cast<int32_t>( pis.size() ) );
    auto func0 = kitty::cofactor0( func, var );
    auto func1 = kitty::cofactor1( func, var );
    auto f = _ntk.create_ite( pis[var], decompose( var - 1, func1 ), decompose( var - 1, func0 ) );
    _cache.insert( {func, f} );
    return f;
  }

private:
  Ntk& _ntk;
  kitty::dynamic_truth_table _func;
  std::vector<signal<Ntk>> pis;
  std::unordered_map<kitty::dynamic_truth_table, signal<Ntk>, kitty::hash<kitty::dynamic_truth_table>> _cache;
};

} // namespace detail

/*! \brief Shannon decomposition
 *
 * This function applies Shannon decomposition on an input truth table and
 * constructs a network based.  The variable ordering is from the most
 * significant to the least significant bit.
 *
 * **Required network functions:**
 * - `create_not`
 * - `create_ite`
 * - `get_constant`
 */
template<class Ntk>
signal<Ntk> shannon_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  detail::shannon_decomposition_impl<Ntk> impl( ntk, func, children );
  return impl.run();
}

} // namespace mockturtle
