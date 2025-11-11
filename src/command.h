/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "bitty.h"
#include "object.h"
#include <deque>
#include <functional>
#include <map>

/*
** {===========================================================================
** Command
*/

class Command {
public:
	typedef std::function<Command*(void)> Creator;
	typedef std::function<void(Command*)> Destroyer;

public:
	Command();
	virtual ~Command();

	virtual unsigned type(void) const = 0;

	virtual const char* toString(void) const = 0;

	template<typename T> static bool is(const Command* ptr) {
		return !!dynamic_cast<const T*>(ptr);
	}
	template<typename T> static const T* as(const Command* ptr) {
		return dynamic_cast<const T*>(ptr);
	}
	template<typename T> static T* as(Command* ptr) {
		return dynamic_cast<T*>(ptr);
	}

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) = 0;
	int redo(Object::Ptr obj) {
		return redo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> int redo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return redo(obj, (int)n, argv);
	}
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) = 0;
	int undo(Object::Ptr obj) {
		return undo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> int undo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return undo(obj, (int)n, argv);
	}

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) = 0;
	int exec(Object::Ptr obj) {
		return exec(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> int exec(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return exec(obj, (int)n, argv);
	}

protected:
	template<typename Arg> static Arg unpack(int argc, const Variant* argv, int idx, Arg default_) {
		return (0 <= idx && idx < argc && argv) ? (Arg)argv[idx] : default_;
	}
};

/* ===========================================================================} */

/*
** {===========================================================================
** Command queue
*/

class CommandQueue {
private:
	struct Factory {
		Command::Creator create = nullptr;
		Command::Destroyer destroy = nullptr;

		Factory(Command::Creator create, Command::Destroyer destroy);
	};
	typedef std::map<unsigned, Factory> Factories;

	typedef std::deque<Command*> Collection;

private:
	Factories _factories;
	Collection _collection;
	Collection::iterator _cursor;
	int _savedpoint = 0;

	int _threshold = -1;

public:
	CommandQueue(int threshold = -1);
	~CommandQueue();

	int cursor(void) const;
	int offset(void) const;

	Command* enqueue(unsigned type);
	template<typename T> T* enqueue(void) {
		return static_cast<T*>(enqueue(T::TYPE()));
	}
	bool empty(void) const;
	Command* back(void);
	Command* at(int index);

	const Command* redoable(void) const;
	const Command* undoable(void) const;

	int redo(Object::Ptr obj, int argc, const Variant* argv);
	int redo(Object::Ptr obj) {
		return redo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> int redo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return redo(obj, (int)n, argv);
	}
	int undo(Object::Ptr obj, int argc, const Variant* argv);
	int undo(Object::Ptr obj) {
		return undo(obj, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> int undo(Object::Ptr obj, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return undo(obj, (int)n, argv);
	}

	bool hasUnsavedChanges(void) const;
	void markChangesSaved(void);

	bool registerFactory(unsigned type, Command::Creator create, Command::Destroyer destroy);
	template<typename T> bool registerFactory(void) {
		return registerFactory(T::TYPE(), T::create, T::destroy);
	}
	bool unregisterFactory(unsigned type);
	template<typename T> bool unregisterFactory(void) {
		return unregisterFactory(T::TYPE());
	}

private:
	Command* createCommand(unsigned type);
	void destroyCommand(Command* ptr);
};

/* ===========================================================================} */

#endif /* __COMMAND_H__ */
