/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __GENERIC_H__
#define __GENERIC_H__

#include "bitty.h"
#include <algorithm>
#include <deque>
#include <functional>

/*
** {===========================================================================
** Generic
**
** @note The principle of these templates is to help writing code in a handier
**   way, for workspaces, editors, etc.
*/

/**
 * @brief Comparison utilities.
 */
namespace Compare {

/**
 * @brief Compares two iterables lexicographically.
 */
template<typename Left, typename Right, typename Cmp> int lex(Left left0, Left left1, Right right0, Right right1, Cmp cmp) {
	while (left0 != left1 && right0 != right1) {
		const int ret = cmp(*left0, *right0);
		if (ret < 0)
			return -1;
		else if (ret > 0)
			return 1;

		++left0;
		++right0;
	}

	if (left0 == left1 && right0 != right1)
		return -1;
	else if (left0 != left1 && right0 == right1)
		return 1;

	return 0;
}

/**
 * @brief Compares two iterables documentally.
 */
template<typename Left, typename Right, typename Cmp> int doc(Left left0, Left left1, Right right0, Right right1, Cmp cmp) {
	while (left0 != left1 && right0 != right1) {
		if (left0 + 1 != left1 && right0 + 1 == right1)
			return -1;
		else if (left0 + 1 == left1 && right0 + 1 != right1)
			return 1;

		const int ret = cmp(*left0, *right0);
		if (ret < 0)
			return -1;
		else if (ret > 0)
			return 1;

		++left0;
		++right0;
	}

	if (left0 == left1 && right0 != right1)
		return -1;
	else if (left0 != left1 && right0 == right1)
		return 1;

	return 0;
}

/**
 * @brief Compares two iterables differentially, noncommutative.
 *
 * @param[out] outdec
 * @param[out] outinc
 */
template<typename Left, typename Right, typename Coll> std::pair<int, Coll> diff(Left left0, Left left1, Right right0, Right right1, int* outdec = nullptr, Coll* outinc = nullptr) {
	int dec = 0;
	Coll inc;

	if (outdec)
		*outdec = 0;
	if (outinc)
		outinc->clear();

	while (left0 != left1 && right0 != right1) {
		if (*left0 != *right0) {
			while (right0 != right1) {
				++dec;
				++right0;
			}
			while (left0 != left1) {
				inc.push_back(*left0);
				++left0;
			}

			if (outdec)
				*outdec = dec;
			if (outinc)
				*outinc = inc;

			return std::make_pair(dec, inc);
		}

		++left0;
		++right0;
	}

	if (left0 == left1 && right0 != right1) {
		while (right0 != right1) {
			++dec;
			++right0;
		}
	} else if (left0 != left1 && right0 == right1) {
		while (left0 != left1) {
			inc.push_back(*left0);
			++left0;
		}
	}

	if (outdec)
		*outdec = dec;
	if (outinc)
		*outinc = inc;

	return std::make_pair(dec, inc);
}

}

/**
 * @brief Dual collection.
 */
