/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../utils/platforms.hpp"

#if defined(BILL_WINDOWS_PLATFORM)
#pragma warning(push)
#pragma warning( \
    disable : 4018 4127 4189 4200 4242 4244 4245 4305 4365 4388 4389 4456 4457 4459 4514 4552 4571 4583 4619 4623 4625 4626 4706 4710 4711 4774 4820 4820 4996 5026 5027 5039)
#include "../solver/ghack.hpp"
#include "../solver/glucose.hpp"
#define ABC_USE_NAMESPACE pabc
#define ABC_NAMESPACE pabc
#define ABC_USE_NO_READLINE
#include "../solver/abc.hpp"
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-comparison"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#include "../solver/ghack.hpp"
#include "../solver/glucose.hpp"
#include "../solver/maple.hpp"
#ifndef LIN64
#define LIN64
#endif
#define ABC_USE_NAMESPACE pabc
#define ABC_NAMESPACE pabc
#define ABC_USE_NO_READLINE
#include "../solver/abc.hpp"
#pragma GCC diagnostic pop
#endif

#include "types.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace bill {

class result {
public:
	using model_type = std::vector<lbool_type>;
	using clause_type = std::vector<lit_type>;

	enum class states : uint8_t {
		satisfiable,
		unsatisfiable,
		undefined,
		timeout,
		dirty,
	};

	static std::string to_string(states const& state)
	{
		switch (state) {
		case states::satisfiable:
			return "satisfiable";
		case states::unsatisfiable:
			return "unsatisfiable";
		case states::timeout:
			return "timeout";
		case states::dirty:
			return "dirty";
		case states::undefined:
		default:
			return "undefined";
		}
	}

#pragma region Constructors
	result(states state = states::undefined)
	    : state_(state)
	{}

	result(model_type const& model)
	    : state_(states::satisfiable)
	    , data_(model)
	{}

	result(clause_type const& unsat_core)
	    : state_(states::unsatisfiable)
	    , data_(unsat_core)
	{}
#pragma endregion

#pragma region Properties
	inline bool is_satisfiable() const
	{
		return (state_ == states::satisfiable);
	}

	inline bool is_unsatisfiable() const
	{
		return (state_ == states::unsatisfiable);
	}

	inline bool is_undefined() const
	{
		return (state_ == states::undefined);
	}

	inline model_type model() const
	{
		return std::get<model_type>(data_);
	}

	inline clause_type core() const
	{
		return std::get<clause_type>(data_);
	}
#pragma endregion

#pragma region Overloads
	inline operator bool() const
	{
		return (state_ == states::satisfiable);
	}

	inline explicit operator std::string() const
	{
		return result::to_string(state_);
	}
#pragma endregion

private:
	states state_;
	std::variant<model_type, clause_type> data_;
};

enum class solvers {
	glucose_41,
	ghack,
	bsat2,
#if !defined(BILL_WINDOWS_PLATFORM)
	maple,
	bmcg,
#endif
#if defined(BILL_HAS_Z3)
	z3,
#endif
};

/*! \brief Solver interface
 */
template<solvers Solver = solvers::ghack>
class solver;

} // namespace bill
