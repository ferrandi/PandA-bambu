/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file tech_library.hpp
  \brief Implements utilities to enumerates gates for technology mapping

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Marcel Walter
*/

#pragma once

#include <cassert>
#include <unordered_map>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>
#include <kitty/static_truth_table.hpp>

#include <parallel_hashmap/phmap.h>

#include "../io/genlib_reader.hpp"
#include "../io/super_reader.hpp"
#include "super_utils.hpp"

namespace mockturtle
{

/*
std::string const mcnc_library = "GATE   inv1    1  O=!a;             PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                 "GATE   inv2    2  O=!a;             PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                 "GATE   inv3    3  O=!a;             PIN * INV 3 999 1.1 0.09 1.1 0.09\n"
                                 "GATE   inv4    4  O=!a;             PIN * INV 4 999 1.2 0.07 1.2 0.07\n"
                                 "GATE   nand2   2  O=!(a*b);         PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
                                 "GATE   nand3   3  O=!(a*b*c);       PIN * INV 1 999 1.1 0.3 1.1 0.3\n"
                                 "GATE   nand4   4  O=!(a*b*c*d);     PIN * INV 1 999 1.4 0.4 1.4 0.4\n"
                                 "GATE   nor2    2  O=!(a+b);         PIN * INV 1 999 1.4 0.5 1.4 0.5\n"
                                 "GATE   nor3    3  O=!(a+b+c);       PIN * INV 1 999 2.4 0.7 2.4 0.7\n"
                                 "GATE   nor4    4  O=!(a+b+c+d);     PIN * INV 1 999 3.8 1.0 3.8 1.0\n"
                                 "GATE   and2    3  O=a*b;            PIN * NONINV 1 999 1.9 0.3 1.9 0.3\n"
                                 "GATE   or2     3  O=a+b;            PIN * NONINV 1 999 2.4 0.3 2.4 0.3\n"
                                 "GATE   xor2a   5  O=a*!b+!a*b;      PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "#GATE  xor2b   5  O=!(a*b+!a*!b);   PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "GATE   xnor2a  5  O=a*b+!a*!b;      PIN * UNKNOWN 2 999 2.1 0.5 2.1 0.5\n"
                                 "#GATE  xnor2b  5  O=!(a*!b+!a*b);   PIN * UNKNOWN 2 999 2.1 0.5 2.1 0.5\n"
                                 "GATE   aoi21   3  O=!(a*b+c);       PIN * INV 1 999 1.6 0.4 1.6 0.4\n"
                                 "GATE   aoi22   4  O=!(a*b+c*d);     PIN * INV 1 999 2.0 0.4 2.0 0.4\n"
                                 "GATE   oai21   3  O=!((a+b)*c);     PIN * INV 1 999 1.6 0.4 1.6 0.4\n"
                                 "GATE   oai22   4  O=!((a+b)*(c+d)); PIN * INV 1 999 2.0 0.4 2.0 0.4\n"
                                 "GATE   buf     2  O=a;              PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                 "GATE   zero    0  O=CONST0;\n"
                                 "GATE   one     0  O=CONST1;";
*/

enum class classification_type : uint32_t
{
  /* generate the NP configurations (n! * 2^n) */
  np_configurations = 0,
  /* generate the P configurations (n!) and N-canonization */
  p_configurations = 1,
};

struct tech_library_params
{
  /*! \brief reports np enumerations */
  bool verbose{ false };

  /*! \brief reports all the entries in the library */
  bool very_verbose{ false };
};

template<unsigned NInputs>
struct supergate
{
  /* pointer to the root gate */
  composed_gate<NInputs> const* root{};

  /* area */
  float area{ 0.0 };

  /* pin-to-pin delay */
  std::array<float, NInputs> tdelay{};

  /* np permutation vector */
  std::vector<uint8_t> permutation{};

