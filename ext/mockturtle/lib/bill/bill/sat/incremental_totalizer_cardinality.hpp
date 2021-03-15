/*
 * This implementation is based on the code of Antonio Morgado and
 * Alexey S. Ignatiev [1].  See [2] for a seminal reference.
 *
 * [1] https://github.com/pysathq/pysat/blob/master/cardenc/itot.hh.
 * [2] Ruben Martins, Saurabh Joshi, Vasco M. Manquinho, Inês Lynce:
 *     Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
 */
#pragma once

#include "interface/types.hpp"

#include <deque>

namespace bill {

struct totalizer_tree {
	std::vector<lit_type> vars;
	uint32_t num_inputs;
	std::shared_ptr<totalizer_tree> left;
	std::shared_ptr<totalizer_tree> right;
}; /* totalizer_tree */

namespace detail {

template<typename Solver>
inline void create_totalizer_internal(Solver& solver, std::vector<std::vector<lit_type>>& dest,
                                      std::vector<lit_type> const& ov, uint32_t rhs,
                                      std::vector<lit_type> const& av,
                                      std::vector<lit_type> const& bv)
{
	(void) solver;

	/* i = 0 */
	uint32_t kmin = std::min(rhs, uint32_t(bv.size()));
	for (auto j = 0u; j < kmin; ++j) {
		dest.emplace_back(std::vector{~bv[j], ov[j]});
	}

	/* j = 0 */
	kmin = std::min(rhs, uint32_t(av.size()));
	for (auto i = 0u; i < kmin; ++i) {
		dest.emplace_back(std::vector{~av[i], ov[i]});
	}

	/* i, j > 0 */
	for (auto i = 1u; i <= kmin; ++i) {
		auto const min_j = std::min(rhs - i, uint32_t(bv.size()));
		for (auto j = 1u; j <= min_j; ++j) {
			dest.emplace_back(std::vector{~av[i - 1], ~bv[j - 1], ov[i + j - 1]});
		}
	}
}

template<typename Solver>
inline void increase_totalizer_internal(Solver& solver, std::vector<std::vector<lit_type>>& dest,
                                        std::vector<lit_type>& ov, uint32_t rhs,
                                        std::vector<lit_type>& av, std::vector<lit_type>& bv)
{
	uint32_t last = ov.size();
	for (auto i = last; i < rhs; ++i) {
		ov.emplace_back(lit_type(solver.add_variable(), positive_polarity));
	}

	// add the constraints
	/* i = 0 */
	uint32_t const max_j = std::min(rhs, uint32_t(bv.size()));
	for (auto j = last; j < max_j; ++j) {
		dest.emplace_back(std::vector{~bv[j], ov[j]});
	}

	/* j = 0 */
	uint32_t const max_i = std::min(rhs, uint32_t(av.size()));
	for (auto i = last; i < max_i; ++i) {
		dest.emplace_back(std::vector{~av[i], ov[i]});
	}

	/* i, j > 0 */
	for (auto i = 1u; i <= max_i; ++i) {
		auto const max_j = std::min(rhs - i, uint32_t(bv.size()));
		auto const min_j = uint32_t(std::max(int(last) - int(i) + 1, 1));
		for (auto j = min_j; j <= max_j; ++j) {
			dest.emplace_back(std::vector{~av[i - 1], ~bv[j - 1], ov[i + j - 1]});
		}
	}
}

} // namespace detail

template<typename Solver>
inline std::shared_ptr<totalizer_tree> create_totalizer(Solver& solver,
                                                        std::vector<std::vector<lit_type>>& dest,
                                                        std::vector<lit_type> const& lhs,
                                                        uint32_t rhs)
{
	auto const n = lhs.size();

	std::deque<std::shared_ptr<totalizer_tree>> queue;
	for (auto i = 0u; i < n; ++i) {
		auto t = std::make_shared<totalizer_tree>();
		t->vars.resize(1, lit_type(lhs[i].variable(), lhs[i].polarity()));
		t->num_inputs = 1;
		queue.push_back(t);
	}

	while (queue.size() > 1) {
		auto const le = queue.front();
		queue.pop_front();

		auto const ri = queue.front();
		queue.pop_front();

		auto t = std::make_shared<totalizer_tree>();
		t->num_inputs = le->num_inputs + ri->num_inputs;
		t->left = le;
		t->right = ri;

		uint32_t kmin = std::min(rhs + 1, t->num_inputs);
		t->vars.resize(kmin, lit_type(var_type(0), negative_polarity));
		for (auto i = 0u; i < kmin; ++i) {
			t->vars[i] = lit_type(solver.add_variable(), positive_polarity);
		}
		detail::create_totalizer_internal(solver, dest, t->vars, kmin, le->vars, ri->vars);
		queue.push_back(t);
	}
	return queue.front();
}

template<typename Solver>
inline void increase_totalizer(Solver& solver, std::vector<std::vector<lit_type>>& dest,
                               std::shared_ptr<totalizer_tree>& t, uint32_t rhs)
{
	uint32_t const kmin = std::min(rhs + 1, t->num_inputs);
	if (kmin <= t->vars.size())
		return;

	increase_totalizer(solver, dest, t->left, rhs);
	increase_totalizer(solver, dest, t->right, rhs);
	detail::increase_totalizer_internal(solver, dest, t->vars, kmin, t->left->vars,
	                                    t->right->vars);
}

template<typename Solver>
inline std::shared_ptr<totalizer_tree> merge_totalizer(Solver& solver,
                                                       std::vector<std::vector<lit_type>>& dest,
                                                       std::shared_ptr<totalizer_tree>& ta,
                                                       std::shared_ptr<totalizer_tree>& tb,
                                                       uint32_t rhs)
{
	increase_totalizer(solver, dest, ta, rhs);
	increase_totalizer(solver, dest, tb, rhs);

	uint32_t n = ta->num_inputs + tb->num_inputs;
	uint32_t kmin = std::min(rhs, n);

	auto t = std::make_shared<totalizer_tree>();
	t->num_inputs = n;
	t->left = ta;
	t->right = tb;

	t->vars.resize(kmin, lit_type(var_type(0), negative_polarity));
	for (auto i = 0u; i < kmin; ++i) {
		t->vars[i] = lit_type(solver.add_variable(), positive_polarity);
	}

	detail::create_totalizer_internal(solver, dest, t->vars, kmin, ta->vars, tb->vars);
	return t;
}

template<typename Solver>
inline std::shared_ptr<totalizer_tree> extend_totalizer(Solver& solver,
                                                        std::vector<std::vector<lit_type>>& dest,
                                                        std::shared_ptr<totalizer_tree>& ta,
                                                        std::vector<lit_type> const& lhs,
                                                        uint32_t rhs)
{
	auto tb = create_totalizer(solver, dest, lhs, rhs);
	return merge_totalizer(solver, dest, ta, tb, rhs);
}

} // namespace bill
