/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "interface/types.hpp"

#include <vector>

namespace bill {

template<typename Cnf>
inline void at_least_one(std::vector<var_type> const& variables, Cnf& cnf_builder)
{
	std::vector<lit_type> clause;
	for (auto var : variables) {
		clause.emplace_back(var, positive_polarity);
	}
	cnf_builder.add_clause(clause);
}

template<typename Cnf>
inline void at_most_one_pairwise(std::vector<var_type> const& variables, Cnf& cnf_builder)
{
	std::vector<lit_type> clause;
	for (auto i = 0u; i < variables.size() - 1; ++i) {
		for (auto j = i + 1u; j < variables.size(); ++j) {
			clause.emplace_back(variables[i], negative_polarity);
			clause.emplace_back(variables[j], negative_polarity);
			cnf_builder.add_clause(clause);
			clause.clear();
		}
	}
}

} // namespace bill