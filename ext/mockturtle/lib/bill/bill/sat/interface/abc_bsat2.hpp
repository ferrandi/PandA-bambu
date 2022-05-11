/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "common.hpp"
#include "types.hpp"

#include <memory>
#include <random>
#include <variant>
#include <vector>

namespace bill {

template<>
class solver<solvers::bsat2> {
	using solver_type = pabc::sat_solver;

public:
#pragma region Constructors
	solver()
	    : variable_counter(1, 0u)
	    , clause_counter(1, 0)
	{
		solver_ = pabc::sat_solver_new();
	}

	~solver()
	{
		pabc::sat_solver_delete(solver_);
		solver_ = nullptr;
	}

	/* disallow copying */
	solver(solver<solvers::bsat2> const&) = delete;
	solver<solvers::bsat2>& operator=(const solver<solvers::bsat2>&) = delete;
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		pabc::sat_solver_restart(solver_);
		state_ = result::states::undefined;
		randomize = false;
		variable_counter.clear();
		variable_counter.emplace_back(0u);
		clause_counter.clear();
		clause_counter.emplace_back(0);
	}

	var_type add_variable()
	{
		++variable_counter.back();
		return pabc::sat_solver_addvar(solver_);
	}

	void add_variables(uint32_t num_variables = 1)
	{
		variable_counter.back() += num_variables;
		for (auto i = 0u; i < num_variables; ++i) {
			pabc::sat_solver_addvar(solver_);
		}
	}

	auto add_clause(std::vector<lit_type>::const_iterator it,
	                std::vector<lit_type>::const_iterator ie)
	{
		literals.resize(ie - it);
		++clause_counter.back();
		auto counter = 0u;
		while (it != ie) {
			literals[counter++] = pabc::Abc_Var2Lit(it->variable(),
			                                        it->is_complemented());
			++it;
		}
		auto const result = pabc::sat_solver_addclause(solver_, literals.data(), literals.data() + counter);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		return add_clause(clause.begin(), clause.end());
	}

	auto add_clause(lit_type lit)
	{
		--clause_counter.back(); /* do not count unit clauses */
		return add_clause(std::vector<lit_type>{lit});
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0u; i < num_variables(); ++i) {
			auto const value = pabc::sat_solver_var_value(solver_, i);
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

		if (randomize) {
			std::vector<uint32_t> vars;
			for (auto i = 0u; i < num_variables(); ++i) {
				if (random() % 2) {
					vars.push_back(i);
				}
			}
			pabc::sat_solver_set_polarity(solver_,
			                              (int*) (const_cast<uint32_t*>(vars.data())),
			                              vars.size());
		}

		int result;
		if (assumptions.size() > 0u) {
			/* solve with assumptions */
			uint32_t counter = 0u;
			literals.resize(assumptions.size());
			auto it = assumptions.begin();
			while (it != assumptions.end()) {
				literals[counter++] = pabc::Abc_Var2Lit(it->variable(),
				                                        it->is_complemented());
				++it;
			}
			result = pabc::sat_solver_solve(solver_, literals.data(), literals.data() + counter,
			                                conflict_limit, 0, 0, 0);
		} else {
			/* solve without assumptions */
			result = pabc::sat_solver_solve(solver_, 0, 0, conflict_limit, 0, 0, 0);
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
		return variable_counter.back();
		/* Note: `pabc::sat_solver_nvars(solver_)` is not correct when bookmark/rollback is used */
	}

	uint32_t num_clauses() const
	{
		return clause_counter.back();
		/* Note: `pabc::sat_solver_nclauses(solver_)` is not correct when bookmark/rollback is used */
	}
#pragma endregion

	void push()
	{
		pabc::sat_solver_bookmark(solver_);
		variable_counter.emplace_back(variable_counter.back());
		clause_counter.emplace_back(clause_counter.back());
	}

	void pop(uint32_t num_levels = 1u)
	{
		assert(num_levels == 1u && "bsat does not support multiple step pop");
		assert(variable_counter.size() >= num_levels);
		assert(clause_counter.size() >= num_levels);
		pabc::sat_solver_rollback(solver_);
		variable_counter.resize(uint32_t(variable_counter.size() - num_levels));
		clause_counter.resize(uint32_t(clause_counter.size() - num_levels));
	}

	void set_random_phase(uint32_t seed = 0u)
	{
		randomize = true;
		pabc::sat_solver_set_random(solver_, 1);
		random.seed(seed);
	}

private:
	/*! \brief Backend solver */
	solver_type* solver_ = nullptr;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::undefined;

	/*! \brief Temporary storage for one clause */
	std::vector<pabc::lit> literals;

	/*! \brief Whether to randomize initial variable values */
	bool randomize = false;
	std::default_random_engine random;

	/*! \brief Stacked counter for number of variables */
	std::vector<uint32_t> variable_counter;

	/*! \brief Stacked counter for number of clauses */
	std::vector<int> clause_counter;
};

} // namespace bill
