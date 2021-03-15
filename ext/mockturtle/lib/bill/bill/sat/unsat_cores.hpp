#pragma once

namespace bill {

namespace detail {

template<typename T>
inline std::vector<T> copy_vector_without_index(std::vector<T> const& vs, uint32_t index)
{
	assert(index < vs.size());
	std::vector<T> copy(vs);
	copy.erase(std::begin(copy) + index);
	return copy;
}

} // namespace detail

template<typename Solver>
inline result::clause_type trim_core_copy(Solver& solver, result::clause_type const& core,
                                          uint32_t num_tries = 8u)
{
	auto current = core;

	uint32_t counter = 0u;
	while (counter++ < num_tries && solver.solve(current) == result::states::unsatisfiable) {
		auto const new_core = solver.get_core().core();
		if (new_core.size() == current.size())
			break;

		current = new_core;
	}

	return current;
}

template<typename Solver>
inline void trim_core(Solver& solver, result::clause_type& core, uint32_t num_tries = 0u)
{
	core = trim_core_copy(solver, core, num_tries);
}

template<typename Solver>
inline result::clause_type minimize_core_copy(Solver& solver, result::clause_type& core,
                                              int64_t budget = 1000)
{
	auto pos = 0u;
	auto current = core;

	while (pos < current.size()) {
		auto temp = detail::copy_vector_without_index(current, pos);

		auto result = solver.solve(temp, budget);
		if (result == result::states::unsatisfiable) {
			current = temp;
		} else {
			++pos;
		}
	}

	if (current.size() < core.size()) {
		return current;
	} else {
		return core;
	}
}

template<typename Solver>
inline void minimize_core(Solver& solver, result::clause_type& core, int64_t budget = 1000)
{
	core = minimize_core_copy(solver, core, budget);
}

} // namespace bill
