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
  \file mig.hpp
  \brief MIG logic network implementation

  \author Eleonora Testa
*/

#pragma once

#include <memory>
#include <optional>
#include <stack>
#include <string>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>

#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "detail/foreach.hpp"
#include "events.hpp"
#include "storage.hpp"

namespace mockturtle
{

struct mig_storage_data
{
  uint32_t num_pis = 0u;
  uint32_t num_pos = 0u;
  std::vector<int8_t> latches;
  uint32_t trav_id = 0u;
};

/*! \brief MIG storage container

  MIGs have nodes with fan-in 3.  We split of one bit of the index pointer to
  store a complemented attribute.  Every node has 64-bit of additional data
  used for the following purposes:

  `data[0].h1`: Fan-out size (we use MSB to indicate whether a node is dead)
  `data[0].h2`: Application-specific value
  `data[1].h1`: Visited flag
*/

using mig_node = regular_node<3, 2, 1>;
using mig_storage = storage<mig_node,
                            mig_storage_data>;

class mig_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 3u;
  static constexpr auto max_fanin_size = 3u;

  using base_type = mig_network;
  using storage = std::shared_ptr<mig_storage>;
  using node = uint64_t;

  struct signal
  {
    signal() = default;

    signal( uint64_t index, uint64_t complement )
        : complement( complement ), index( index )
    {
    }

    explicit signal( uint64_t data )
        : data( data )
    {
    }

    signal( mig_storage::node_type::pointer_type const& p )
        : complement( p.weight ), index( p.index )
    {
    }

    union {
      struct
      {
        uint64_t complement : 1;
        uint64_t index : 63;
      };
      uint64_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return {index, 0};
    }

    signal operator-() const
    {
      return {index, 1};
    }

    signal operator^( bool complement ) const
    {
      return signal( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( signal const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( signal const& other ) const
    {
      return data != other.data;
    }

    bool operator<( signal const& other ) const
    {
      return data < other.data;
    }

    operator mig_storage::node_type::pointer_type() const
    {
      return {index, complement};
    }
  };

  mig_network()
      : _storage( std::make_shared<mig_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  mig_network( std::shared_ptr<mig_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return {0, static_cast<uint64_t>( value ? 1 : 0 )};
  }

  signal create_pi( std::string const& name = std::string() )
  {
    (void)name;

    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = node.children[2].data = ~static_cast<uint64_t>( 0 );
    _storage->inputs.emplace_back( index );
    ++_storage->data.num_pis;
    return {index, 0};
  }

  uint32_t create_po( signal const& f, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    ++_storage->data.num_pos;
    return po_index;
  }

  signal create_ro( std::string const& name = std::string() )
  {
    (void)name;

    auto const index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = node.children[2].data = _storage->inputs.size();
    _storage->inputs.emplace_back( index );
    return {index, 0};
  }

  uint32_t create_ri( signal const& f, int8_t reset = 0, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const ri_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    _storage->data.latches.emplace_back( reset );
    return ri_index;
  }

  int8_t latch_reset( uint32_t index ) const
  {
    assert( index < _storage->data.latches.size() );
    return _storage->data.latches[index];
  }

  bool is_combinational() const
  {
    return ( static_cast<uint32_t>( _storage->inputs.size() ) == _storage->data.num_pis &&
             static_cast<uint32_t>( _storage->outputs.size() ) == _storage->data.num_pos );
  }

  bool is_constant( node const& n ) const
  {
    return n == 0;
  }

  bool is_ci( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data;
  }

  bool is_pi( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[1].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[2].data == ~static_cast<uint64_t>( 0 );
  }

  bool is_ro( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data && _storage->nodes[n].children[0].data >= static_cast<uint64_t>( _storage->data.num_pis );
  }

  bool constant_value( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return a;
  }

  signal create_not( signal const& a )
  {
    return !a;
  }
#pragma endregion

#pragma region Create binary / ternary functions
  signal create_maj( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }
    else
    {
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }

    /*  complemented edges minimization */
    auto node_complement = false;
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) +
             static_cast<unsigned>( c.complement ) >=
         2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return {it->second, node_complement};
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      _storage->hash.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      fn( index );
    }

    return {index, node_complement};
  }