  /* pin negations */
  uint8_t polarity{ 0 };
};

/*! \brief Library of gates for Boolean matching
 *
 * This class creates a technology library from a set
 * of input gates. Each NP- or P-configuration of the gates
 * are enumerated and inserted in the library.
 *
 * The configuration is selected using the template
 * parameter `Configuration`. P-configuration is suggested
 * for big libraries with few symmetric gates. The template
 * parameter `NInputs` selects the maximum number of variables
 * allowed for a gate in the library.
 *
 * The library can be generated also using supergates definitions.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      std::vector<gate> gates;
      lorina::read_genlib( "file.genlib", genlib_reader( gates ) );
      // standard library
      mockturtle::tech_library lib( gates );

      super_lib supergates_spec;
      lorina::read_super( "file.super", super_reader( supergates_spec ) );
      // library with supergates
      mockturtle::tech_library lib_super( gates, supergates_spec );
   \endverbatim
 */
template<unsigned NInputs = 4u, classification_type Configuration = classification_type::np_configurations>
class tech_library
{
  using supergates_list_t = std::vector<supergate<NInputs>>;
  using tt_hash = kitty::hash<kitty::static_truth_table<NInputs>>;
  using lib_t = phmap::flat_hash_map<kitty::static_truth_table<NInputs>, supergates_list_t, tt_hash>;

public:
  explicit tech_library( std::vector<gate> const& gates, tech_library_params const ps = {}, super_lib const& supergates_spec = {} )
      : _gates( gates ),
        _supergates_spec( supergates_spec ),
        _ps( ps ),
        _super( _gates, _supergates_spec ),
        _use_supergates( false ),
        _super_lib()
  {
    generate_library();
  }

  explicit tech_library( std::vector<gate> const& gates, super_lib const& supergates_spec, tech_library_params const ps = {} )
      : _gates( gates ),
        _supergates_spec( supergates_spec ),
        _ps( ps ),
        _super( _gates, _supergates_spec, super_utils_params{ ps.verbose } ),
        _use_supergates( true ),
        _super_lib()
  {
    generate_library();
  }

  /*! \brief Get the gates matching the function.
   *
   * Returns a list of gates that match the function represented
   * by the truth table.
   */
  const supergates_list_t* get_supergates( kitty::static_truth_table<NInputs> const& tt ) const
  {
    auto match = _super_lib.find( tt );
    if ( match != _super_lib.end() )
      return &match->second;
    return nullptr;
  }

  /*! \brief Get inverter information.
   *
   * Returns area, delay, and ID of the smallest inverter.
   */
  const std::tuple<float, float, uint32_t> get_inverter_info() const
  {
    return std::make_tuple( _inv_area, _inv_delay, _inv_id );
  }

  /*! \brief Get buffer information.
   *
   * Returns area, delay, and ID of the smallest buffer.
   */
  const std::tuple<float, float, uint32_t> get_buffer_info() const
  {
    return std::make_tuple( _buf_area, _buf_delay, _buf_id );
  }

  /*! \brief Returns the maximum number of variables of the gates. */
  unsigned max_gate_size()
  {
    return _max_size;
  }

