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
  \file events.hpp
  \brief Event API for updating a logic network.
  \author Mathias Soeken
*/

#pragma once

#include <functional>
#include <vector>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Network events.
 *
 * This data structure can be returned by a network.  Clients can add functions
 * to network events to call code whenever an event occurs.  Events are adding
 * a node, modifying a node, and deleting a node.
 */
template<class Ntk>
struct network_events
{
  /*! \brief Event when node `n` is added. */
  std::vector<std::function<void( node<Ntk> const& n )>> on_add;

  /*! \brief Event when `n` is modified.
   *
   * The event also informs about the previous children.  Note that the new
   * children are already available at the time the event is triggered.
   */
  std::vector<std::function<void( node<Ntk> const& n, std::vector<signal<Ntk>> const& previous_children )>> on_modified;

  /*! \brief Event when `n` is deleted. */
  std::vector<std::function<void( node<Ntk> const& n )>> on_delete;
};

} // namespace mockturtle
