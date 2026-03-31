/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "command.h"

/*
** {===========================================================================
** Command
*/

namespace Bitty {

Command::Command() {
}

Command::~Command() {
}

}

/* ===========================================================================} */

/*
** {===========================================================================
** Command queue
*/

namespace Bitty {

CommandQueue::Factory::Factory(Command::Creator create_, Command::Destroyer destroy_) : create(create_), destroy(destroy_) {
}

CommandQueue::CommandQueue(int threshold) : _threshold(threshold) {
	_cursor = _collection.end();
}

CommandQueue::~CommandQueue() {
	for (Command* c : _collection)
		destroyCommand(c);
	_collection.clear();
}

int CommandQueue::cursor(void) const {
	return (int)(_cursor - _collection.begin());
}

int CommandQueue::offset(void) const {
	const int c = cursor();

	if (_savedpoint == -1)
		return c;

	return c - _savedpoint;
}

Command* CommandQueue::enqueue(unsigned type) {
	if (_cursor != _collection.end()) {
		for (Collection::iterator it = _cursor; it != _collection.end(); ++it) {
			Command* obsolete = *it;
			destroyCommand(obsolete);
		}
		_collection.erase(_cursor, _collection.end());
		_cursor = _collection.end();
	}

	Command* current = createCommand(type);
	_collection.push_back(current);
	_cursor = _collection.end();

	if (_threshold > 0) {
		while ((int)_collection.size() > _threshold) {
			if (_savedpoint > 0)
				--_savedpoint;
			const int offset = cursor();
			Command* overflow = _collection.front();
			destroyCommand(overflow);
			_collection.pop_front();
			_cursor = _collection.begin() + offset - 1;
		}
	}

	if (_savedpoint >= cursor())
		_savedpoint = -1;

	return _collection.back();
}

bool CommandQueue::empty(void) const {
	return _collection.empty() || _cursor == _collection.begin();
}

Command* CommandQueue::back(void) {
	return at(-1);
}

Command* CommandQueue::at(int index) {
	if (index >= 0 && index < (int)_collection.size())
		return _collection[index];

	if (index < 0 && -index <= (int)_collection.size())
		return _collection[(int)_collection.size() + index];

	return nullptr;
}

const Command* CommandQueue::redoable(void) const {
	if (_cursor == _collection.end())
		return nullptr;

	return *_cursor;
}

const Command* CommandQueue::undoable(void) const {
	if (_cursor == _collection.begin())
		return nullptr;

	Collection::iterator it = _cursor;
	--it;

	return *it;
}

int CommandQueue::redo(Object::Ptr obj, int argc, const Variant* argv) {
	if (_cursor == _collection.end())
		return -1;

	Command* c = *_cursor;
	int ret = c->redo(obj, argc, argv);
	++_cursor;

	return ret;
}

int CommandQueue::undo(Object::Ptr obj, int argc, const Variant* argv) {
	if (_cursor == _collection.begin())
		return -1;

	--_cursor;
	Command* c = *_cursor;

	return c->undo(obj, argc, argv);
}

bool CommandQueue::hasUnsavedChanges(void) const {
	return _savedpoint == -1 || _savedpoint != cursor();
}

void CommandQueue::markChangesSaved(void) {
	_savedpoint = cursor();
}

bool CommandQueue::registerFactory(unsigned type, Command::Creator create, Command::Destroyer destroy) {
	Factories::iterator it = _factories.find(type);
	if (it != _factories.end())
		return false;

	_factories.insert(std::make_pair(type, Factory(create, destroy)));

	return true;
}

bool CommandQueue::unregisterFactory(unsigned type) {
	Factories::iterator it = _factories.find(type);
	if (it == _factories.end())
		return false;

	_factories.erase(it);

	return true;
}

Command* CommandQueue::createCommand(unsigned type) {
	Factories::iterator it = _factories.find(type);
	if (it == _factories.end())
		return nullptr;

	return it->second.create();
}

void CommandQueue::destroyCommand(Command* ptr) {
	Factories::iterator it = _factories.find(ptr->type());
	if (it == _factories.end())
		return;

	it->second.destroy(ptr);
}

}

/* ===========================================================================} */
