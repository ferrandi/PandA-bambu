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
  \file blif_reader.hpp
  \brief Lorina reader for BLIF files

  \author Heinz Riener
*/

#pragma once

#include <mockturtle/networks/aig.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <lorina/blif.hpp>

#include <map>
#include <string>
#include <vector>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Lorina reader callback for BLIF files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_node`
 * - `get_constant`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      klut_network klut;
      lorina::read_blif( "file.blif", blif_reader( klut ) );
   \endverbatim
 */
template<typename Ntk>
class blif_reader : public lorina::blif_reader
{
public:
  explicit blif_reader( Ntk& ntk )
    : ntk_( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_node_v<Ntk>, "Ntk does not implement the create_node function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
  }

  ~blif_reader()
  {
    for ( auto const& o : outputs )
    {
      ntk_.create_po( signals[o], o );
    }
  }

  virtual void on_model( const std::string& model_name ) const override
  {
    (void)model_name;
  }

  virtual void on_input( const std::string& name ) const override
  {
    signals[name] = ntk_.create_pi( name );
    if constexpr ( has_set_name_v<Ntk> )
    {
      ntk_.set_name( signals[name], name );
    }
  }

  virtual void on_output( const std::string& name ) const override
  {
    if constexpr ( has_set_output_name_v<Ntk> )
    {
      ntk_.set_output_name( outputs.size(), name );
    }
    outputs.emplace_back( name );
  }

  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const output_cover_t& cover ) const override
  {
    if ( inputs.size() == 0u )
    {
      if ( cover.size() == 0u )
      {
        signals[output] = ntk_.get_constant( false );
        return;
      }

      assert( cover.size() == 1u );
      assert( cover.at( 0u ).first.size() == 0u );
      assert( cover.at( 0u ).second.size() == 1u );

      auto const assigned_value = cover.at( 0u ).second.at( 0u );
      auto const const_value = ntk_.get_constant( assigned_value == '1' ? true : false );
      signals[output] = const_value;
      return;
    }

    auto const len = 1u << inputs.size();

    assert( cover.size() > 0u );
    assert( cover.at( 0u ).second.size() == 1 );
    auto const first_output_value = cover.at( 0u ).second.at( 0u );
    auto const default_value = first_output_value == '0' ? '1' : '0';

    std::string func( len, default_value );
    for ( const auto& c : cover )
    {
      assert( c.second.size() == 1 );

      uint32_t pos = 0u;
      for ( auto i = 0u; i < c.first.size(); ++i )
      {
        if ( c.first.at( i ) == '1' )
          pos += 1u << i;
      }

      func[pos] = c.second.at( 0u );
    }

    kitty::dynamic_truth_table tt( static_cast<int>( inputs.size() ) );
    std::reverse( std::begin( func ), std::end( func ) );
    kitty::create_from_binary_string( tt, func );

    std::vector<signal<Ntk>> input_signals;
    for ( const auto& i : inputs )
    {
      assert( signals.find( i ) != signals.end() );
      input_signals.push_back( signals.at( i ) );
    }
    signals[output] = ntk_.create_node( input_signals, tt );
  }

  virtual void on_end() const override {}

  virtual void on_comment( const std::string& comment ) const override
  {
    (void)comment;
  }

private:
  Ntk& ntk_;

  mutable std::map<std::string, signal<Ntk>> signals;
  mutable std::vector<std::string> outputs;
}; /* blif_reader */

} /* namespace mockturtle */