  signal create_and( signal const& a, signal const& b )
  {
    return create_maj( get_constant( false ), a, b );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return create_maj( get_constant( true ), a, b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return !create_or( a, b );
  }

  signal create_lt( signal const& a, signal const& b )
  {
    return create_and( !a, b );
  }

  signal create_le( signal const& a, signal const& b )
  {
    return !create_and( a, !b );
  }

  signal create_xor( signal const& a, signal const& b )
  {
    const auto fcompl = a.complement ^ b.complement;
    const auto c1 = create_and( +a, -b );
    const auto c2 = create_and( +b, -a );
    return create_and( !c1, !c2 ) ^ !fcompl;
  }

  signal create_ite( signal cond, signal f_then, signal f_else )
  {
    bool f_compl{false};
    if ( f_then.index < f_else.index )
    {
      std::swap( f_then, f_else );
      cond.complement ^= 1;
    }
    if ( f_then.complement )
    {
      f_then.complement = 0;
      f_else.complement ^= 1;
      f_compl = true;
    }

    return create_and( !create_and( !cond, f_else ), !create_and( cond, f_then ) ) ^ !f_compl;
  }

  signal create_xor3( signal const& a, signal const& b, signal const& c )
  {
    const auto f = create_maj( a, !b, c );
    const auto g = create_maj( a, b, !c );
    return create_maj( !a, f, g );
  }
#pragma endregion

#pragma region Create nary functions
  signal create_nary_and( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( true ), [this]( auto const& a, auto const& b ) { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_xor( a, b ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( mig_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( children.size() == 3u );
    return create_maj( children[0u], children[1u], children[2u] );
  }
#pragma endregion

#pragma region Restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return std::nullopt;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];

    if ( child0.index > child1.index )
    {
      std::swap( child0, child1 );
    }
    if ( child1.index > child2.index )
    {
      std::swap( child1, child2 );
    }
    if ( child0.index > child1.index )
    {
      std::swap( child0, child1 );
    }

    assert( child0.index <= child1.index );
    assert( child1.index <= child2.index );

    // check for trivial cases?
    if ( child0.index == child1.index )
    {
      const auto diff_pol = child0.complement != child1.complement;
      return std::make_pair( n, diff_pol ? child2 : child0 );
    }
    else if ( child1.index == child2.index )
    {
      const auto diff_pol = child1.complement != child2.complement;
      return std::make_pair( n, diff_pol ? child0 : child1 );
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    _hash_obj.children[2] = child2;
    if ( const auto it = _storage->hash.find( _hash_obj ); it != _storage->hash.end() )
    {
      return std::make_pair( n, signal( it->second, 0 ) );
    }

    // remember before
    const auto old_child0 = signal{node.children[0]};
    const auto old_child1 = signal{node.children[1]};
    const auto old_child2 = signal{node.children[2]};

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    _storage->hash[node] = n;

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      fn( n, {old_child0, old_child1, old_child2} );
    }

    return std::nullopt;
  }

  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        // increment fan-in of new node
        _storage->nodes[new_signal.index].data[0].h1++;
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs or constants */
    if ( n == 0 || is_ci( n ) )
      return;

    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */
    _storage->hash.erase( nobj );

    for ( auto const& fn : _events->on_delete )
    {
      fn( n );
    }

    for ( auto i = 0u; i < 3u; ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  inline bool is_dead( node const& n ) const
  {
    return ( _storage->nodes[n].data[0].h1 >> 31 ) & 1;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( {old_node, new_signal} );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      take_out_node( _old );
    }
  }

  void substitute_node_of_parents( std::vector<node> const& parents, node const& old_node, signal const& new_signal )
  {
    for ( auto& p : parents )
    {
      auto& n = _storage->nodes[p];
      for ( auto& child : n.children )
      {
        if ( child.index == old_node )
        {
          child.index = new_signal.index;
          child.weight ^= new_signal.complement;

          // increment fan-in of new node
          _storage->nodes[new_signal.index].data[0].h1++;

          // decrement fan-in of old node
          _storage->nodes[old_node].data[0].h1--;
        }
      }
    }

    /* check outputs */
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        // increment fan-in of new node
        _storage->nodes[new_signal.index].data[0].h1++;

        // decrement fan-in of old node
        _storage->nodes[old_node].data[0].h1--;
      }
    }
  }
#pragma endregion

#pragma region Structural properties
  auto size() const
  {
    return static_cast<uint32_t>( _storage->nodes.size() );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_latches() const
  {
      return static_cast<uint32_t>( _storage->data.latches.size() );
  }

  auto num_pis() const
  {
    return _storage->data.num_pis;
  }

  auto num_pos() const
  {
    return _storage->data.num_pos;
  }

  auto num_registers() const
  {
    assert( static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis ) == static_cast<uint32_t>( _storage->outputs.size() - _storage->data.num_pos ) );
    return static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis );
  }

