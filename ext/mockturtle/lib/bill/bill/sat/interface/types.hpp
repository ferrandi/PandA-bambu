/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

namespace bill {

/*! \brief Wrapper class to represent variables.
 *
 * A variable is an element of a convenient set.  They are often identified by symbols such as 
 * x1, x2, ..., xn; of course, any other symbol can also be used, e.g., a, b, c.  In code, however,
 * we use unsigned numerals 1, 2, 3, ..., n that stand for variables.
 * 
 * Because of its relation to literals (see below), using `uint32_t` to hold variable identifiers
 * limits the number of possibles variables to 2^31 - 1 = 2,147,483,647.
 */
class var_type {
	constexpr static uint32_t max_value = (std::numeric_limits<uint32_t>::max() >> 1);

public:
	constexpr var_type(uint32_t var = 0)
	    : data_(var)
	{
		assert(var < max_value);
	}

#pragma region Overloads
	constexpr operator uint32_t() const
	{
		return data_;
	}

	bool operator<(var_type other) const
	{
		return data_ < other.data_;
	}

	bool operator==(var_type other) const
	{
		return data_ == other.data_;
	}

	bool operator!=(var_type other) const
	{
		return data_ != other.data_;
	}
#pragma endregion

private:
	uint32_t data_;
};

/*! \brief Wrapper class to represent literals.
 *
 * A literal is either a variable or the complement of a variable. In other words, if x1 is a
 * variable, both x1 and ~x1 are literals. If there are n possible variables in some problem, there
 * are 2n possible literals. We call x1 and ~x1 the positive polarity literal and negative polarity
 * literal of x1, respectively.
 *
 * We also use unsigned numerals to represent literals (though we could have used singed numerals
 * and use the sign to represent each polarity). When using unsigned numerals, even numerals
 * represent positive polarity and odd numerals represent negative polarity.
 *
 * Using `uint32_t` to hold literals identifiers limits the number of possible literals to
 * 2^32 - 1 = 4,294,967,295.
 */
class lit_type {
public:
	enum class polarities : bool {
		positive = 0,
		negative = 1,
	};

	constexpr lit_type(var_type var = {}, polarities polarity = polarities::positive)
	    : data_((var << 1) | ((polarity == polarities::positive) ? 0 : 1))
	{}

#pragma region Properties
	var_type variable() const
	{
		return (data_ >> 1);
	}

	polarities polarity() const
	{
		return polarities((data_ & 1) == 1);
	}

	bool is_complemented() const
	{
		return (data_ & 1) == 1;
	}
#pragma endregion

#pragma region Modifiers
	void complement()
	{
		data_ ^= 1;
	}
#pragma endregion

#pragma region Overloads
	lit_type operator~() const
	{
		lit_type complemented(*this);
		complemented.data_ ^= 1;
		return complemented;
	}

	bool operator<(lit_type other) const
	{
		return data_ < other.data_;
	}

	bool operator==(lit_type other) const
	{
		return data_ == other.data_;
	}

	bool operator!=(lit_type other) const
	{
		return data_ != other.data_;
	}
#pragma endregion

private:
	uint32_t data_;
};

constexpr auto positive_polarity = lit_type::polarities::positive;
constexpr auto negative_polarity = lit_type::polarities::negative;

/*! \brief Lifted Boolean wrapper class.
 */
enum class lbool_type : uint8_t {
	true_,
	false_,
	undefined,
};

} // namespace bill
