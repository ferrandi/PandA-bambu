/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "interface/types.hpp"

#include <queue>
#include <vector>

namespace bill {

/*! \brief Adds CNF clauses for `y = ((l_0 ^ ... ^ l_{n-1}) == pol)` to the solver.
 *
 * \param solver Solver
 * \param clause List of literals
 * \param pol Clause polarity
 * \return Literal y
 */
template<typename Solver>
lit_type add_xor_clause(Solver& solver, std::vector<lit_type> const& clause,
                        lit_type::polarities pol = lit_type::polarities::positive)
{
	std::queue<lit_type> lits;
	bool first = pol == lit_type::polarities::negative;
	for (const auto& l : clause) {
		if (first) {
			lits.push(~l);
			first = false;
		} else {
			lits.push(l);
		}
	}

	while (lits.size() > 1) {
		auto const a = lits.front();
		lits.pop();
		auto const b = lits.front();
		lits.pop();

		lits.push(add_tseytin_xor(solver, a, b));
	}

	assert(lits.size() == 1u);
	return lits.front();
}

} /* namespace bill */