  /*! \brief Returns the original gates. */
  const std::vector<gate> get_gates() const
  {
    return _gates;
  }

private:
  void generate_library()
  {
    bool inv = false;
    bool buf = false;

    /* extract the smallest inverter and buffer info */
    for ( auto& gate : _gates )
    {
      if ( gate.function.num_vars() == 1 )
      {
        /* extract inverter delay and area */
        if ( kitty::is_const0( kitty::cofactor1( gate.function, 0 ) ) )
        {
          /* get the smallest area inverter */
          if ( !inv || gate.area < _inv_area )
          {
            _inv_area = gate.area;
            _inv_delay = compute_worst_delay( gate );
            _inv_id = gate.id;
            inv = true;
          }
        }
        else
        {
          /* get the smallest area buffer */
          if ( !buf || gate.area < _buf_area )
          {
            _buf_area = gate.area;
            _buf_delay = compute_worst_delay( gate );
            _buf_id = gate.id;
            buf = true;
          }
        }
      }
    }

    auto const& supergates = _super.get_super_library();
    uint32_t const standard_gate_size = _super.get_standard_library_size();

    /* generate the configurations for the standard gates */
    uint32_t i = 0u;
    for ( auto const& gate : supergates )
    {
      uint32_t np_count = 0;

      if ( gate.root == nullptr )
      {
        /* exclude PIs */
        continue;
      }

      _max_size = std::max( _max_size, gate.num_vars );

      if ( i++ < standard_gate_size )
      {
        const auto on_np = [&]( auto const& tt, auto neg, auto const& perm ) {
          supergate<NInputs> sg = { &gate,
                                    static_cast<float>( gate.area ),
                                    {},
                                    perm,
                                    0 };

          for ( auto i = 0u; i < perm.size() && i < NInputs; ++i )
          {
            sg.tdelay[i] = gate.tdelay[perm[i]];
            sg.polarity |= ( ( neg >> perm[i] ) & 1 ) << i; /* permutate input negation to match the right pin */
          }

          const auto static_tt = kitty::extend_to<NInputs>( tt );

          auto& v = _super_lib[static_tt];

          /* ordered insert by ascending area and number of input pins */
          auto it = std::lower_bound( v.begin(), v.end(), sg, [&]( auto const& s1, auto const& s2 ) {
            if ( s1.area < s2.area )
              return true;
            if ( s1.area > s2.area )
              return false;
            if ( s1.root->num_vars < s2.root->num_vars )
              return true;
            if ( s1.root->num_vars > s2.root->num_vars )
              return true;
            return s1.root->id < s2.root->id;
          } );

          bool to_add = true;
          /* search for duplicated element due to symmetries */
          while ( it != v.end() )
          {
            if ( sg.root->id == it->root->id )
            {
              /* if already in the library exit, else ignore permutations if with equal delay cost */
              if ( sg.polarity == it->polarity && sg.tdelay == it->tdelay )
              {
                to_add = false;
                break;
              }
            }
            else
            {
              break;
            }
            ++it;
          }

          if ( to_add )
          {
            v.insert( it, sg );
            ++np_count;
          }
        };

        const auto on_p = [&]( auto const& tt, auto const& perm ) {
          /* get all the configurations that lead to the N-class representative */
          auto [tt_canon, phases] = kitty::exact_n_canonization_complete( tt );

          for ( auto phase : phases )
          {
            supergate<NInputs> sg = { &gate,
                                      static_cast<float>( gate.area ),
                                      {},
                                      perm,
                                      static_cast<uint8_t>( phase ) };

            for ( auto i = 0u; i < perm.size() && i < NInputs; ++i )
            {
              sg.tdelay[i] = gate.tdelay[perm[i]];
            }

            const auto static_tt = kitty::extend_to<NInputs>( tt_canon );

            auto& v = _super_lib[static_tt];

            /* ordered insert by ascending area and number of input pins */
            auto it = std::lower_bound( v.begin(), v.end(), sg, [&]( auto const& s1, auto const& s2 ) {
              if ( s1.area < s2.area )
                return true;
              if ( s1.area > s2.area )
                return false;
              if ( s1.root->num_vars < s2.root->num_vars )
                return true;
              if ( s1.root->num_vars > s2.root->num_vars )
                return true;
              return s1.root->id < s2.root->id;
            } );

            bool to_add = true;
            /* search for duplicated element due to symmetries */
            while ( it != v.end() )
            {
              if ( sg.root->id == it->root->id )
              {
                /* if already in the library exit, else ignore permutations if with equal delay cost */
                if ( sg.polarity == it->polarity && sg.tdelay == it->tdelay )
                {
                  to_add = false;
                  break;
                }
              }
              else
              {
                break;
              }
              ++it;
            }

            if ( to_add )
            {
              v.insert( it, sg );
              ++np_count;
            }
          }
        };

        if constexpr ( Configuration == classification_type::np_configurations )
        {
          /* NP enumeration of the function */
          const auto tt = gate.function;
          kitty::exact_np_enumeration( tt, on_np );
        }
        else
        {
          /* P enumeration followed by N canonization of the function */
          const auto tt = gate.function;
          kitty::exact_p_enumeration( tt, on_p );
        }
      }
      else
      {
        /* process the supergates */
        if ( !gate.is_super )
        {
          /* ignore simple gates */
          continue;
        }

        const auto on_np = [&]( auto const& tt, auto neg ) {
          std::vector<uint8_t> perm( gate.num_vars );
          std::iota( perm.begin(), perm.end(), 0u );

          supergate<NInputs> sg = { &gate,
                                    static_cast<float>( gate.area ),
                                    {},
                                    perm,
                                    static_cast<uint8_t>( neg ) };

          for ( auto i = 0u; i < perm.size() && i < NInputs; ++i )
          {
            sg.tdelay[i] = gate.tdelay[perm[i]];
          }

          const auto static_tt = kitty::extend_to<NInputs>( tt );

          auto& v = _super_lib[static_tt];

          /* ordered insert by ascending area and number of input pins */
          auto it = std::lower_bound( v.begin(), v.end(), sg, [&]( auto const& s1, auto const& s2 ) {
            if ( s1.area < s2.area )
              return true;
            if ( s1.area > s2.area )
              return false;
            if ( s1.root->num_vars < s2.root->num_vars )
              return true;
            if ( s1.root->num_vars > s2.root->num_vars )
              return true;
            return s1.root->id < s2.root->id;
          } );

          bool to_add = true;
          /* search for duplicated element due to symmetries */
          while ( it != v.end() )
          {
            if ( sg.root->id == it->root->id )
            {
              /* if already in the library exit, else ignore permutations if with equal delay cost */
              if ( sg.polarity == it->polarity && sg.tdelay == it->tdelay )
              {
                to_add = false;
                break;
              }
            }
            else
            {
              break;
            }
            ++it;
          }

          if ( to_add )
          {
            v.insert( it, sg );
            ++np_count;
          }
        };

        const auto on_p = [&]() {
          auto [tt_canon, phases] = kitty::exact_n_canonization_complete( gate.function );
          std::vector<uint8_t> perm( gate.num_vars );
          std::iota( perm.begin(), perm.end(), 0u );

          for ( auto phase : phases )
          {
            supergate<NInputs> sg = { &gate,
                                      static_cast<float>( gate.area ),
                                      {},
                                      perm,
                                      static_cast<uint8_t>( phase ) };

            for ( auto i = 0u; i < perm.size() && i < NInputs; ++i )
            {
              sg.tdelay[i] = gate.tdelay[perm[i]];
            }

            const auto static_tt = kitty::extend_to<NInputs>( tt_canon );

            auto& v = _super_lib[static_tt];

            /* ordered insert by ascending area and number of input pins */
            auto it = std::lower_bound( v.begin(), v.end(), sg, [&]( auto const& s1, auto const& s2 ) {
              if ( s1.area < s2.area )
                return true;
              if ( s1.area > s2.area )
                return false;
              if ( s1.root->num_vars < s2.root->num_vars )
                return true;
              if ( s1.root->num_vars > s2.root->num_vars )
                return true;
              return s1.root->id < s2.root->id;
            } );

            bool to_add = true;
            /* search for duplicated element due to symmetries */
            while ( it != v.end() )
            {
              if ( sg.root->id == it->root->id )
              {
                /* if already in the library exit, else ignore permutations if with equal delay cost */
                if ( sg.polarity == it->polarity && sg.tdelay == it->tdelay )
                {
                  to_add = false;
                  break;
                }
              }
              else
              {
                break;
              }
              ++it;
            }

            if ( to_add )
            {
              v.insert( it, sg );
              ++np_count;
            }
          }
        };

        if constexpr ( Configuration == classification_type::np_configurations )
        {
          /* N enumeration of the function */
          const auto tt = gate.function;
          kitty::exact_n_enumeration( tt, on_np );
        }
        else
        {
          /* N canonization of the function */
          const auto tt = gate.function;
          on_p();
        }
      }

      if ( _ps.verbose )
      {
        std::cout << "Gate " << gate.root->name << ", num_vars = " << gate.num_vars << ", np entries = " << np_count << std::endl;
      }
    }

    if ( !inv )
    {
      std::cerr << "[i] WARNING: inverter gate has not been detected in the library" << std::endl;
    }

    if ( !buf )
    {
      std::cerr << "[i] WARNING: buffer gate has not been detected in the library" << std::endl;
    }

    if ( _ps.very_verbose )
    {
      for ( auto const& entry : _super_lib )
      {
        kitty::print_hex( entry.first );
        std::cout << ": ";
        for ( auto const& gate : entry.second )
        {
          printf( "%d(a:%.2f, p:%d) ", gate.root->id, gate.area, gate.polarity );
        }
        std::cout << std::endl;
      }
    }
  }

