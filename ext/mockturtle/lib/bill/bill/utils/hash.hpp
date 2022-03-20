/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <set>
#include <utility>


// TODO: this is a bit of a hack!

namespace std {

template<class T>
inline void hash_combine(std::size_t& seed, T const& v)
{
	seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T>
struct hash<set<T>> {
	using argument_type = set<T>;
	using result_type = size_t;
	result_type operator()(argument_type const& in) const
	{
		result_type seed = 0;
		for (auto& element : in) {
			hash_combine(seed, element);
		}
		return seed;
	}
};

template<>
struct hash<pair<uint32_t, uint32_t>> {
	using argument_type = pair<uint32_t, uint32_t>;
	using result_type = size_t;
	result_type operator()(argument_type const& in) const
	{
		result_type seed = 0;
		hash_combine(seed, in.first);
		hash_combine(seed, in.second);
		return seed;
	}
};

} // namespace std
