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

#include <kitty/kitty.hpp>
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

    for ( auto const& latch : latches )
    {
      auto const lit = std::get<0>( latch );
      auto const reset = std::get<1>( latch );

      auto signal = signals[lit];
      ntk_.create_ri( signal, reset );
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

  virtual void on_latch( const std::string& input, const std::string& output, const std::optional<latch_type>& l_type, const std::optional<std::string>& control, const std::optional<latch_init_value>& reset ) const override
  {
    signals[output] = ntk_.create_ro( output );
    if constexpr ( has_set_name_v<Ntk> && has_set_output_name_v<Ntk> )
    {
      ntk_.set_name( signals[output], output );
      ntk_.set_output_name( outputs.size() + latches.size(), input );
    }

    std::string type = "re";
    if( l_type )
    {
      switch ( *l_type )
      {
      case latch_type::FALLING:
        {
          type = "fe";
        }
        break;
      case latch_type::RISING:
        {
          type = "re";
        }
        break;
      case latch_type::ACTIVE_HIGH:
        {
          type = "ah";
        }
        break;
      case latch_type::ACTIVE_LOW:
        {
          type = "al";
        }
        break;
      case latch_type::ASYNC:
        {
          type = "as";
        }
        break;
      default:
        {
          type = "";
        }
        break;
      }
    }

    uint32_t r = 3;
    if ( reset )
    {
      switch ( *reset )
      {
      case latch_init_value::NONDETERMINISTIC:
        {
          r = 2;
        }
        break;
      case latch_init_value::ONE:
        {
          r = 1;
        }
        break;
      case latch_init_value::ZERO:
        {
          r = 0;
        }
        break;
      default:
        break;
      }
    }

    latch_info l_info;
    l_info.init = r;
    l_info.type = type;

    if ( control.has_value() )
      l_info.control = control.value();
    else
      l_info.control = "clock";

    ntk_._storage->latch_information[ntk_.get_node( signals[output] )] = l_info;
    latches.emplace_back( std::make_tuple( input, r, type, l_info.control, "" ) );
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

    assert( cover.size() > 0u );
    assert( cover.at( 0u ).second.size() == 1 );
    auto const first_output_value = cover.at( 0u ).second.at( 0u );

    std::vector<kitty::cube> minterms;
    std::vector<kitty::cube> maxterms;
    for ( const auto& c : cover )
    {
      assert( c.second.size() == 1 );

      auto const output = c.second[0u];
      assert( output == '0' || output == '1' );
      assert( output == first_output_value );
      (void)first_output_value;

      if ( output == '1' )
      {
        minterms.emplace_back( kitty::cube( c.first ) );
      }
      else if ( output == '0' )
      {
        maxterms.emplace_back( ~kitty::cube( c.first ) );
      }
    }

    assert( minterms.size() == 0u || maxterms.size() == 0u );

    kitty::dynamic_truth_table tt( int( inputs.size() ) );
    if ( minterms.size() != 0 )
    {
      kitty::create_from_cubes( tt, minterms, false );
    }
    else if ( maxterms.size() != 0 )
    {
      kitty::create_from_clauses( tt, maxterms, false );
    }

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
  mutable std::vector<std::tuple<std::string, int8_t, std::string, std::string, std::string>> latches;
}; /* blif_reader */

} /* namespace mockturtle */