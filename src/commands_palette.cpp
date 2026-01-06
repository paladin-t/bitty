/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_palette.h"
#include "palette.h"

/*
** {===========================================================================
** Palette commands
*/

namespace Commands {

namespace Palette {

Change::Change() {
	index(-1);
}

Change::~Change() {
}

unsigned Change::type(void) const {
	return TYPE();
}

const char* Change::toString(void) const {
	return "Set color";
}

int Change::redo(Object::Ptr obj, int, const Variant*) {
	::Palette::Ptr ptr = Object::as<::Palette::Ptr>(obj);

	return ptr->set(index(), &color()) ? 1 : 0;
}

int Change::undo(Object::Ptr obj, int, const Variant*) {
	::Palette::Ptr ptr = Object::as<::Palette::Ptr>(obj);

	return ptr->set(index(), &old()) ? 1 : 0;
}

Change* Change::with(int idx, const Color &old_, const Color &val) {
	index(idx);
	color(val);

	old(old_);

	return this;
}

int Change::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Change::create(void) {
	Change* result = new Change();

	return result;
}

void Change::destroy(Command* ptr) {
	Change* impl = static_cast<Change*>(ptr);
	delete impl;
}

Cut::Cut() {
}

Cut::~Cut() {
}

unsigned Cut::type(void) const {
	return TYPE();
}

const char* Cut::toString(void) const {
	return "Cut color";
}

Command* Cut::create(void) {
	Cut* result = new Cut();

	return result;
}

void Cut::destroy(Command* ptr) {
	Cut* impl = static_cast<Cut*>(ptr);
	delete impl;
}

Paste::Paste() {
}

Paste::~Paste() {
}

unsigned Paste::type(void) const {
	return TYPE();
}

const char* Paste::toString(void) const {
	return "Paste color";
}

Command* Paste::create(void) {
	Paste* result = new Paste();

	return result;
}

void Paste::destroy(Command* ptr) {
	Paste* impl = static_cast<Paste*>(ptr);
	delete impl;
}

Delete::Delete() {
}

Delete::~Delete() {
}

unsigned Delete::type(void) const {
	return TYPE();
}

const char* Delete::toString(void) const {
	return "Delete color";
}

Command* Delete::create(void) {
	Delete* result = new Delete();

	return result;
}

void Delete::destroy(Command* ptr) {
	Delete* impl = static_cast<Delete*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
