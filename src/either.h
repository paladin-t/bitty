/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EITHER_H__
#define __EITHER_H__

#include "bitty.h"

/*
** {===========================================================================
** Maybe
*/

template<class T> class Maybe;
template<> class Maybe<void> {
public:
	template<typename T> bool operator == (const Maybe<T> &other) const {
		if (other.empty())
			return true;

		return false;
	}
	bool operator == (const Maybe<void> &) const {
		return true;
	}

	template<typename T> bool operator != (const Maybe<T> &other) const {
		if (other.empty())
			return false;

		return true;
	}
	bool operator != (const Maybe<void> &) const {
		return false;
	}

	operator bool (void) const {
		return false;
	}

	bool empty(void) const {
		return true;
	}
};
template<class T> class Maybe {
private:
	union {
		T _value;
	};

	bool _hasValue = false;

public:
	Maybe() : _hasValue(false) {
	}
	Maybe(const T &value) : _value(value), _hasValue(true) {
	}
	Maybe(Maybe<void>) : _hasValue(false) {
	}
	Maybe(const Maybe<T> &other) : _hasValue(other._hasValue) {
		if (other._hasValue)
			new (&_value) T(other._value);
	}
	~Maybe() {
		if (_hasValue)
			_value.~T();
	}

	bool operator == (const Maybe<T> &other) {
		if (empty() && other.empty())
			return true;
		if(!empty() && !other.empty())
			return get() == other.get();

		return false;
	}
	bool operator == (const Maybe<void> &) {
		return empty();
	}

	bool operator != (const Maybe<T> &other) {
		if (empty() && other.empty())
			return false;
		if(!empty() && !other.empty())
			return get() != other.get();

		return true;
	}
	bool operator != (const Maybe<void> &) {
		return !empty();
	}

	explicit operator bool (void) const {
		return _hasValue;
	}

	bool empty(void) const {
		return !_hasValue;
	}
	T get(T default_) {
		return _hasValue ? _value : default_;
	}
	T get(void) {
		assert(_hasValue);

		return _value;
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Either
*/

template<class T> struct Left {
	T value;

	Left(const T &val) : value(val) {
	}
	Left(const Left<T> &other) : value(other.val) {
	}

	Left<T> &operator = (const T &val) {
		value = val;

		return *this;
	}
	Left<T> &operator = (const Left<T> &other) {
		value = other.value;

		return *this;
	}
};

template<class T> struct Right {
	T value;

	Right(const T &val) : value(val) {
	}
	Right(const Right<T> &other) : value(other.val) {
	}

	Right<T> &operator = (const T &val) {
		value = val;

		return *this;
	}
	Right<T> &operator = (const Right<T> &other) {
		value = other.value;

		return *this;
	}
};

/**
 * @brief Either.
 */
template<class L, class R> class Either {
private:
	L _left;
	R _right;

	bool _isLeft = false;

public:
	Either(const Left<L> &left) : _left(left.value), _isLeft(true) {
	}
	Either(const Right<R> &right) : _right(right.value), _isLeft(false) {
	}
	Either(const Either<L, R> &either) : _isLeft(either._isLeft) {
		if (_isLeft)
			_left = either._left;
		else
			_right = either._right;
	}
	~Either() {
	}

	Either<L, R> &operator = (const Left<L> &left) {
		_left = left.value;
		_isLeft = true;

		return *this;
	}
	Either<L, R> &operator = (const Right<R> &right) {
		_right = right.value;
		_isLeft = false;

		return *this;
	}
	Either<L, R> &operator = (const Either<L, R> &other) {
		if (other._isLeft)
			_left = other._left;
		else
			_right = other._right;
		_isLeft = other._isLeft;

		return *this;
	}

	bool operator == (const Either<L, R> &either) {
		if (isLeft()) {
			if (either.isLeft())
				return left() == either.left();
		} else {
			if (!either.isLeft())
				return right() == either.right();
		}

		return false;
	}
	bool operator != (const Either<L, R> &either) {
		if (isLeft()) {
			if (either.isLeft())
				return left() != either.left();
		} else {
			if (!either.isLeft())
				return right() != either.right();
		}

		return true;
	}

	operator bool (void) const {
		return !_isLeft;
	}

	bool isLeft(void) const {
		return _isLeft;
	}
	bool isRight(void) const {
		return !_isLeft;
	}

	Maybe<L> left(void) const {
		return _isLeft ? Maybe<L>(_left) : Maybe<void>();
	}
	Maybe<R> right(void) const {
		return _isLeft ? Maybe<void>() : Maybe<R>(_right);
	}
};

/* ===========================================================================} */

#endif /* __EITHER_H__ */
