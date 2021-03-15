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

template<>
class solver<solvers::ghack> {
	using solver_type = GHack::Solver;

public:
#pragma region Constructors
	solver()
	    : solver_(std::make_unique<solver_type>())
	{}

	/* disallow copying */
	solver(solver<solvers::ghack> const&) = delete;
	solver<solvers::ghack>& operator=(const solver<solvers::ghack>&) = delete;
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		solver_.reset();
		solver_ = std::make_unique<solver_type>();
		state_ = result::states::undefined;
	}

	var_type add_variable()
	{
		return solver_->newVar();
	}

	void add_variables(uint32_t num_variables = 1)
	{
		for (auto i = 0u; i < num_variables; ++i) {
			solver_->newVar();
		}
	}

	auto add_clause(std::vector<lit_type>::const_iterator it,
	                std::vector<lit_type>::const_iterator ie)
	{
		GHack::vec<GHack::Lit> literals;
		while (it != ie) {
			literals.push(GHack::mkLit(it->variable(), it->is_complemented()));
			++it;
		}
		auto const result = solver_->addClause_(literals);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		return add_clause(clause.begin(), clause.end());
	}

	auto add_clause(lit_type lit)
	{
		auto const result = solver_->addClause(
		    GHack::mkLit(lit.variable(), lit.is_complemented()));
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0; i < solver_->model.size(); ++i) {
			if (solver_->model[i] == GHack::l_False) {
				model.emplace_back(lbool_type::false_);
			} else if (solver_->model[i] == GHack::l_True) {
				model.emplace_back(lbool_type::true_);
			} else {
				model.emplace_back(lbool_type::undefined);
			}
		}
		return result(model);
	}

	result get_core() const
	{
		assert(state_ == result::states::unsatisfiable);
		result::clause_type unsat_core;
		for (auto i = 0; i < solver_->conflict.size(); ++i) {
			unsat_core.emplace_back(GHack::var(solver_->conflict[i]),
			                        GHack::sign(solver_->conflict[i]) ?
			                            negative_polarity :
			                            positive_polarity);
		}
		return result(unsat_core);
	}

	result get_result() const
	{
		assert(state_ != result::states::dirty);
		if (state_ == result::states::satisfiable) {
			return get_model();
		} else if (state_ == result::states::unsatisfiable) {
			return get_core();
		} else {
			return result();
		}
	}

	result::states solve(std::vector<lit_type> const& assumptions = {},
	                     uint32_t conflict_limit = 0)
	{
		if (state_ != result::states::dirty && assumptions.empty()) {
			return state_;
		}

		assert(solver_->okay() == true);
		if (conflict_limit) {
			solver_->setConfBudget(conflict_limit);
		}

		GHack::vec<GHack::Lit> literals;
		for (auto lit : assumptions) {
			literals.push(GHack::mkLit(lit.variable(), lit.is_complemented()));
		}

		GHack::lbool state = solver_->solveLimited(literals);
		if (state == GHack::l_True) {
			state_ = result::states::satisfiable;
		} else if (state == GHack::l_False) {
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
		return solver_->nVars();
	}

	uint32_t num_clauses() const
	{
		return solver_->nClauses();
	}
#pragma endregion

private:
	/*! \brief Backend solver */
	std::unique_ptr<solver_type> solver_;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::undefined;
};

} // namespace bill
