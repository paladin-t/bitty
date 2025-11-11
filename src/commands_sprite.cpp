/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "commands_sprite.h"
#include "sprite.h"

/*
** {===========================================================================
** Sprite commands
*/

namespace Commands {

namespace Sprite {

AddAnimation::AddAnimation() {
	index(-1);
	interval(std::numeric_limits<double>::quiet_NaN());
}

AddAnimation::~AddAnimation() {
}

unsigned AddAnimation::type(void) const {
	return TYPE();
}

const char* AddAnimation::toString(void) const {
	return "Add animation";
}

int AddAnimation::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	Object::Ptr arg0 = unpack<Object::Ptr>(argc, argv, 0, nullptr);
	Texture::Ptr tex = Object::as<Texture::Ptr>(arg0);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	const char* key = animation().empty() ? nullptr : animation().c_str();
	ptr->add(tex, &area(), &interval(), key);
	if (index() < 0) {
		index(ptr->count() - 1);
	}

	if (arg1)
		*((bool*)arg1) = true;

	return 1;
}

int AddAnimation::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	if (!ptr->remove(index(), nullptr, nullptr, nullptr, nullptr))
		return 0;

	if (arg1)
		*((bool*)arg1) = true;

	return 1;
}

AddAnimation* AddAnimation::with(const char* anim, const Math::Recti &area_, double interval_) {
	animation(anim);
	area(area_);
	interval(interval_);

	return this;
}

int AddAnimation::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddAnimation::create(void) {
	AddAnimation* result = new AddAnimation();

	return result;
}

void AddAnimation::destroy(Command* ptr) {
	AddAnimation* impl = static_cast<AddAnimation*>(ptr);
	delete impl;
}

CutAnimation::CutAnimation() {
	index(-1);

	filled(false);
}

CutAnimation::~CutAnimation() {
}

unsigned CutAnimation::type(void) const {
	return TYPE();
}

const char* CutAnimation::toString(void) const {
	return "Cut animation";
}

int CutAnimation::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	int result = 0;

	if (!filled()) {
		const ::Sprite::Range range = ptr->rangeOf(animation());
		const int beginIdx = std::get<0>(range);
		const int endIdx = std::get<1>(range);
		for (int i = beginIdx; i <= endIdx; ++i) {
			Math::Recti area;
			double interval = 0;
			const char* key = nullptr;
			ptr->get(i, nullptr, &area, &interval, &key);
			old().push_back(Frame(area, interval, key));
		}
		index(beginIdx);
		filled(true);
	}
	for (int i = index() + (int)old().size() - 1; i >= index(); --i) {
		if (ptr->remove(i, nullptr, nullptr, nullptr, nullptr))
			++result;
	}

	if (arg1)
		*((bool*)arg1) = true;

	return result;
}

int CutAnimation::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	Object::Ptr arg0 = unpack<Object::Ptr>(argc, argv, 0, nullptr);
	Texture::Ptr tex = Object::as<Texture::Ptr>(arg0);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	int result = 0;

	for (int i = (int)old().size() - 1; i >= 0; --i) {
		const Frame &f = old()[i];
		ptr->insert(index(), tex, &f.area, &f.interval, f.key.c_str());
		++result;
	}

	if (arg1)
		*((bool*)arg1) = true;

	return result;
}

CutAnimation* CutAnimation::with(const char* anim) {
	animation(anim);

	return this;
}

int CutAnimation::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* CutAnimation::create(void) {
	CutAnimation* result = new CutAnimation();

	return result;
}

void CutAnimation::destroy(Command* ptr) {
	CutAnimation* impl = static_cast<CutAnimation*>(ptr);
	delete impl;
}

PasteAnimation::PasteAnimation() {
}

PasteAnimation::~PasteAnimation() {
}

unsigned PasteAnimation::type(void) const {
	return TYPE();
}

const char* PasteAnimation::toString(void) const {
	return "Paste animation";
}

Command* PasteAnimation::create(void) {
	PasteAnimation* result = new PasteAnimation();

	return result;
}

void PasteAnimation::destroy(Command* ptr) {
	PasteAnimation* impl = static_cast<PasteAnimation*>(ptr);
	delete impl;
}

DeleteAnimation::DeleteAnimation() {
}

DeleteAnimation::~DeleteAnimation() {
}

unsigned DeleteAnimation::type(void) const {
	return TYPE();
}

const char* DeleteAnimation::toString(void) const {
	return "Delete animation";
}