  auto num_gates() const
  {
    return static_cast<uint32_t>( _storage->hash.size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    return 3;
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1++ & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  bool is_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_maj( node const& n ) const
  {
    return n > 0 && !is_ci( n );
  }

  bool is_ite( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor3( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_xor( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    (void)n;
    kitty::dynamic_truth_table _maj( 3 );
    _maj._bits[0] = 0xe8;
    return _maj;
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return signal( n, 0 );
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pis );
    return *( _storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pos );
    return *( _storage->outputs.begin() + index );
  }

  node ro_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() - _storage->data.num_pis );
    return *( _storage->inputs.begin() + _storage->data.num_pis + index );
  }

  signal ri_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() - _storage->data.num_pos );
    return *( _storage->outputs.begin() + _storage->data.num_pos + index );
  }

  uint32_t ci_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && 
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data);
    assert( _storage->nodes[n].children[0].data < _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t po_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_po( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }

  uint32_t ro_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && 
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    assert( _storage->nodes[n].children[0].data >= _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data - _storage->data.num_pis );
  }

  uint32_t ri_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_ri( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }

  signal ro_to_ri( signal const& s ) const
  {
    return *( _storage->outputs.begin() + _storage->data.num_pos + _storage->nodes[s.index].children[0].data - _storage->data.num_pis );
  }

  node ri_to_ro( signal const& s ) const
  {
    return *( _storage->inputs.begin() + _storage->data.num_pis + ri_index( s ) );
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    auto r = range<uint64_t>( _storage->nodes.size() );
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.begin() + _storage->data.num_pis, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.begin() + _storage->data.num_pos, fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin() + _storage->data.num_pis, _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin() + _storage->data.num_pos, _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_register( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> );

    assert( _storage->inputs.size() - _storage->data.num_pis == _storage->outputs.size() - _storage->data.num_pos );
    auto ro = _storage->inputs.begin() + _storage->data.num_pis;
    auto ri = _storage->outputs.begin() + _storage->data.num_pos;
    if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> )
    {
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ) ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> )
    {
      uint32_t index{0};
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ), index++ ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> )
    {
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ) );
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> )
    {
      uint32_t index{0};
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ), index++ );
      }
    }
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); // start from 1 to avoid constant
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    // we don't use foreach_element here to have better performance
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{_storage->nodes[n].children[0]} ) )
        return;
      if ( !fn( signal{_storage->nodes[n].children[1]} ) )
        return;
      fn( signal{_storage->nodes[n].children[2]} );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{_storage->nodes[n].children[0]}, 0 ) )
        return;
      if ( !fn( signal{_storage->nodes[n].children[1]}, 1 ) )
        return;
      fn( signal{_storage->nodes[n].children[2]}, 2 );
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn( signal{_storage->nodes[n].children[0]} );
      fn( signal{_storage->nodes[n].children[1]} );
      fn( signal{_storage->nodes[n].children[2]} );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn( signal{_storage->nodes[n].children[0]}, 0 );
      fn( signal{_storage->nodes[n].children[1]}, 1 );
      fn( signal{_storage->nodes[n].children[2]}, 2 );
    }
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto v1 = *begin++;
    auto v2 = *begin++;
    auto v3 = *begin++;

    return ( ( v1 ^ c1.weight ) && ( v2 ^ c2.weight ) ) || ( ( v3 ^ c3.weight ) && ( v1 ^ c1.weight ) ) || ( ( v3 ^ c3.weight ) && ( v2 ^ c2.weight ) );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    return kitty::ternary_majority( c1.weight ? ~tt1 : tt1, c2.weight ? ~tt2 : tt2, c3.weight ? ~tt3 : tt3 );
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[0].h2 = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h2;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[1].h1 = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const
  {
    return _storage->data.trav_id;
  }

  void incr_trav_id() const
  {
    ++_storage->data.trav_id;
  }
#pragma endregion

#pragma region General methods
  auto& events() const
  {
    return *_events;
  }
#pragma endregion

public:
  std::shared_ptr<mig_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
};

} // namespace mockturtle

namespace std
{

template<>
struct hash<mockturtle::mig_network::signal>
{
  uint64_t operator()( mockturtle::mig_network::signal const& s ) const noexcept
  {
    uint64_t k = s.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

} // namespace std