  float compute_worst_delay( gate const& g )
  {
    float worst_delay = 0.0f;

    /* consider only block_delay */
    for ( auto const& pin : g.pins )
    {
      float worst_pin_delay = static_cast<float>( std::max( pin.rise_block_delay, pin.fall_block_delay ) );
      worst_delay = std::max( worst_delay, worst_pin_delay );
    }
    return worst_delay;
  }

private:
  /* inverter info */
  float _inv_area{ 0.0 };
  float _inv_delay{ 0.0 };
  uint32_t _inv_id{ UINT32_MAX };

  /* buffer info */
  float _buf_area{ 0.0 };
  float _buf_delay{ 0.0 };
  uint32_t _buf_id{ UINT32_MAX };

  unsigned _max_size{ 0 }; /* max #fanins of the gates in the library */

  bool _use_supergates;

  std::vector<gate> const _gates;    /* collection of gates */
  super_lib const& _supergates_spec; /* collection of supergates declarations */
  tech_library_params const _ps;
  super_utils<NInputs> _super; /* supergates generation */
  lib_t _super_lib;            /* library of enumerated gates */
};                             /* class tech_library */

template<typename Ntk, unsigned NInputs>
struct exact_supergate
{
  signal<Ntk> const root;

  /* number of inputs of the supergate */
  uint8_t n_inputs{ 0 };
  /* saved polarities for inputs and/or outputs */
  uint8_t polarity{ 0 };

