/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "bitty.h"
#include <list>
#include <memory>
#include <string>

/*
** {===========================================================================
** Object and variant
*/

/**
 * @brief Object base.
 */
class Object {
public:
	typedef std::shared_ptr<Object> Ptr;
	typedef std::weak_ptr<Object> WeakPtr;

public:
	virtual ~Object() {
	}

	BITTY_CLASS_TYPE('O', 'B', 'J', 'T')

	virtual unsigned type(void) const = 0;

	virtual int compare(const Object* other) const;
	virtual bool equals(const Object* other) const;

	virtual bool clone(Object** ptr) const;

	template<typename T> static bool is(const Object* ptr) {
		return !!dynamic_cast<const T*>(ptr);
	}
	template<typename T> static const T* as(const Object* ptr) {
		return dynamic_cast<const T*>(ptr);
	}
	template<typename T> static T* as(Object* ptr) {
		return dynamic_cast<T*>(ptr);
	}

	template<typename T> static bool is(const Ptr &ptr) {
		return !!std::dynamic_pointer_cast<typename T::element_type>(ptr);
	}
	template<typename T> static const T as(const Ptr &ptr) {
		return std::dynamic_pointer_cast<typename T::element_type>(ptr);
	}
	template<typename T> static T as(Ptr &ptr) {
		return std::dynamic_pointer_cast<typename T::element_type>(ptr);
	}

	template<typename T> static bool is(const WeakPtr &ptr) {
		if (ptr.expired())
			return false;

		return !!std::dynamic_pointer_cast<T::element_type>(ptr);
	}
	template<typename T> static const T as(const WeakPtr &ptr) {
		return std::dynamic_pointer_cast<T::element_type>(ptr.lock());
	}
	template<typename T> static T as(WeakPtr &ptr) {
		if (ptr.expired())
			return nullptr;

		return std::dynamic_pointer_cast<T::element_type>(ptr);
	}
};

/**
 * @brief Variant.
 */
class Variant {
public:
	typedef std::pair<Variant, Variant> Pair;

	enum Types : unsigned char {
		NIL,
		BOOLEAN,
		INTEGER,
		REAL,
		STRING,
		POINTER,
		OBJECT
	};

	typedef int Int;
	typedef double Real;

private:
	union Union {
		bool boolean;
		Int integer;
		Real real;
		std::string string;
		void* pointer;
		Object::Ptr object;

		Union();
		~Union();
	};

private:
	Types _type = INTEGER;
	Union _var;

public:
	Variant();
	Variant(std::nullptr_t);
	Variant(bool val);
	Variant(Int val);
	Variant(Real val);
	Variant(const char* val);
	Variant(const std::string &val);
	Variant(void* val);
	Variant(Object::Ptr val);
	Variant(const Variant &other);
	~Variant();

	Variant &operator = (const Variant &other);
	bool operator < (const Variant &other) const;

	explicit operator std::nullptr_t (void) const;
	explicit operator bool (void) const;
	explicit operator Int (void) const;
	explicit operator Real (void) const;
	explicit operator const char* (void) const;
	explicit operator const std::string (void) const;
	explicit operator void* (void) const;
	explicit operator Object::Ptr (void) const;

	Types type(void) const;

	int compare(const Variant &other) const;
	bool equals(const Variant &other) const;

	void clear(void);

	bool isNumber(void) const;

	std::string toString(void) const;
};

/* ===========================================================================} */

/*
** {===========================================================================
** Enumerator and enumerable interfaces
*/

/**
 * @brief Enumerator object interface.
 */
class IEnumerator : public virtual Object {
public:
	typedef std::shared_ptr<IEnumerator> Ptr;
	typedef std::weak_ptr<IEnumerator> WeakPtr;

public:
	virtual bool next(void) = 0;

	virtual Variant::Pair current(void) const = 0;

	virtual void invalidate(void) = 0;
};

/**
 * @brief Enumerable object interface.
 */
class IEnumerable : public virtual Object {
public:
	typedef std::shared_ptr<IEnumerable> Ptr;

protected:
	typedef std::list<IEnumerator::WeakPtr> Enumerators;

public:
	virtual IEnumerator::Ptr enumerate(void) = 0;
};

/* ===========================================================================} */

/*
** {===========================================================================
** List and dictionary interfaces
*/

/**
 * @brief List object interface.
 */
class IList : public virtual Object {
public:
	typedef std::shared_ptr<IList> Ptr;

public:
	virtual int count(void) const = 0;

	virtual Variant at(int index) const = 0;

	virtual void add(const Variant &val) = 0;
	virtual bool insert(int index, const Variant &val) = 0;
	virtual bool set(int index, const Variant &val) = 0;

	virtual bool remove(int index) = 0;
	virtual void clear(void) = 0;
};

/**
 * @brief Dictionary object interface.
 */
class IDictionary : public virtual Object {
public:
	typedef std::shared_ptr<IDictionary> Ptr;

	typedef std::list<std::string> Keys;

public:
	virtual int count(void) const = 0;

	virtual Keys keys(void) const = 0;
	virtual bool contains(const std::string &key) const = 0;
	virtual Variant get(const std::string &key) const = 0;

	virtual void add(const std::string &key, const Variant &val) = 0;
	virtual void set(const std::string &key, const Variant &val) = 0;

	virtual bool remove(const std::string &key) = 0;
	virtual void clear(void) = 0;
};

/* ===========================================================================} */

/*
** {===========================================================================
** Enumerable class
*/

/**
 * @brief Enumerable class.
 */
class Enumerable : public IEnumerable {
private:
	Enumerators _enumerators;

public:
	virtual ~Enumerable() override;

	using IEnumerable::enumerate;

protected:
	IEnumerator::Ptr enumerate(IEnumerator* ptr);

protected:
	void onEnumerableDestructing(void);

	void onEnumeratorDestructing(IEnumerator* obj);
};

/* ===========================================================================} */

/*
** {===========================================================================
** List and dictionary classes
*/

/**
 * @brief List class.
 */
class List : public IList, public Enumerable {
public:
	BITTY_CLASS_TYPE('L', 'I', 'S', 'T')

	static List* create(void);
	static void destroy(List* ptr);
};

/**
 * @brief Dictionary class.
 */
class Dictionary : public IDictionary, public Enumerable {
public:
	BITTY_CLASS_TYPE('D', 'I', 'C', 'T')

	static Dictionary* create(void);
	static void destroy(Dictionary* ptr);
};

/* ===========================================================================} */

#endif /* __OBJECT_H__ */
