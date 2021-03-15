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
  \file mockturtle.hpp
  \brief Main header file for mockturtle

  \author Mathias Soeken
*/

#pragma once

#include "mockturtle/traits.hpp"
#include "mockturtle/io/write_bench.hpp"
#include "mockturtle/io/bench_reader.hpp"
#include "mockturtle/io/verilog_reader.hpp"
#include "mockturtle/io/write_blif.hpp"
#include "mockturtle/io/blif_reader.hpp"
#include "mockturtle/io/write_dot.hpp"
#include "mockturtle/io/write_verilog.hpp"
#include "mockturtle/io/pla_reader.hpp"
#include "mockturtle/io/index_list.hpp"
#include "mockturtle/io/aiger_reader.hpp"
#include "mockturtle/io/write_dimacs.hpp"
#include "mockturtle/algorithms/simulation.hpp"
#include "mockturtle/algorithms/xag_resub_withDC.hpp"
#include "mockturtle/algorithms/xmg_resub.hpp"
#include "mockturtle/algorithms/dont_cares.hpp"
#include "mockturtle/algorithms/equivalence_checking.hpp"
#include "mockturtle/algorithms/equivalence_classes.hpp"
#include "mockturtle/algorithms/extract_linear.hpp"
#include "mockturtle/algorithms/lut_mapping.hpp"
#include "mockturtle/algorithms/bi_decomposition.hpp"
#include "mockturtle/algorithms/cut_rewriting.hpp"
#include "mockturtle/algorithms/cut_enumeration/spectr_cut.hpp"
#include "mockturtle/algorithms/cut_enumeration/cnf_cut.hpp"
#include "mockturtle/algorithms/cut_enumeration/gia_cut.hpp"
#include "mockturtle/algorithms/cut_enumeration/mf_cut.hpp"
#include "mockturtle/algorithms/cleanup.hpp"
#include "mockturtle/algorithms/xag_optimization.hpp"
#include "mockturtle/algorithms/xmg_algebraic_rewriting.hpp"
#include "mockturtle/algorithms/dsd_decomposition.hpp"
#include "mockturtle/algorithms/cnf.hpp"
#include "mockturtle/algorithms/miter.hpp"
#include "mockturtle/algorithms/collapse_mapped.hpp"
#include "mockturtle/algorithms/reconv_cut2.hpp"
#include "mockturtle/algorithms/refactoring.hpp"
#include "mockturtle/algorithms/node_resynthesis/exact.hpp"
#include "mockturtle/algorithms/node_resynthesis/shannon.hpp"
#include "mockturtle/algorithms/node_resynthesis/mig_npn.hpp"
#include "mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp"
#include "mockturtle/algorithms/node_resynthesis/bidecomposition.hpp"
#include "mockturtle/algorithms/node_resynthesis/akers.hpp"
#include "mockturtle/algorithms/node_resynthesis/dsd.hpp"
#include "mockturtle/algorithms/node_resynthesis/xmg_npn.hpp"
#include "mockturtle/algorithms/node_resynthesis/xag_npn.hpp"
#include "mockturtle/algorithms/node_resynthesis/xag_minmc.hpp"
#include "mockturtle/algorithms/node_resynthesis/direct.hpp"
#include "mockturtle/algorithms/akers_synthesis.hpp"
#include "mockturtle/algorithms/gates_to_nodes.hpp"
#include "mockturtle/algorithms/satlut_mapping.hpp"
#include "mockturtle/algorithms/mig_algebraic_rewriting.hpp"
#include "mockturtle/algorithms/xmg_optimization.hpp"
#include "mockturtle/algorithms/cut_enumeration.hpp"
#include "mockturtle/algorithms/cell_window.hpp"
#include "mockturtle/algorithms/decomposition.hpp"
#include "mockturtle/algorithms/node_resynthesis.hpp"
#include "mockturtle/algorithms/linear_resynthesis.hpp"
#include "mockturtle/algorithms/mig_resub.hpp"
#include "mockturtle/algorithms/reconv_cut.hpp"
#include "mockturtle/algorithms/resubstitution.hpp"
#include "mockturtle/algorithms/aig_resub.hpp"
#include "mockturtle/utils/stopwatch.hpp"
#include "mockturtle/utils/truth_table_cache.hpp"
#include "mockturtle/utils/string_utils.hpp"
#include "mockturtle/utils/algorithm.hpp"
#include "mockturtle/utils/progress_bar.hpp"
#include "mockturtle/utils/mixed_radix.hpp"
#include "mockturtle/utils/node_map.hpp"
#include "mockturtle/utils/cuts.hpp"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/events.hpp"
#include "mockturtle/networks/klut.hpp"
#include "mockturtle/networks/xmg.hpp"
#include "mockturtle/networks/xag.hpp"
#include "mockturtle/networks/storage.hpp"
#include "mockturtle/networks/mig.hpp"
#include "mockturtle/properties/migcost.hpp"
#include "mockturtle/properties/mccost.hpp"
#include "mockturtle/mockturtle.hpp"
#include "mockturtle/generators/sorting.hpp"
#include "mockturtle/generators/arithmetic.hpp"
#include "mockturtle/generators/control.hpp"
#include "mockturtle/generators/majority.hpp"
#include "mockturtle/generators/majority_n.hpp"
#include "mockturtle/generators/random_logic_generator.hpp"
#include "mockturtle/generators/modular_arithmetic.hpp"
#include "mockturtle/views/mffc_view.hpp"
#include "mockturtle/views/immutable_view.hpp"
#include "mockturtle/views/topo_view.hpp"
#include "mockturtle/views/window_view.hpp"
#include "mockturtle/views/names_view.hpp"
#include "mockturtle/views/mapping_view.hpp"
#include "mockturtle/views/fanout_view.hpp"
#include "mockturtle/views/cut_view.hpp"
#include "mockturtle/views/depth_view.hpp"
