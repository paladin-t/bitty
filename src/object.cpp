/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "object.h"
#include "text.h"
#include <deque>

/*
** {===========================================================================
** Object and variant
*/

int Object::compare(const Object* other) const {
	if (!other)
		return 1;

	if (type() < other->type())
		return -1;
	if (type() > other->type())
		return 1;

	uintptr_t lptr = (uintptr_t)this;
	uintptr_t rptr = (uintptr_t)other;
	if (lptr < rptr)
		return -1;
	if (lptr > rptr)
		return 1;

	return 0;
}

bool Object::equals(const Object* other) const {
	return compare(other) == 0;
}

bool Object::clone(Object** ptr) const { // Non-clonable.
	if (ptr)
		*ptr = nullptr;

	return false;
}

Variant::Union::Union() {
}

Variant::Union::~Union() {
}

Variant::Variant() {
	_type = NIL;
	memset(&_var, 0, sizeof(Union));
}

Variant::Variant(std::nullptr_t) {
	_type = NIL;
	memset(&_var, 0, sizeof(Union));
}

Variant::Variant(bool val) {
	_type = BOOLEAN;
	_var.boolean = val;
}

Variant::Variant(Int val) {
	_type = INTEGER;
	_var.integer = val;
}

Variant::Variant(Real val) {
	_type = REAL;
	_var.real = val;
}

Variant::Variant(const char* val) {
	_type = STRING;
	new (&_var.string) std::string(val);
}

Variant::Variant(const std::string &val) {
	_type = STRING;
	new (&_var.string) std::string(val);
}

Variant::Variant(void* val) {
	_type = POINTER;
	_var.pointer = val;
}

Variant::Variant(Object::Ptr val) {
	_type = OBJECT;
	new (&_var.object) Object::Ptr(val);
}

Variant::Variant(const Variant &other) {
	clear();

	_type = other._type;
	switch (_type) {
	case NIL:
		memset(&_var, 0, sizeof(Union));

		break;
	case BOOLEAN:
		_var.boolean = other._var.boolean;

		break;
	case INTEGER:
		_var.integer = other._var.integer;

		break;
	case REAL:
		_var.real = other._var.real;

		break;
	case STRING:
		new (&_var.string) std::string(other._var.string);

		break;
	case POINTER:
		_var.pointer = other._var.pointer;

		break;
	case OBJECT:
		new (&_var.object) Object::Ptr(other._var.object);

		break;
	}
}

Variant::~Variant() {
	clear();
}

Variant &Variant::operator = (const Variant &other) {
	clear();

	_type = other._type;
	switch (_type) {
	case NIL:
		memset(&_var, 0, sizeof(Union));

		break;
	case BOOLEAN:
		_var.boolean = other._var.boolean;

		break;
	case INTEGER:
		_var.integer = other._var.integer;

		break;
	case REAL:
		_var.real = other._var.real;

		break;
	case STRING:
		new (&_var.string) std::string(other._var.string);

		break;
	case POINTER:
		_var.pointer = other._var.pointer;

		break;
	case OBJECT:
		new (&_var.object) Object::Ptr(other._var.object);

		break;
	}

	return *this;
}

bool Variant::operator < (const Variant &other) const {
	return compare(other) < 0;
}

Variant::operator std::nullptr_t (void) const {
	if (_type == NIL)
		return nullptr;

	return nullptr;
}

Variant::operator bool (void) const {
	switch (_type) {
	case NIL: return false;
	case BOOLEAN: return _var.boolean;
	case INTEGER: return !!_var.integer;
	case REAL: return !!_var.real;
	case STRING: return true;
	case POINTER: return !!_var.pointer;
	case OBJECT: return !!_var.object;
	default:
		return false;
	}
}

Variant::operator Int (void) const {
	switch (_type) {
	case NIL: return 0;
	case BOOLEAN: return _var.boolean ? 1 : 0;
	case INTEGER: return _var.integer;
	case REAL: return (Int)_var.real;
	case STRING: // Fall through.
	case POINTER: // Fall through.
	case OBJECT: // Fall through.
	default:
		return 0;
	}
}

Variant::operator Real (void) const {
	switch (_type) {
	case NIL: return 0;
	case BOOLEAN: return _var.boolean ? 1 : 0;
	case INTEGER: return (Real)_var.integer;
	case REAL: return _var.real;
	case STRING: // Fall through.
	case POINTER: // Fall through.
	case OBJECT: // Fall through.
	default:
		return 0;
	}
}

Variant::operator const char* (void) const {
	if (_type == STRING)
		return _var.string.c_str();

	return "";
}

