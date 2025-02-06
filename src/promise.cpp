/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "promise.h"

/*
** {===========================================================================
** Promise
*/

class PromiseImpl : public Promise {
private:
	States _state = PENDING;
	ThenHandler _then;
	FailHandler _fail;
	AlwaysHandler _always;

	Variant _value = nullptr;
	Variant _error = nullptr;
	bool _finished = false;

	mutable Mutex _lock;

public:
	PromiseImpl() {
	}
	virtual ~PromiseImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual States state(void) const override {
		LockGuard<Mutex> guard(_lock);

		return _state;
	}
	virtual Variant value(void) const override {
		LockGuard<Mutex> guard(_lock);

		return _value;
	}
	virtual Variant error(void) const override {
		LockGuard<Mutex> guard(_lock);

		return _error;
	}

	virtual Promise* then(const ThenHandler &cb) override {
		LockGuard<Mutex> guard(_lock);

		_then = cb;

		if (_state == RESOLVED) {
			if (!_then.empty())
				_then(&_then, _value);
		}

		return this;
	}
	virtual Promise* fail(const FailHandler &cb) override {
		LockGuard<Mutex> guard(_lock);

		_fail = cb;

		if (_state == REJECTED) {
			if (!_fail.empty())
				_fail(&_fail, _error);
		}

		return this;
	}
	virtual Promise* always(const AlwaysHandler &cb) override {
		LockGuard<Mutex> guard(_lock);

		_always = cb;

		if (_state == RESOLVED || _state == RESOLVED) {
			if (!_always.empty())
				_always(&_always);
		}

		return this;
	}

	virtual void resolve(const Variant &val) override {
		LockGuard<Mutex> guard(_lock);

		_state = RESOLVED;
		_value = val;
		_error = nullptr;
	}
	virtual void reject(const Variant &val) override {
		LockGuard<Mutex> guard(_lock);

		_state = REJECTED;
		_value = nullptr;
		_error = val;
	}

	virtual void clear(void) override {
		LockGuard<Mutex> guard(_lock);

		_state = PENDING;
		_then.clear();
		_fail.clear();
		_always.clear();

		_value = nullptr;
		_error = nullptr;
		_finished = false;
	}

	virtual bool update(double) override {
		LockGuard<Mutex> guard(_lock);

		if (_finished)
			return false;

		switch (_state) {
		case RESOLVED:
			_finished = true;

			if (!_then.empty())
				_then(&_then, _value);

			if (!_always.empty())
				_always(&_always);

			break;
		case REJECTED:
			_finished = true;

			if (!_fail.empty())
				_fail(&_fail, _error);

			if (!_always.empty())
				_always(&_always);

			break;
		default: // Do nothing.
			break;
		}

		return true;
	}
};

Promise* Promise::create(void) {
	PromiseImpl* result = new PromiseImpl();

	return result;
}

void Promise::destroy(Promise* ptr) {
	PromiseImpl* impl = static_cast<PromiseImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
