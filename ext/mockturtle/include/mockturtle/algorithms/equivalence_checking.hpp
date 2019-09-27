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
  \file equivalence_checking.hpp
  \brief Combinational equivalence checking

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "../traits.hpp"
#include "../utils/stopwatch.hpp"
#include "cnf.hpp"

#include <fmt/format.h>
#include <percy/solvers/bsat2.hpp>

namespace mockturtle
{

/*! \brief Parameters for equivalence_checking.
 *
 * The data structure `equivalence_checking_params` holds configurable
 * parameters with default arguments for `equivalence_checking`.
 */
struct equivalence_checking_params
{
  /*! \brief Conflict limit for SAT solver.
   *
   * The default limit is 0, which means the number of conflicts is not used
   * as a resource limit.
   */
  uint32_t conflict_limit{0u};

  /* \brief Be verbose. */
  bool verbose{false};
};

/*! \brief Statistics for equivalence_checking.
 *
 * The data structure `equivalence_checking_stats` provides data collected by
 * running `equivalence_checking`.
 */
struct equivalence_checking_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{};

  /*! \brief Counter-example, in case miter is not equivalent. */
  std::vector<bool> counter_example;

  void report() const
  {
    std::cout << fmt::format( "[i] total time     = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

template<class Ntk>
class equivalence_checking_impl
{
public:
  equivalence_checking_impl( Ntk const& miter, equivalence_checking_params const& ps, equivalence_checking_stats& st )
      : miter_( miter ),
        ps_( ps ),
        st_( st )
  {
  }

  std::optional<bool> run()
  {
    stopwatch<> t( st_.time_total );

    percy::bsat_wrapper solver;
    int output = generate_cnf( miter_, [&]( auto const& clause ) {
      solver.add_clause( clause );
    } )[0];

    const auto res = solver.solve( &output, &output + 1, 0 );

    switch ( res )
    {
    default:
      return std::nullopt;
    case percy::synth_result::success:
    {
      st_.counter_example.clear();
      for ( auto i = 1u; i <= miter_.num_pis(); ++i )
      {
        st_.counter_example.push_back( solver.var_value( i ) );
      }
      return false;
    }
    case percy::synth_result::failure:
      return true;
    }
  }

private:
  Ntk const& miter_;
  equivalence_checking_params const& ps_;
  equivalence_checking_stats& st_;
};

} // namespace detail

/*! \brief Combinational equivalence checking.
 *
 * This function expects as input a miter circuit that can be generated, e.g.,
 * with the function `miter`.  It returns an optional which is `nullopt`, if no
 * solution can be found (this happens when a resource limit is set using the
 * function's parameters).  Otherwise it returns `true`, if the miter is
 * equivalent or `false`, if the miter is not equivalent.  In the latter case
 * the counter example is written to the statistics pointer as a
 * `std::vector<bool>` following the same order as the primary inputs.
 *
 * \param miter Miter network
 * \param ps Parameters
 * \param st Statistics
 */
template<class Ntk>
std::optional<bool> equivalence_checking( Ntk const& miter, equivalence_checking_params const& ps = {}, equivalence_checking_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  if ( miter.num_pos() != 1u )
  {
    std::cout << "[e] miter network must have a single output\n";
    return std::nullopt;
  }

  equivalence_checking_stats st;
  detail::equivalence_checking_impl<Ntk> impl( miter, ps, st );
  const auto result = impl.run();

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }

  return result;
}

} /* namespace mockturtle */