Variant::operator const std::string (void) const {
	if (_type == STRING)
		return _var.string;

	return "";
}

Variant::operator void* (void) const {
	if (_type == POINTER)
		return _var.pointer;

	return nullptr;
}

Variant::operator Object::Ptr (void) const {
	if (_type == OBJECT)
		return _var.object;

	return nullptr;
}

Variant::Types Variant::type(void) const {
	return _type;
}

int Variant::compare(const Variant &other) const {
	if (_type < other._type)
		return -1;
	if (_type > other._type)
		return 1;

	switch (_type) {
	case NIL:
		return 0;
	case BOOLEAN:
		if (!_var.boolean && other._var.boolean)
			return -1;
		if (_var.boolean && !other._var.boolean)
			return 1;

		return 0;
	case INTEGER:
		if (_var.integer < other._var.integer)
			return -1;
		if (_var.integer > other._var.integer)
			return 1;

		return 0;
	case REAL:
		if (_var.real < other._var.real)
			return -1;
		if (_var.real > other._var.real)
			return 1;

		return 0;
	case STRING:
		if (_var.string < other._var.string)
			return -1;
		if (_var.string > other._var.string)
			return 1;

		return 0;
	case POINTER: {
			if (!_var.pointer && other._var.pointer)
				return -1;
			if (_var.pointer && !other._var.pointer)
				return 1;

			const uintptr_t left = (uintptr_t)_var.pointer;
			const uintptr_t right = (uintptr_t)other._var.pointer;
			if (left < right)
				return -1;
			else if (left > right)
				return 1;
		}

		return 0;
	case OBJECT:
		if (!_var.object && other._var.object)
			return -1;
		if (_var.object && !other._var.object)
			return 1;

		return _var.object->compare(other._var.object.get()) < 0;
	}

	return 0;
}

bool Variant::equals(const Variant &other) const {
	return compare(other) == 0;
}

void Variant::clear(void) {
	switch (_type) {
	case NIL:
		// Do nothing.

		break;
	case BOOLEAN:
		_var.boolean = false;

		break;
	case INTEGER:
		_var.integer = 0;

		break;
	case REAL:
		_var.real = 0;

		break;
	case STRING:
		_var.string.~basic_string();

		break;
	case POINTER:
		_var.pointer = nullptr;

		break;
	case OBJECT:
		_var.object.~shared_ptr();

		break;
	}
}

bool Variant::isNumber(void) const {
	return _type == INTEGER || _type == REAL;
}

std::string Variant::toString(void) const {
	switch (_type) {
	case NIL:
		return "nil";
	case BOOLEAN:
		return Text::toString(_var.boolean);
	case INTEGER:
		return Text::toString(_var.integer);
	case REAL:
		return Text::toString(_var.real);
	case STRING:
		return _var.string;
	case POINTER:
		return "pointer";
	case OBJECT:
		return "object";
	}

	return "unknown";
}

/* ===========================================================================} */

/*
** {===========================================================================
** Enumerable class
*/

Enumerable::~Enumerable() {
	onEnumerableDestructing();
}

IEnumerator::Ptr Enumerable::enumerate(IEnumerator* ptr) {
	IEnumerator::Ptr result(
		ptr,
		std::bind(&Enumerable::onEnumeratorDestructing, this, std::placeholders::_1)
	);
	_enumerators.push_back(result);

	return result;
}

void Enumerable::onEnumerableDestructing(void) {
	for (IEnumerator::WeakPtr &wptr : _enumerators) {
		if (wptr.expired())
			continue;

		IEnumerator::Ptr ptr = wptr.lock();

		return ptr->invalidate();
	}
}

void Enumerable::onEnumeratorDestructing(IEnumerator* obj) {
	_enumerators.remove_if(
		[&] (IEnumerator::WeakPtr &wptr) -> bool {
			if (wptr.expired())
				return true;

			IEnumerator::Ptr ptr = wptr.lock();

			return ptr.get() == obj;
		}
	);

	delete obj;
}

/* ===========================================================================} */

/*
** {===========================================================================
** List and dictionary class
*/

class ListImpl : public List {
private:
	typedef std::deque<Variant> Collection;

	class Enumerator : public IEnumerator {
	private:
		Collection* _collection = nullptr;
		bool _invalidated = false;
		Variant::Int _index = -1;
		Collection::iterator _iterator;

	public:
		Enumerator(Collection &coll) {
			_collection = &coll;
			_iterator = _collection->begin();
		}

		BITTY_CLASS_TYPE('L', 'S', 'T', 'I')

