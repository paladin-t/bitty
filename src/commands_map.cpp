/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_map.h"
#include "map.h"
#include "../lib/lz4/lib/lz4.h"

/*
** {===========================================================================
** Map commands
*/

namespace Commands {

namespace Map {

Pencil::Pencil() {
}

Pencil::~Pencil() {
}

Command* Pencil::create(void) {
	Pencil* result = new Pencil();

	return result;
}

void Pencil::destroy(Command* ptr) {
	Pencil* impl = static_cast<Pencil*>(ptr);
	delete impl;
}

Line::Line() {
}

Line::~Line() {
}

Command* Line::create(void) {
	Line* result = new Line();

	return result;
}

void Line::destroy(Command* ptr) {
	Line* impl = static_cast<Line*>(ptr);
	delete impl;
}

Box::Box() {
}

Box::~Box() {
}

Command* Box::create(void) {
	Box* result = new Box();

	return result;
}

void Box::destroy(Command* ptr) {
	Box* impl = static_cast<Box*>(ptr);
	delete impl;
}

BoxFill::BoxFill() {
}

BoxFill::~BoxFill() {
}

Command* BoxFill::create(void) {
	BoxFill* result = new BoxFill();

	return result;
}

void BoxFill::destroy(Command* ptr) {
	BoxFill* impl = static_cast<BoxFill*>(ptr);
	delete impl;
}

Ellipse::Ellipse() {
}

Ellipse::~Ellipse() {
}

Command* Ellipse::create(void) {
	Ellipse* result = new Ellipse();

	return result;
}

void Ellipse::destroy(Command* ptr) {
	Ellipse* impl = static_cast<Ellipse*>(ptr);
	delete impl;
}

EllipseFill::EllipseFill() {
}

EllipseFill::~EllipseFill() {
}

Command* EllipseFill::create(void) {
	EllipseFill* result = new EllipseFill();

	return result;
}

void EllipseFill::destroy(Command* ptr) {
	EllipseFill* impl = static_cast<EllipseFill*>(ptr);
	delete impl;
}

Fill::Fill() {
}

Fill::~Fill() {
}

Command* Fill::create(void) {
	Fill* result = new Fill();

	return result;
}

void Fill::destroy(Command* ptr) {
	Fill* impl = static_cast<Fill*>(ptr);
	delete impl;
}

Replace::Replace() {
}

Replace::~Replace() {
}

Command* Replace::create(void) {
	Replace* result = new Replace();

	return result;
}

void Replace::destroy(Command* ptr) {
	Replace* impl = static_cast<Replace*>(ptr);
	delete impl;
}

Stamp::Stamp() {
}

Stamp::~Stamp() {
}

Command* Stamp::create(void) {
	Stamp* result = new Stamp();

	return result;
}

void Stamp::destroy(Command* ptr) {
	Stamp* impl = static_cast<Stamp*>(ptr);
	delete impl;
}

Rotate::Rotate() {
}

Rotate::~Rotate() {
}

Command* Rotate::create(void) {
	Rotate* result = new Rotate();

	return result;
}

void Rotate::destroy(Command* ptr) {
	Rotate* impl = static_cast<Rotate*>(ptr);
	delete impl;
}

Flip::Flip() {
}

Flip::~Flip() {
}

Command* Flip::create(void) {
	Flip* result = new Flip();

	return result;
}

void Flip::destroy(Command* ptr) {
	Flip* impl = static_cast<Flip*>(ptr);
	delete impl;
}

Cut::Cut() {
}

Cut::~Cut() {
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

Command* Delete::create(void) {
	Delete* result = new Delete();

	return result;
}

void Delete::destroy(Command* ptr) {
	Delete* impl = static_cast<Delete*>(ptr);
	delete impl;
}

Resize::Resize() {
	bytes(0);
}

Resize::~Resize() {
}

unsigned Resize::type(void) const {
	return TYPE();
}

const char* Resize::toString(void) const {
	return "Resize";
}

int Resize::redo(Object::Ptr obj, int, const Variant*) {
	::Map::Ptr map = Object::as<::Map::Ptr>(obj);

	if (!old()) {
		old(Bytes::Ptr(Bytes::create()));

		Bytes::Ptr tmp(Bytes::create());
		rapidjson::Document doc;
		map->toJson(doc);
		std::string str;
		Json::toString(doc, str, false);
		tmp->writeString(str);
		bytes((int)tmp->count());
		int n = LZ4_compressBound((int)tmp->count());
		old()->resize((size_t)n);
		n = LZ4_compress_default(
			(const char*)tmp->pointer(), (char*)old()->pointer(),
			(int)tmp->count(), (int)old()->count()
		);
		assert(n);
		if (n < (int)tmp->count()) {
			old()->resize((size_t)n);
		} else {
			bytes(0);
			old()->clear();
			old()->writeBytes(tmp.get());
		}
	}
	map->resize(size().x, size().y);

	return 1;
}

int Resize::undo(Object::Ptr obj, int, const Variant*) {
	::Map::Ptr map = Object::as<::Map::Ptr>(obj);

	if (bytes()) {
		Bytes::Ptr tmp(Bytes::create());
		tmp->resize(bytes());
		const int n = LZ4_decompress_safe(
			(const char*)old()->pointer(), (char*)tmp->pointer(),
			(int)old()->count(), (int)tmp->count()
		);
		(void)n;
		assert(n == (int)bytes());
		Texture::Ptr texture = nullptr;
		::Map::Tiles tiles;
		if (map->tiles(tiles))
			texture = tiles.texture;
		std::string str;
		tmp->readString(str);
		rapidjson::Document doc;
		Json::fromString(doc, str.c_str());
		map->fromJson(texture, doc);
	} else {
		Texture::Ptr texture = nullptr;
		::Map::Tiles tiles;
		if (map->tiles(tiles))
			texture = tiles.texture;
		std::string str;
		old()->poke(0);
		old()->readString(str);
		rapidjson::Document doc;
		Json::fromString(doc, str.c_str());
		map->fromJson(texture, doc);
	}

	return 1;
}

Resize* Resize::with(const Math::Vec2i &size_) {
	size(size_);

	return this;
}

int Resize::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* Resize::create(void) {
	Resize* result = new Resize();

	return result;
}

void Resize::destroy(Command* ptr) {
	Resize* impl = static_cast<Resize*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