Command* DeleteAnimation::create(void) {
	DeleteAnimation* result = new DeleteAnimation();

	return result;
}

void DeleteAnimation::destroy(Command* ptr) {
	DeleteAnimation* impl = static_cast<DeleteAnimation*>(ptr);
	delete impl;
}

RenameAnimation::RenameAnimation() {
	index(-1);

	filled(false);
}

RenameAnimation::~RenameAnimation() {
}

unsigned RenameAnimation::type(void) const {
	return TYPE();
}

const char* RenameAnimation::toString(void) const {
	return "Rename animation";
}

int RenameAnimation::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	if (!filled()) {
		const int idx = ptr->indexOf(old());
		index(idx);
		filled(true);
	}
	if (!ptr->set(index(), nullptr, nullptr, animation().c_str()))
		return 0;

	if (arg1)
		*((bool*)arg1) = true;

	return 1;
}

int RenameAnimation::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	void* arg1 = unpack<void*>(argc, argv, 1, nullptr);

	if (!ptr->set(index(), nullptr, nullptr, old().c_str()))
		return 0;

	if (arg1)
		*((bool*)arg1) = true;

	return 1;
}

RenameAnimation* RenameAnimation::with(const char* anim, const char* newAnim) {
	animation(newAnim);
	old(anim);

	return this;
}

int RenameAnimation::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* RenameAnimation::create(void) {
	RenameAnimation* result = new RenameAnimation();

	return result;
}

void RenameAnimation::destroy(Command* ptr) {
	RenameAnimation* impl = static_cast<RenameAnimation*>(ptr);
	delete impl;
}

AddFrame::AddFrame() {
	index(-1);
	interval(std::numeric_limits<double>::quiet_NaN());
	append(true);
}

AddFrame::~AddFrame() {
}

unsigned AddFrame::type(void) const {
	return TYPE();
}

const char* AddFrame::toString(void) const {
	return "Add frame";
}

int AddFrame::redo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	Object::Ptr arg0 = unpack<Object::Ptr>(argc, argv, 0, nullptr);
	Texture::Ptr tex = Object::as<Texture::Ptr>(arg0);

	const char* key = nullptr;
	if (!append())
		ptr->get(index(), nullptr, nullptr, nullptr, &key);
	if (!ptr->insert(index(), tex, &area(), &interval(), key))
		return 0;
	if (key && *key)
		ptr->set(index() + 1, nullptr, nullptr, "");

	return 1;
}

int AddFrame::undo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	std::string key;
	if (!ptr->remove(index(), nullptr, nullptr, nullptr, &key))
		return 0;
	if (!key.empty()) {
		const char* anotherKey = nullptr;
		if (ptr->get(index(), nullptr, nullptr, nullptr, &anotherKey) && !(*anotherKey))
			ptr->set(index(), nullptr, nullptr, key.c_str());
	}

	return 1;
}

AddFrame* AddFrame::with(int idx, const Math::Recti &area_, double interval_, bool append_) {
	index(idx);
	area(area_);
	interval(interval_);
	append(append_);

	return this;
}

int AddFrame::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* AddFrame::create(void) {
	AddFrame* result = new AddFrame();

	return result;
}

void AddFrame::destroy(Command* ptr) {
	AddFrame* impl = static_cast<AddFrame*>(ptr);
	delete impl;
}

CutFrame::CutFrame() {
	index(-1);
	interval(std::numeric_limits<double>::quiet_NaN());

	filled(false);
}

CutFrame::~CutFrame() {
}

unsigned CutFrame::type(void) const {
	return TYPE();
}

const char* CutFrame::toString(void) const {
	return "Cut frame";
}

int CutFrame::redo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	std::string key;
	if (filled()) {
		if (!ptr->remove(index(), nullptr, nullptr, nullptr, &key))
			return 0;
	} else {
		Math::Recti area_;
		double intvl = 0;
		if (!ptr->remove(index(), nullptr, &area_, &intvl, &key))
			return 0;

		area(area_);
		interval(intvl);

		filled(true);
	}
	if (!key.empty()) {
		const char* anotherKey = nullptr;
		if (ptr->get(index(), nullptr, nullptr, nullptr, &anotherKey) && !(*anotherKey))
			ptr->set(index(), nullptr, nullptr, key.c_str());
	}

	return 1;
}