		virtual unsigned type(void) const override {
			return TYPE();
		}

		virtual bool next(void) override {
			if (_invalidated)
				return false;

			if (_index == -1)
				_iterator = _collection->begin();
			else
				++_iterator;
			++_index;

			return _iterator != _collection->end();
		}

		virtual Variant::Pair current(void) const override {
			if (_invalidated)
				return Variant::Pair(Variant(), Variant());

			const Variant &val = *_iterator;

			return Variant::Pair(Variant(_index), val);
		}

		virtual void invalidate(void) override {
			_invalidated = true;
		}
	};

private:
	Collection _collection;

public:
	virtual ~ListImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual int count(void) const override {
		return (int)_collection.size();
	}

	virtual Variant at(int index) const override {
		if (index < 0 || index >= (int)_collection.size())
			return Variant();

		return _collection[index];
	}

	virtual void add(const Variant &val) override {
		_collection.push_back(val);
	}
	virtual bool insert(int index, const Variant &val) override {
		if (index == (int)_collection.size()) {
			_collection.push_back(val);

			return true;
		}

		if (index < 0 || index >= (int)_collection.size())
			return false;

		Collection::const_iterator where = _collection.begin() + index;
		_collection.insert(where, val);

		return true;
	}
	virtual bool set(int index, const Variant &val) override {
		if (index < 0 || index >= (int)_collection.size())
			return false;

		_collection[index] = val;

		return true;
	}

	virtual bool remove(int index) override {
		if (index < 0 || index >= (int)_collection.size())
			return false;

		Collection::const_iterator where = _collection.begin() + index;
		_collection.erase(where);

		return true;
	}
	virtual void clear(void) override {
		_collection.clear();
	}

	virtual IEnumerator::Ptr enumerate(void) override {
		return Enumerable::enumerate(new Enumerator(_collection));
	}
};

List* List::create(void) {
	ListImpl* result = new ListImpl();

	return result;
}

void List::destroy(List* ptr) {
	ListImpl* impl = static_cast<ListImpl*>(ptr);
	delete impl;
}

class DictionaryImpl : public Dictionary {
private:
	typedef std::map<std::string, Variant> Collection;

	class Enumerator : public IEnumerator {
	private:
		Collection* _collection = nullptr;
		bool _invalidated = false;
		int _index = -1;
		Collection::iterator _iterator;

	public:
		Enumerator(Collection &coll) {
			_collection = &coll;
			_iterator = _collection->begin();
		}

		BITTY_CLASS_TYPE('D', 'C', 'T', 'I')

		virtual unsigned type(void) const override {
			return TYPE();
		}

		virtual bool next(void) override {
			if (_invalidated)
				return false;

			if (_index == -1)
				_iterator = _collection->begin();
			else
				++_iterator;
			++_index;

			return _iterator != _collection->end();
		}

		virtual Variant::Pair current(void) const override {
			if (_invalidated)
				return Variant::Pair(Variant(), Variant());

			const std::string &key = _iterator->first;
			const Variant &val = _iterator->second;

			return Variant::Pair(Variant(key), val);
		}

		virtual void invalidate(void) override {
			_invalidated = true;
		}
	};

private:
	Collection _collection;

public:
	virtual ~DictionaryImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual int count(void) const override {
		return (int)_collection.size();
	}

	virtual Keys keys(void) const override {
		Keys result;
		for (Collection::value_type kv : _collection)
			result.push_back(kv.first);

		return result;
	}
	virtual bool contains(const std::string &key) const override {
		Collection::const_iterator it = _collection.find(key);

		return it != _collection.end();
	}
	virtual Variant get(const std::string &key) const override {
		Collection::const_iterator it = _collection.find(key);
		if (it == _collection.end())
			return Variant();

		return it->second;
	}

	virtual void add(const std::string &key, const Variant &val) override {
		_collection[key] = val;
	}
	virtual void set(const std::string &key, const Variant &val) override {
		_collection[key] = val;
	}

	virtual bool remove(const std::string &key) override {
		Collection::iterator it = _collection.find(key);
		if (it == _collection.end())
			return false;

		_collection.erase(it);

		return true;
	}
	virtual void clear(void) override {
		_collection.clear();
	}

	virtual IEnumerator::Ptr enumerate(void) override {
		return Enumerable::enumerate(new Enumerator(_collection));
	}
};

Dictionary* Dictionary::create(void) {
	DictionaryImpl* result = new DictionaryImpl();

	return result;
}

void Dictionary::destroy(Dictionary* ptr) {
	DictionaryImpl* impl = static_cast<DictionaryImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
