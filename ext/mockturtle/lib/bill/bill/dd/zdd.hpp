/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../utils/hash.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fmt/format.h>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace bill {

/*! \brief A zero-suppressed decision diagram (ZDD).
 *
 *  NOTE: This is a simple implementation. I would advise against its use when high-performance
 *        is a requirement.
 * 
 *  Limitations:
 *  	- The number of variables `N` must be known at instantiation time. 
 * 
 * Variables are numbered from `0` to `N - 1`.
 */

// TODO: Implement complemented edges
// TODO: Implement variable reordering
// TODO: Implement Variable order heuristics
// TODO: Implement Chain reduction
// TODO: Implement subsets operator
// TODO: Implement supersets operator
class zdd_base {
#pragma region Types and constructors
private:
	struct node_type {
		node_type(uint32_t var, uint32_t lo, uint32_t hi)
		    : marked(0)
		    , var(var)
		    , refs(0)
		    , lo(lo)
		    , hi(hi)
		{}

		uint32_t marked : 1;
		uint32_t var : 31;
		int32_t  refs; // Number of references - 1
		uint32_t lo;
		uint32_t hi;
	};

	enum operations : uint32_t {
		zdd_choose,
		zdd_difference,
		zdd_edivide,
		zdd_intersection,
		zdd_join,
		zdd_maximal,
		zdd_meet,
		zdd_nonsubsets,
		zdd_nonsupersets,
		zdd_union,
		num_operations
	};

public:
	using node_index = uint32_t;

	/* \!brief Creates a new ZDD base.
	 * 
	 * \param num_vars Number of variables
	 * \param log_num_objs Log number of nodes to pre-allocate (default: 16)
	 */
	explicit zdd_base(uint32_t num_vars, uint32_t log_num_objs = 16)
	    : unique_tables_(num_vars)
	    , num_dead_nodes_(0u)
	    , num_cache_lookups_(0u)
	    , num_cache_misses_(0u)
	{
		assert(num_variables() <= 4095);
		nodes_.reserve(1u << log_num_objs);
		nodes_.emplace_back(num_vars, 0, 0);
		nodes_.emplace_back(num_vars, 1, 1);
		build_elementary();
		build_tautologies();
	}
#pragma endregion

#pragma region ZDD base properties
public:
	/*! \brief Return the number of active nodes. */
	uint32_t num_nodes() const
	{
		return nodes_.size() - 2 - num_dead_nodes_ - free_nodes_.size();
	}

	/*! \brief Return the number of active nodes. */
	uint32_t num_variables() const
	{
		return unique_tables_.size();
	}
#pragma endregion

#pragma region ZDD base operations
private:

	/* \!brief Returns an unique node for the tuple (var, lo, hi)
	 *
	 * Given a variable `var` and node indexes lo and hi, we want to see if the ZDD base
	 * contains a node (var, lo, hi). If no such node exists, we create it. This function 
	 * returns a index to this _unique_ node. One crucial technicality should be noted:
	 * 
	 * /!\ This operation can potentially invalidate pointers, iterators and references /!\
	 * 
	 * Indexes are not invalidated.
	 */ 
	node_index unique(uint32_t var, node_index lo, node_index hi)
	{
		assert(var < num_variables());
		/* ZDD reduction rule */
		if (hi == bottom()) {
			--nodes_.at(hi).refs;
			return lo;
		}
		assert(nodes_.at(lo).var > var);
		assert(nodes_.at(hi).var > var);

		/* Unique table lookup */
		const auto it = unique_tables_.at(var).find({lo, hi});
		if (it != unique_tables_.at(var).end()) {
			if (nodes_.at(it->second).refs < 0) {
				--num_dead_nodes_;
				nodes_.at(it->second).refs = 0;
				return it->second;
			} else {
				--nodes_.at(lo).refs;
				--nodes_.at(hi).refs;
			}
			return ref(it->second);
		}

		/* Create new node */
		node_index new_node_index;
	restart:
		if (!free_nodes_.empty()) {
			new_node_index = free_nodes_.top();
			free_nodes_.pop();
			nodes_.at(new_node_index).marked = 0;
			nodes_.at(new_node_index).var = var;
			nodes_.at(new_node_index).refs = 0;
			nodes_.at(new_node_index).lo = lo;
			nodes_.at(new_node_index).hi = hi;
		} else {
			if (num_dead_nodes_ > num_nodes() / 8) {
				collect_garbage();
				goto restart;
			}
			new_node_index = nodes_.size();
			nodes_.emplace_back(var, lo, hi);
		} 
		unique_tables_.at(var)[{lo, hi}] = new_node_index;
		return new_node_index;
	}

	/* \! brief Recursively revives a dead, but unrecycled node
	 *
	 * When we discover that a node exists, but it is dead, i.e. all links to it have gone
	 * away, but we haven’t recycled it yet. We bring it back to life!
	 * 
	 * It increases the reference counts of the node's children, and resuscitates them if they
	 * were dead.
	 */
	void revive_node(node_index index)
	{
		assert(nodes_.at(index).refs < 0);
	restart:
		node_type& node = nodes_.at(index);
		node.refs = 0;
		--num_dead_nodes_;
		if (nodes_.at(node.lo).refs < 0) {
			revive_node(node.lo);
		} else {
			ref(node.lo);
		}
		if (nodes_.at(node.hi).refs < 0) {
			index = node.hi;
			goto restart;
		}
		ref(node.hi);
	}

	/* \!brief Recursively kills a node 
	 *
	 * When the reference count of a node reeaches -1, we kill it. It decreases the reference
	 * counts of the node's children, and kill them too(!) if necessary, i.e. their reference
	 * count reaches -1.
	 */
	void kill_node(node_index index)
	{
		assert(nodes_.at(index).refs == 0);
	restart:
		node_type& node = nodes_.at(index);
		node.refs = -1;
		++num_dead_nodes_;
		if (nodes_.at(node.lo).refs == 0) {
			kill_node(node.lo);
		} else {
			--nodes_.at(node.lo).refs;
		}
		if (nodes_.at(node.hi).refs == 0) {
			index = node.hi;
			goto restart;
		}
		--nodes_.at(node.hi).refs;
	}

	/* \!brief Return the tautology function */
	node_index tautology(uint32_t var)
	{
		if (var == num_variables()) {
			return top();
		}
		return (2 * num_variables()) + 1u - var;
	}

	void cache_cleanup()
	{
		auto check_nodes = [&](node_index a, node_index b, node_index c) -> bool {
			if (nodes_.at(a).refs < 0) {
				return true;
			}
			if (nodes_.at(b).refs < 0) {
				return true;
			}
			return nodes_.at(c).refs < 0;
		};
		for (auto& table : computed_tables_) {
			for (auto it = table.begin(); it != table.end();) {
				auto [index_f, index_g] = it->first;
				if (check_nodes(it->second, index_f, index_g)) {
					it = table.erase(it);
					continue;
				} else {
					++it;
				}
			}
		}
	}

	void tables_cleanup()
	{
		/* Skip terminals and elementary nodes */
		uint32_t const begin = (2 * num_variables()) + 2;
		for (uint32_t index = begin; index < nodes_.size(); ++index) {
			node_type& node = nodes_.at(index);
			if (node.refs >= 0) {
				continue;
			}
			auto const it = unique_tables_.at(node.var).find({node.lo, node.hi});
			assert(it != unique_tables_.at(node.var).end());
			unique_tables_.at(node.var).erase(it);
			free_nodes_.push(index);
			node.refs = 0;
		}
	}

	/*! \brief Creates a node at each level that means "tautology from here on" */
	void build_tautologies()
	{
		assert(nodes_.size() == num_variables() + 2u);
		node_index last = top();
		for (int var = num_variables() - 1; var >= 0; --var) {
			ref(last, 2);
			last = unique(var, last, last);
			assert(last == (2 * num_variables()) + 1u - var);
			if (var != 0) {
				--nodes_.at(last).refs;
			}
		}
	}

	/*! \brief Create nodes corresponding to the elementary families */
	void build_elementary()
	{
		for (auto var = 0u; var < num_variables(); ++var) {
			unique(var, bottom(), top());
			ref(bottom());
			ref(top());
		};
		--nodes_.at(bottom()).refs;
		--nodes_.at(top()).refs;
	}

public:
	/*! \brief Returns the node index corresponding to the `empty family` */
	node_index bottom() const
	{
		return 0u;
	}

	/*! \brief Returns the node index corresponding to the `unit family` */
	node_index top() const
	{
		return 1u;
	}

	/*! \brief Returns the node-id corresponding to the elementary family `{{var}}` */
	node_index elementary(uint32_t var)
	{
		assert(var < num_variables());
		return var + 2u;
	}

	/*! \brief Increase the reference count of a node. */
	node_index ref(node_index index, int32_t i = 1)
	{
		assert(index < nodes_.size());
		nodes_.at(index).refs += i;
		return index;
	}

	/*! \brief Decrease the reference count of a node. */
	void deref(node_index index)
	{
		assert(index < nodes_.size());
		assert(nodes_.at(index).refs >= 0);
		if (nodes_.at(index).refs == 0) {
			kill_node(index);
			return;
		}
		--nodes_.at(index).refs;
	}

	/*! \brief Recycle all the dead nodes */
	void collect_garbage()
	{
		cache_cleanup();
		tables_cleanup();
		num_dead_nodes_ = 0;
	}
#pragma endregion

#pragma region ZDD Operations
public:
	/* \!brief Computes the family of all ``k``-combinations of a ZDD.  */
	node_index choose(node_index index_f, uint32_t k)
	{
		constexpr operations op = operations::zdd_choose;
		if (index_f <= top()) {
			return k > 0 ? bottom() : top();
		}
		if (k == 1) {
			return ref(index_f);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, k});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_index index_new = choose(node_f.lo, k);
		if (k > 0) {
			node_index temp = choose(node_f.lo, k - 1);
			ref(index_new);
			index_new = unique(node_f.var, index_new, temp);
		} else {
			deref(index_new);
		}
		computed_tables_.at(op)[{index_f, k}] = index_new;
		return index_new;
	}

	/* \!brief Computes the difference of two ZDDs (`f - g`)
	 *  Keep in mind that `f - g` is different from `g - f` !
	 */
	node_index difference(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_difference;
		if (index_f == bottom()) {
			return ref(bottom());
		}
		node_type& node_f = nodes_.at(index_f);
	
	restart:
		if (index_f == index_g) {
			return ref(bottom());
		}
		if (index_g == bottom()) {
			return ref(index_f);
		}
		node_type& node_g = nodes_.at(index_g);
		if (node_g.var < node_f.var) {
			index_g = node_g.lo;
			goto restart;
		} 

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_index r_lo;
		node_index r_hi;
		if (node_f.var == node_g.var) {
			r_lo = difference(node_f.lo, node_g.lo);
			r_hi = difference(node_f.hi, node_g.hi);
		} else {
			r_lo = difference(node_f.lo, index_g);
			r_hi = ref(node_f.hi);
		}
		node_index index_new = unique(node_f.var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Computes the intersection of two ZDDs */
	node_index intersection(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_intersection;
		if (index_f == tautology()) {
			return ref(index_g); 
		}
		if (index_g == tautology()) {
			return ref(index_f); 
		}
	restart:
		if (index_f > index_g) {
			std::swap(index_f, index_g);
		}
		if (index_f == bottom()) {
			return ref(bottom());
		}
		if (index_f == index_g) {
			return ref(index_f);
		}

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		if (node_f.var <node_g.var) {
			index_f = node_f.lo;
			goto restart;
		} else if (node_f.var > node_g.var) {
			index_g = node_g.lo;
			goto restart;
		}
		if (index_f == tautology(node_f.var)) {
			return ref(index_g); 
		}
		if (index_g == tautology(node_g.var)) {
			return ref(index_f); 
		}
		assert(node_f.var == node_g.var);

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_index r_lo = intersection(node_f.lo, node_g.lo);
		node_index r_hi = intersection(node_f.hi, node_g.hi);
		node_index index_new = unique(node_f.var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Computes the join of two ZDDs */
	node_index join(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_join;
		if (index_f > index_g) {
			std::swap(index_f, index_g);
		}
		if (index_f == bottom()) {
			return ref(bottom());
		}
		if (index_f == top()) {
			return ref(index_g);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		node_index r_lo;
		node_index r_hi;
		uint32_t var = node_f.var;
		if (node_f.var < node_g.var) {
			r_lo = join(node_f.lo, index_g);
			r_hi = join(node_f.hi, index_g);
		} else if (node_f.var > node_g.var) {
			r_lo = join(node_g.lo, index_f);
			r_hi = join(node_g.hi, index_f);
			var = node_g.var;
		} else {
			// In this case node_f.var == node_g.var
			r_lo = union_(node_g.lo, node_g.hi);
			node_index const r_hl = join(node_f.hi, r_lo);
			deref(r_lo);
			node_index const r_lh = join(node_f.lo, node_g.hi);
			r_hi = union_(r_hl, r_lh);
			deref(r_hl);
			deref(r_lh);
			r_lo = join(node_f.lo, node_g.lo);
		}
		node_index index_new = unique(var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Computes the maximal of a ZDD */
	node_index maximal(node_index index_f)
	{
		constexpr operations op = operations::zdd_maximal;
		if (index_f <= top()) {
			return ref(index_f);
		}
		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, 0});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_index r_hi = maximal(node_f.hi);
		node_index temp = maximal(node_f.lo);
		node_index r_lo = nonsubsets(temp, r_hi);
		deref(temp);
		node_index index_new = unique(node_f.var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, 0}] = index_new;
		return index_new;;
	}

	/* \!brief Computes the meet of two ZDDs */
	node_index meet(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_meet;
		if (index_f > index_g) {
			std::swap(index_f, index_g);
		}
		if (index_f <= top()) {
			return ref(index_f);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		node_index r_lo;
		node_index r_hi;
		if (node_f.var < node_g.var) {
			r_lo = union_(node_f.lo, node_f.hi);
			r_hi = meet(r_lo, index_g);
			deref(r_lo);
			return r_hi;
		} else if (node_f.var > node_g.var) {
			r_lo = union_(node_g.lo, node_g.hi);
			r_hi = meet(r_lo, index_f);
			deref(r_lo);
			return r_hi;
		} else { 
			// In this case node_f.var == node_g.var
			r_hi = union_(node_f.lo, node_f.hi);
			node_index r_hl = meet(r_hi, node_g.lo);
			deref(r_hi);
			node_index r_lh = meet(node_f.lo, node_g.hi);
			r_lo = union_(r_hl, r_lh);
			deref(r_hl);
			deref(r_lh);
			r_hi = meet(node_f.hi, node_g.hi);
		}
		node_index index_new = unique(node_f.var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Computes the nonsubsets of two ZDDs */
	node_index nonsubsets(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_nonsubsets;
		if (index_g == bottom()) {
			return ref(index_f);
		}
		if (index_f <= top()) {
			return ref(bottom());
		}
		if (index_f == index_g) {
			return ref(bottom());
		}

		if (nodes_.at(index_f).var > nodes_.at(index_g).var) {
			return nonsubsets(index_f, nodes_.at(index_g).lo);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		node_index r_lo;
		node_index r_hi;
		if (node_f.var < node_g.var) {
			r_lo = nonsubsets(node_f.lo, index_g);
			r_hi = ref(node_f.hi);
		} else {
			node_index const temp = nonsubsets(node_f.lo, node_g.hi);
			r_hi = nonsubsets(node_f.lo, node_g.lo);
			r_lo = intersection(temp, r_hi);
			deref(temp);
			deref(r_hi);
			r_hi = nonsubsets(node_f.hi, node_g.hi);
		}
		node_index index_new = unique(node_f.var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Computes the nonsupersets of two ZDDs */
	node_index nonsupersets(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_nonsupersets;
		if (index_g == bottom()) {
			return ref(index_f);
		}
		if (index_f == bottom()) {
			return ref(bottom());
		}
		// This operation can be potentially faster if instead I check for top \in g
		// TODO: experiment!
		if (index_g == top()) {
			return ref(bottom());
		}
		if (index_f == index_g) {
			return ref(bottom());
		}

		if (nodes_.at(index_f).var > nodes_.at(index_g).var) {
			return nonsupersets(index_f, nodes_.at(index_g).lo);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		node_index r_lo;
		node_index r_hi;
		uint32_t var = node_f.var;
		if (node_f.var < node_g.var) {
			r_lo = nonsupersets(node_f.lo, index_g);
			r_hi = nonsupersets(node_f.hi, index_g);
		} else {
			r_lo = nonsupersets(node_f.hi, node_g.hi);
			node_index temp = nonsupersets(node_f.hi, node_g.lo);
			r_hi = intersection(temp, r_lo);
			deref(temp);
			deref(r_lo);
			r_lo = nonsupersets(node_f.lo, node_g.lo);
		}
		node_index index_new = unique(var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}

	/* \!brief Return the tautology function */
	node_index tautology()
	{
		return (2 * num_variables()) + 1u;
	}

	/* \!brief Computes the union of two ZDDs */
	node_index union_(node_index index_f, node_index index_g)
	{
		constexpr operations op = operations::zdd_union;
		if (index_f == index_g) {
			return ref(index_f);
		}
		if (index_f > index_g) {
			std::swap(index_f, index_g);
		}
		if (index_f == bottom()) {
			return ref(index_g);
		}

		// Cache lookup
		++num_cache_lookups_;
		const auto it = computed_tables_.at(op).find({index_f, index_g});
		if (it != computed_tables_.at(op).end()) {
			if (nodes_.at(it->second).refs < 0) {
				revive_node(it->second);
				return it->second;
			}
			return ref(it->second);
		}
		++num_cache_misses_;;

		node_type node_f = nodes_.at(index_f);
		node_type node_g = nodes_.at(index_g);
		node_index r_lo;
		node_index r_hi;
		uint32_t var = node_f.var;
		if (node_f.var < node_g.var) {
			if (index_f == tautology(node_f.var)) {
				return ref(index_f); 
			}
			r_lo = union_(node_f.lo, index_g);
			r_hi = ref(node_f.hi);
		} else if (node_f.var > node_g.var) {
			if (index_g == tautology(node_g.var)) {
				return ref(index_g); 
			}
			r_lo = union_(index_f, node_g.lo);
			r_hi = ref(node_g.hi);
			var = node_g.var;
		} else {
			// In this case node_f.var == node_g.var
			if (index_g == tautology(node_g.var)) {
				return ref(index_g); 
			}
			r_lo = union_(node_f.lo, node_g.lo);
			r_hi = union_(node_f.hi, node_g.hi);
		}
		node_index index_new = unique(var, r_lo, r_hi);
		computed_tables_.at(op)[{index_f, index_g}] = index_new;
		return index_new;
	}
#pragma endregion

#pragma region ZDD iterators
private:
	template<class Fn>
	bool foreach_set_rec(node_index index, std::vector<uint32_t>& set, Fn&& fn) const
	{
		if (index == 1u) {
			return fn(set);
		}
		if (index != 0u) {
			if (!foreach_set_rec(nodes_.at(index).lo, set, fn)) {
				return false;
			}
			auto new_set = set;
			new_set.push_back(nodes_.at(index).var);
			if (!foreach_set_rec(nodes_.at(index).hi, new_set, fn)) {
				return false;
			}
		}
		return true;
	}

public:
	template<class Fn>
	void foreach_set(node_index index, Fn&& fn) const
	{
		std::vector<uint32_t> set;
		foreach_set_rec(index, set, fn);
	}
#pragma endregion

#pragma region ZDD properties
private:
	void count_nodes_rec(node_index index, std::unordered_set<node_index>& visited) const
	{
		if (index <= 1 || visited.count(index)) {
			return;
		}
		visited.insert(index);
		node_type const& node = nodes_.at(index);
		count_nodes_rec(node.lo, visited);
		count_nodes_rec(node.hi, visited);
	}

	uint64_t count_sets_rec(node_index index, std::unordered_map<node_index, uint64_t>& visited) const
	{
		if (index <= 1) {
			return index;
		}
		const auto it = visited.find(index);
		if (it != visited.end()) {
			return it->second;
		}
		node_type const& node = nodes_.at(index);
		return visited[index] = count_sets_rec(node.lo, visited)
		                      + count_sets_rec(node.hi, visited);
	}

public:
	/* \!brief Return the number of nodes in a ZDD. */
	uint64_t count_nodes(node_index index_root) const
	{
		if (index_root <= 1) {
			return 0;
		}
		std::unordered_set<node_index> visited;
		count_nodes_rec(index_root, visited);
		return visited.size();
	}

	/* \!brief Return the number of sets in a ZDD. */
	uint64_t count_sets(node_index index_root) const
	{
		if (index_root <= 1) {
			return index_root;
		}
		std::unordered_map<node_index, uint64_t> visited;
		return count_sets_rec(index_root, visited);
	}

	std::vector<std::vector<uint32_t>> sets_as_vectors(node_index index) const
	{
		std::vector<std::vector<uint32_t>> sets_vectors;
		foreach_set(index, [&](auto const& set){
			sets_vectors.emplace_back(set);
			return true;
		});
		return sets_vectors;
	}
#pragma endregion

#pragma region Debug
public:
	void print_debug(std::ostream& os = std::cout) const
	{
		os << "ZDD nodes:\n";
		os << "    i     VAR    LO    HI   REF\n";
		uint32_t i = 0u;
		for (node_type const& node : nodes_) {
			os << fmt::format("{:5} : {:5} {:5} {:5} {:5}\n", i++, node.var, node.lo,
			                  node.hi, node.refs);
		}
	}

	void print_sets(node_index index, std::ostream& os = std::cout) const
	{
		foreach_set(index, [&](auto const& set){
			os << fmt::format("{{ {} }}\n", fmt::join(set, ", "));
			return true;
		});
	}
#pragma endregion

private:
	using children_type = std::pair<node_index, node_index>;
	using unique_table_type = std::unordered_map<children_type, node_index>;

	std::vector<node_type> nodes_;
	std::stack<node_index> free_nodes_;
	std::vector<unique_table_type> unique_tables_;
	std::array<unique_table_type, operations::num_operations> computed_tables_;

	// Stats
	uint32_t num_dead_nodes_;
	uint32_t num_cache_lookups_;
	uint32_t num_cache_misses_;
};

} // namespace bill
