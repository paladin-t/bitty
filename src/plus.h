/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PLUS_H__
#define __PLUS_H__

#include "bitty.h"
#include <functional>
#include <memory>
#if BITTY_MULTITHREAD_ENABLED
#	include <atomic>
#endif /* BITTY_MULTITHREAD_ENABLED */
#include <mutex>

/*
** {===========================================================================
** C++ utilities
**
** @note The principle of these utilities is to help writing code in a handier
**   way, for the application.
*/

/**
 * @brief Non-copyable.
 */
class NonCopyable {
public:
	NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;

	NonCopyable &operator = (const NonCopyable &) = delete;
};

#if BITTY_MULTITHREAD_ENABLED
/**
 * @brief Atomic.
 */
template<typename T> class Atomic : public NonCopyable {
private:
	std::atomic<T> _t;

public:
	Atomic() {
	}
	Atomic(T val) : _t(val) {
	}

	T operator = (T val) {
		_t = val;

		return _t;
	}
	T operator = (const Atomic &other) {
		_t = (T)other;

		return _t;
	}

	operator T (void) const {
		return _t;
	}
};

/**
 * @brief Mutex.
 */
class Mutex : public NonCopyable {
private:
	std::mutex _lock;

public:
	Mutex() {
	}
	~Mutex() {
	}

	void lock(void) {
		_lock.lock();
	}
	void unlock(void) {
		_lock.unlock();
	}
	bool tryLock(void) {
		return _lock.try_lock();
	}
};

/**
 * @brief Recursive mutex.
 */
class RecursiveMutex : public NonCopyable {
private:
	std::recursive_mutex _lock;

public:
	RecursiveMutex() {
	}
	~RecursiveMutex() {
	}

	void lock(void) {
		_lock.lock();
	}
	void unlock(void) {
		_lock.unlock();
	}
	bool tryLock(void) {
		return _lock.try_lock();
	}
};
#else /* BITTY_MULTITHREAD_ENABLED */
/**
 * @brief Atomic.
 */
template<typename T> class Atomic : public NonCopyable {
private:
	T _t;

public:
	Atomic() {
	}
	Atomic(T val) : _t(val) {
	}

	T operator = (T val) {
		_t = val;

		return _t;
	}
	T operator = (const Atomic &other) {
		_t = (T)other;

		return _t;
	}

	operator T (void) const {
		return _t;
	}
};

/**
 * @brief Mutex.
 */
class Mutex : public NonCopyable {
public:
	Mutex() {
	}
	~Mutex() {
	}

	void lock(void) {
	}
	void unlock(void) {
	}
	bool tryLock(void) {
		return true;
	}
};

/**
 * @brief Recursive mutex.
 */
class RecursiveMutex : public NonCopyable {
public:
	RecursiveMutex() {
	}
	~RecursiveMutex() {
	}

	void lock(void) {
	}
	void unlock(void) {
	}
	bool tryLock(void) {
		return true;
	}
};
#endif /* BITTY_MULTITHREAD_ENABLED */

/**
 * @brief Lock guard.
 */
template<typename ...T> class LockGuard final : public std::lock_guard<T...> {
public:
	typedef std::unique_ptr<std::lock_guard<T...> > UniquePtr;

public:
	using std::lock_guard<T...>::lock_guard;
};

/**
 * @brief Try lock guard.
 */
template<typename T> class TryLockGuard final : public NonCopyable {
private:
	bool _locked = false;
	T &_lock;

public:
	TryLockGuard(T &lock) : _lock(lock) {
		if (_lock.tryLock())
			_locked = true;
	}
	~TryLockGuard() {
		if (_locked)
			_lock.unlock();
	}

	bool locked(void) const {
		return _locked;
	}
};

/**
 * @brief Variable guard.
 */
template<typename T> class VariableGuard {
private:
	T* _ptr = nullptr;
	T _prev;
	T _curr;
	bool _changed = false;

public:
	VariableGuard(T* ptr, const T &prev, const T &curr) : _prev(prev), _curr(curr) {
		_ptr = ptr;

		if (*_ptr == _prev) {
			*_ptr = _curr;
			_changed = true;
		}
	}
	~VariableGuard() {
		if (_changed) {
			*_ptr = _prev;
		}
	}

	const T &previous(void) const {
		return _prev;
	}
	const T &current(void) const {
		return _curr;
	}

	bool changed(void) const {
		return _changed;
	}
};

/**
 * @brief Procedure guard.
 */
template<typename T> class ProcedureGuard {
public:
	typedef std::function<T*(void)> Prev;
	typedef std::function<void(T*)> Post;

private:
	T* _ptr = nullptr;
	Prev _prev;
	Post _post;

public:
	ProcedureGuard(T* &var, Prev prev, Post post) {
		_prev = prev;
		_post = post;
		var = _ptr = _prev();
	}
	ProcedureGuard(Prev prev, Post post) {
		_prev = prev;
		_post = post;
		_ptr = _prev();
	}
	~ProcedureGuard() {
		_post(_ptr);
	}
};

/**
 * @brief Gets whether a shared pointer is unique.
 */
template<typename T> bool unique(const std::shared_ptr<T> &ptr) {
	return ptr.use_count() == 1; // `std::shared_ptr<T>::unique()` is deprecated in C++17, removed in C++20.
}

/**
 * @brief Pointer of anything.
 */
typedef std::shared_ptr<void> Any;

/**
 * @brief Handler.
 */
template<typename Self, typename Ret, typename ...Args> class Handler {
public:
	typedef std::function<Ret(Args...)> Callback;

private:
	Callback _callback = nullptr;
	Any _userdata = nullptr;

public:
	Handler() {
	}
	Handler(std::nullptr_t) {
	}
	Handler(const Callback &cb) : _callback(cb) {
	}
	Handler(const Callback &cb, const Any &ud) : _callback(cb), _userdata(ud) {
	}

	Ret operator () (Args... args) const {
		return _callback(args...);
	}

	const Any &userdata(void) const {
		return _userdata;
	}
	Any &userdata(void) {
		return _userdata;
	}

	bool empty(void) const {
		return !_callback;
	}
	void clear(void) {
		_callback = nullptr;
		_userdata = nullptr;
	}
};

/* ===========================================================================} */

#endif /* __PLUS_H__ */
