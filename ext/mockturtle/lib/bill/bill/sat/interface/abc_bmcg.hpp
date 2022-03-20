/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "common.hpp"
#include "types.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace bill {

#if !defined(BILL_WINDOWS_PLATFORM)
template<>
class solver<solvers::bmcg> {
	using solver_type = pabc::bmcg_sat_solver;

public:
#pragma region Constructors
	solver()
	{
		solver_ = pabc::bmcg_sat_solver_start();
	}

	~solver()
	{
		pabc::bmcg_sat_solver_stop(solver_);
		solver_ = nullptr;
	}

	/* disallow copying */
	solver(solver<solvers::bmcg> const&) = delete;
	solver<solvers::bmcg>& operator=(const solver<solvers::bmcg>&) = delete;
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		pabc::bmcg_sat_solver_reset(solver_);
		state_ = result::states::undefined;
		variable_counter_ = 0u;
	}

	var_type add_variable()
	{
		variable_counter_++;
		return pabc::bmcg_sat_solver_addvar(solver_);
	}

	void add_variables(uint32_t num_variables = 1)
	{
		for (auto i = 0u; i < num_variables; ++i) {
			pabc::bmcg_sat_solver_addvar(solver_);
		}
		variable_counter_ += num_variables;
	}

	auto add_clause(std::vector<lit_type>::const_iterator it,
	                std::vector<lit_type>::const_iterator ie)
	{
		auto counter = 0u;
		while (it != ie) {
			literals[counter++] = pabc::Abc_Var2Lit(it->variable(),
			                                        it->is_complemented());
			++it;
		}
		auto const result = pabc::bmcg_sat_solver_addclause(solver_, literals, counter);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		return add_clause(clause.begin(), clause.end());
	}

	auto add_clause(lit_type lit)
	{
		return add_clause(std::vector<lit_type>{lit});
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0u; i < num_variables(); ++i) {
			auto const value = pabc::bmcg_sat_solver_read_cex_varvalue(solver_, i);
			if (value == 1) {
				model.emplace_back(lbool_type::true_);
			} else {
				model.emplace_back(lbool_type::false_);
			}
		}
		return result(model);
	}

	result get_result() const
	{
		assert(state_ != result::states::dirty);
		if (state_ == result::states::satisfiable) {
			return get_model();
		} else {
			return result();
		}
	}

	result::states solve(std::vector<lit_type> const& assumptions = {},
	                     uint32_t conflict_limit = 0)
	{
		/* special case: empty solver state */
		if (num_variables() == 0u)
			return result::states::undefined;

		if (conflict_limit > 0)
			pabc::bmcg_sat_solver_set_conflict_budget(solver_, conflict_limit);

		int result;
		if (assumptions.size() > 0u) {
			/* solve with assumptions */
			uint32_t counter = 0u;
			auto it = assumptions.begin();
			while (it != assumptions.end()) {
				literals[counter++] = pabc::Abc_Var2Lit(it->variable(),
				                                        it->is_complemented());
				++it;
			}
			result = pabc::bmcg_sat_solver_solve(solver_, literals, counter);
		} else {
			/* solve without assumptions */
			result = pabc::bmcg_sat_solver_solve(solver_, 0, 0);
		}

		if (result == 1) {
			state_ = result::states::satisfiable;
		} else if (result == -1) {
			state_ = result::states::unsatisfiable;
		} else {
			state_ = result::states::undefined;
		}

		return state_;
	}
#pragma endregion

#pragma region Properties
	uint32_t num_variables() const
	{
		return variable_counter_;
	}

	uint32_t num_clauses() const
	{
		return pabc::bmcg_sat_solver_clausenum(solver_);
	}
#pragma endregion

private:
	/*! \brief Backend solver */
	solver_type* solver_ = nullptr;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::undefined;

	/*! \brief Temporary storage for one clause */
	pabc::lit literals[2048];

	/*! \brief Count the number of variables */
	uint32_t variable_counter_ = 0u;
};
#endif

} // namespace bill