template<typename Val, typename Coll1 = std::deque<Val>, typename Coll2 = std::deque<Val> > class Dual {
public:
	typedef Val ValueType;
	typedef Coll1 FirstCollection;
	typedef Coll2 SecondCollection;
	typedef FirstCollection Collection;

	typedef typename Collection::const_iterator ConstIterator;

	typedef std::function<int(const ValueType &, const ValueType &)> Comparer;

	struct Index {
	private:
		int _index = 0;
		bool _second = false;

	public:
		Index() {
		}
		Index(int index) : _index(index) {
		}
		Index(int index, bool second) : _index(index), _second(second) {
		}
		Index(const Index &other) {
			_index = other._index;
			_second = other._second;
		}

		bool operator == (const Index &other) const {
			return _index == other._index && _second == other._second;
		}
		bool operator == (int other) const {
			return _index == other;
		}
		bool operator != (const Index &other) const {
			return _index != other._index || _second != other._second;
		}
		bool operator != (int other) const {
			return _index != other;
		}

		Index &operator = (const Index &other) {
			_index = other._index;
			_second = other._second;

			return *this;
		}

		Index &operator ++ (void) {
			++_index;

			return *this;
		}
		Index operator ++ (int _) {
			(void)_;

			return Index(_index++, _second);
		}

		operator int (void) const {
			return _index;
		}

		bool second(void) const {
			return _second;
		}
	};

	typedef std::function<void(const ValueType &, Index)> ConstEnumerator;
	typedef std::function<void(ValueType &, Index)> Enumerator;

public:
	FirstCollection first;
	SecondCollection second;

private:
	Comparer _firstComparer = nullptr;
	Comparer _secondComparer = nullptr;

public:
	Dual() {
	}
	Dual(Comparer fstCmp, Comparer sndCmp) : _firstComparer(fstCmp), _secondComparer(sndCmp) {
	}

	const Collection &all(void) const {
		return first;
	}

	const ValueType &front(void) const {
		return first.front();
	}
	ValueType &front(void) {
		return first.front();
	}

	int count(void) const {
		return (int)first.size();
	}
	bool empty(void) const {
		return first.empty();
	}
	template<typename T> ConstIterator get(const T &val, std::function<int(const ValueType &, const T &)> cmp) const {
		if (_firstComparer) {
			ConstIterator it = std::lower_bound(
				first.begin(), first.end(),
				val,
				[&] (const ValueType &left, const T &right) -> bool {
					return cmp(left, right) < 0;
				}
			);
			if (it == first.end())
				return first.end();
			if (cmp(*it, val) != 0)
				return first.end();

			return it;
		}

		return std::find_if(
			first.begin(), first.end(),
			[&] (const ValueType &val_) -> bool {
				return cmp(val_, val) == 0;
			}
		);
	}
	bool add(const ValueType &val) {
		if (_firstComparer) {
			first.insert(
				std::upper_bound(
					first.begin(), first.end(),
					val,
					[&] (const ValueType &left, const ValueType &right) -> bool {
						return _firstComparer(left, right) < 0;
					}
				),
				val
			);
		} else {
			first.push_back(val);
		}

		if (_secondComparer) {
			second.insert(
				std::upper_bound(
					second.begin(), second.end(),
					val,
					[&] (const ValueType &left, const ValueType &right) -> bool {
						return _secondComparer(left, right) < 0;
					}
				),
				val
			);
		} else {
			second.push_back(val);
		}

		return true;
	}
	int remove(const ValueType &val) {
		typename Collection::iterator fit = std::find_if(
			first.begin(), first.end(),
			[&] (const ValueType &val_) -> bool {
				return val_ == val;
			}
		);
		if (fit == first.end())
			return 0;

		first.erase(fit);

		typename SecondCollection::iterator sit = std::find_if(
			second.begin(), second.end(),
			[&] (const ValueType &val_) -> bool {
				return val_ == val;
			}
		);
		assert(sit != second.end());
		second.erase(sit);

		return 1;
	}
	void clear(void) {
		first.clear();
		second.clear();
	}
	Index indexOf(const ValueType &val, bool second_) const {
		if (!second_) {
			if (_firstComparer) {
				typename FirstCollection::const_iterator it = std::upper_bound(
					first.begin(), first.end(),
					val,
					[&] (const ValueType &left, const ValueType &right) -> bool {
						return _firstComparer(left, right) < 0;
					}
				);
				if (it != first.end())
					return Index((int)(it - first.begin()), second_);
			} else {
				typename FirstCollection::const_iterator it = std::find_if(
					first.begin(), first.end(),
					[&] (const ValueType &val_) -> bool {
						return val_ == val;
					}
				);
				if (it != first.end())
					return Index((int)(it - first.begin()), second_);
			}

			return Index(-1, second_);
		}

		if (_secondComparer) {
			typename SecondCollection::const_iterator it = std::upper_bound(
				second.begin(), second.end(),
				val,
				[&] (const ValueType &left, const ValueType &right) -> bool {
					return _secondComparer(left, right) < 0;
				}
			);
			if (it != first.end())
				return Index((int)(it - second.begin()), second_);
		} else {
			typename SecondCollection::const_iterator it = std::find_if(
				second.begin(), second.end(),
				[&] (const ValueType &val_) -> bool {
					return val_ == val;
				}
			);
			if (it != second.end())
				return Index((int)(it - second.begin()), second_);
		}

		return Index(-1, second_);
	}

	ConstIterator begin(void) const {
		return first.begin();
	}
	ConstIterator end(void) const {
		return first.end();
	}

	int foreach(ConstEnumerator enumerator) const {
		int result = 0;
		Index i = 0;
		for (ConstIterator fit = first.begin(); fit != first.end(); ++fit) {
			enumerator(*fit, i++);
			++result;
		}

		return result;
	}
	/**
	 * @note This function gives you both read and write right, but you should
	 *   never modify the relevant data if it's ordered.
	 */
	int foreach(Enumerator enumerator) {
		int result = 0;
		Index i = 0;
		for (typename Collection::iterator fit = first.begin(); fit != first.end(); ++fit) {
			enumerator(*fit, i++);
			++result;
		}

		return result;
	}

	void sort(void) {
		if (_firstComparer) {
			std::sort(
				first.begin(), first.end(),
				[&] (const ValueType &left, const ValueType &right) -> bool {
					return _firstComparer(left, right) < 0;
				}
			);
		}

		if (_secondComparer) {
			std::sort(
				second.begin(), second.end(),
				[&] (const ValueType &left, const ValueType &right) -> bool {
					return _secondComparer(left, right) < 0;
				}
			);
		}
	}
};

/* ===========================================================================} */

#endif /* __GENERIC_H__ */