  /* area */
  float area{ 0 };
  /* worst delay */
  float worstDelay{ 0 };
  /* pin-to-pin delay */
  std::array<float, NInputs> tdelay{ 0 };

  exact_supergate( signal<Ntk> const root )
      : root( root ) {}
};

struct exact_library_params
{
  /* area of a gate */
  float area_gate{ 1.0f };
  /* area of an inverter */
  float area_inverter{ 0.0f };
  /* delay of a gate */
  float delay_gate{ 1.0f };
  /* delay of an inverter */
  float delay_inverter{ 0.0f };

  /* classify in NP instead of NPN */
  bool np_classification{ true };
  /* verbose */
  bool verbose{ false };
};

/*! \brief Library of graph structures for Boolean matching
 *
 * This class creates a technology library from a database
 * of structures classified in NPN classes. Each NPN-entry in
 * the database is stored in its NP class by removing the output
 * inverter if present. The class creates supergates from the
 * database computing area and delay information.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      mockturtle::mig_npn_resynthesis mig_resyn{true};
      mockturtle::exact_library<mockturtle::mig_network, mockturtle::mig_npn_resynthesis> lib( mig_resyn );
   \endverbatim
 */
template<typename Ntk, class RewritingFn, unsigned NInputs = 4u>
class exact_library
{
  using supergates_list_t = std::vector<exact_supergate<Ntk, NInputs>>;
  using tt_hash = kitty::hash<kitty::static_truth_table<NInputs>>;
  using lib_t = std::unordered_map<kitty::static_truth_table<NInputs>, supergates_list_t, tt_hash>;

public:
  explicit exact_library( RewritingFn const& rewriting_fn, exact_library_params const& ps = {} )
      : _database(),
        _rewriting_fn( rewriting_fn ),
        _ps( ps ),
        _super_lib()
  {
    generate_library();
  }

  /*! \brief Get the structures matching the function.
   *
   * Returns a list of graph structures that match the function
   * represented by the truth table.
   */
  const supergates_list_t* get_supergates( kitty::static_truth_table<NInputs> const& tt ) const
  {
    auto match = _super_lib.find( tt );
    if ( match != _super_lib.end() )
      return &match->second;
    return nullptr;
  }

  /*! \brief Returns the NPN database of structures. */
  const Ntk& get_database() const
  {
    return _database;
  }