int CutFrame::undo(Object::Ptr obj, int argc, const Variant* argv) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);
	Object::Ptr arg0 = unpack<Object::Ptr>(argc, argv, 0, nullptr);
	Texture::Ptr tex = Object::as<Texture::Ptr>(arg0);

	const char* key = nullptr;
	if (!append())
		ptr->get(index(), nullptr, nullptr, nullptr, &key);
	if (!ptr->insert(index(), tex, &area(), &interval(), key))
		return 0;
	if (key)
		ptr->set(index() + 1, nullptr, nullptr, "");

	return 1;
}

CutFrame* CutFrame::with(int idx, bool append_) {
	index(idx);
	append(append_);

	return this;
}

int CutFrame::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* CutFrame::create(void) {
	CutFrame* result = new CutFrame();

	return result;
}

void CutFrame::destroy(Command* ptr) {
	CutFrame* impl = static_cast<CutFrame*>(ptr);
	delete impl;
}

PasteFrame::PasteFrame() {
}

PasteFrame::~PasteFrame() {
}

unsigned PasteFrame::type(void) const {
	return TYPE();
}

const char* PasteFrame::toString(void) const {
	return "Paste frame";
}

Command* PasteFrame::create(void) {
	PasteFrame* result = new PasteFrame();

	return result;
}

void PasteFrame::destroy(Command* ptr) {
	PasteFrame* impl = static_cast<PasteFrame*>(ptr);
	delete impl;
}

DeleteFrame::DeleteFrame() {
}

DeleteFrame::~DeleteFrame() {
}

unsigned DeleteFrame::type(void) const {
	return TYPE();
}

const char* DeleteFrame::toString(void) const {
	return "Delete frame";
}

Command* DeleteFrame::create(void) {
	DeleteFrame* result = new DeleteFrame();

	return result;
}

void DeleteFrame::destroy(Command* ptr) {
	DeleteFrame* impl = static_cast<DeleteFrame*>(ptr);
	delete impl;
}

ChangeArea::ChangeArea() {
	index(-1);

	filled(false);
}

ChangeArea::~ChangeArea() {
}

unsigned ChangeArea::type(void) const {
	return TYPE();
}

const char* ChangeArea::toString(void) const {
	return "Set area";
}

int ChangeArea::redo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	if (!filled()) {
		Math::Recti old_;
		ptr->get(index(), nullptr, &old_, nullptr, nullptr);
		old(old_);
		filled(true);
	}
	if (!ptr->set(index(), &area(), nullptr, nullptr))
		return 0;

	return 1;
}

int ChangeArea::undo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	if (!ptr->set(index(), &old(), nullptr, nullptr))
		return 0;

	return 1;
}

ChangeArea* ChangeArea::with(int idx, const Math::Recti &area_) {
	index(idx);
	area(area_);

	return this;
}

int ChangeArea::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ChangeArea::create(void) {
	ChangeArea* result = new ChangeArea();

	return result;
}

void ChangeArea::destroy(Command* ptr) {
	ChangeArea* impl = static_cast<ChangeArea*>(ptr);
	delete impl;
}

ChangeInterval::ChangeInterval() {
	index(-1);

	filled(false);
}

ChangeInterval::~ChangeInterval() {
}

unsigned ChangeInterval::type(void) const {
	return TYPE();
}

const char* ChangeInterval::toString(void) const {
	return "Set interval";
}

int ChangeInterval::redo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	if (!filled()) {
		double old_ = 0;
		ptr->get(index(), nullptr, nullptr, &old_, nullptr);
		old(old_);
		filled(true);
	}
	if (!ptr->set(index(), nullptr, &interval(), nullptr))
		return 0;

	return 1;
}

int ChangeInterval::undo(Object::Ptr obj, int, const Variant*) {
	::Sprite::Ptr ptr = Object::as<::Sprite::Ptr>(obj);

	if (!ptr->set(index(), nullptr, &old(), nullptr))
		return 0;

	return 1;
}

ChangeInterval* ChangeInterval::with(int idx, double interval_) {
	index(idx);

	interval(interval_);

	return this;
}

int ChangeInterval::exec(Object::Ptr obj, int argc, const Variant* argv) {
	return redo(obj, argc, argv);
}

Command* ChangeInterval::create(void) {
	ChangeInterval* result = new ChangeInterval();

	return result;
}

void ChangeInterval::destroy(Command* ptr) {
	ChangeInterval* impl = static_cast<ChangeInterval*>(ptr);
	delete impl;
}

}

}

/* ===========================================================================} */