  /*! \brief Get inverter information.
   *
   * Returns area, and delay cost of the inverter.
   */
  const std::tuple<float, float> get_inverter_info() const
  {
    return std::make_pair( _ps.area_inverter, _ps.delay_inverter );
  }

private:
  void generate_library()
  {
    std::vector<signal<Ntk>> pis;
    for ( auto i = 0u; i < NInputs; ++i )
    {
      pis.push_back( _database.create_pi() );
    }

    /* Compute NPN classes */
    std::unordered_set<kitty::static_truth_table<NInputs>, tt_hash> classes;
    kitty::static_truth_table<NInputs> tt;
    do
    {
      const auto res = kitty::exact_npn_canonization( tt );
      classes.insert( std::get<0>( res ) );
      kitty::next_inplace( tt );
    } while ( !kitty::is_const0( tt ) );

    /* Construct supergates */
    for ( auto const& entry : classes )
    {
      supergates_list_t supergates_pos;
      supergates_list_t supergates_neg;
      auto const not_entry = ~entry;

      const auto add_supergate = [&]( auto const& f_new ) {
        bool complemented = _database.is_complemented( f_new );
        auto f = f_new;
        if ( _ps.np_classification && complemented )
        {
          f = !f;
        }
        exact_supergate<Ntk, NInputs> sg( f );
        compute_info( sg );
        if ( _ps.np_classification && complemented )
        {
          supergates_neg.push_back( sg );
        }
        else
        {
          supergates_pos.push_back( sg );
        }
        _database.create_po( f );
        return true;
      };

      kitty::dynamic_truth_table function = kitty::extend_to( entry, NInputs );
      _rewriting_fn( _database, function, pis.begin(), pis.end(), add_supergate );
      if ( supergates_pos.size() > 0 )
        _super_lib.insert( { entry, supergates_pos } );
      if ( _ps.np_classification && supergates_neg.size() > 0 )
        _super_lib.insert( { not_entry, supergates_neg } );
    }

    if ( _ps.verbose )
    {
      std::cout << "Classified in " << _super_lib.size() << " entries" << std::endl;
      for ( auto const& pair : _super_lib )
      {
        kitty::print_hex( pair.first );
        std::cout << ": ";

        for ( auto const& gate : pair.second )
        {
          printf( "%.2f,%.2f,%x,%d,:", gate.worstDelay, gate.area, gate.polarity, gate.n_inputs );
          for ( auto j = 0u; j < NInputs; ++j )
            printf( "%.2f/", gate.tdelay[j] );
          std::cout << " ";
        }
        std::cout << std::endl;
      }
    }
  }

  /* Computes delay and area info */
  void compute_info( exact_supergate<Ntk, NInputs>& sg )
  {
    _database.incr_trav_id();
    /* info does not consider input and output inverters */
    bool compl_root = _database.is_complemented( sg.root );
    auto const root = compl_root ? !sg.root : sg.root;
    sg.area = compute_info_rec( sg, root, 0.0f );

    /* output polarity */
    sg.polarity |= ( unsigned( compl_root ) ) << NInputs;
    /* number of inputs */
    for ( auto i = 0u; i < NInputs; ++i )
    {
      sg.tdelay[i] *= -1; /* invert to positive value */
      if ( sg.tdelay[i] != 0.0f )
        sg.n_inputs++;
    }
    sg.worstDelay *= -1;
  }

  float compute_info_rec( exact_supergate<Ntk, NInputs>& sg, signal<Ntk> const& root, float delay )
  {
    auto n = _database.get_node( root );

    if ( _database.is_constant( n ) )
      return 0.0f;

    float area = 0.0f;
    float tdelay = delay;

    if ( _database.is_pi( n ) )
    {
      sg.tdelay[_database.index_to_node( n ) - 1u] = std::min( sg.tdelay[_database.index_to_node( n ) - 1u], tdelay );
      sg.worstDelay = std::min( sg.worstDelay, tdelay );
      sg.polarity |= ( unsigned( _database.is_complemented( root ) ) ) << ( _database.index_to_node( n ) - 1u );
      return area;
    }

    tdelay -= _ps.delay_gate;

    /* add gate area once */
    if ( _database.visited( n ) != _database.trav_id() )
    {
      area += _ps.area_gate;
      _database.set_value( n, 0u );
      _database.set_visited( n, _database.trav_id() );
    }

    if ( _database.is_complemented( root ) )
    {
      tdelay -= _ps.delay_inverter;
      /* add inverter area only once (shared by fanout) */
      if ( _database.value( n ) == 0u )
      {
        area += _ps.area_inverter;
        _database.set_value( n, 1u );
      }
    }

    _database.foreach_fanin( n, [&]( auto const& child ) {
      area += compute_info_rec( sg, child, tdelay );
    } );

    return area;
  }

private:
  Ntk _database;
  RewritingFn const& _rewriting_fn;
  exact_library_params const _ps;
  lib_t _super_lib;
}; /* class exact_library */

} // namespace mockturtle
