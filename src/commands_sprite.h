/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COMMANDS_SPRITE_H__
#define __COMMANDS_SPRITE_H__

#include "command.h"
#include "editing.h"

/*
** {===========================================================================
** Sprite commands
*/

namespace Commands {

namespace Sprite {

class Cursorial : public Command {
public:
	BITTY_PROPERTY(Editing::Frame, cursor)
};

class AddAnimation : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(std::string, animation)
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(Math::Recti, area)
	BITTY_PROPERTY_READONLY(double, interval)

public:
	AddAnimation();
	virtual ~AddAnimation() override;

	BITTY_CLASS_TYPE('S', 'A', 'D', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	AddAnimation* with(const char* anim, const Math::Recti &area, double interval);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class CutAnimation : public Cursorial {
private:
	struct Frame {
		Math::Recti area;
		double interval = 0;
		std::string key;

		Frame() {
		}
		Frame(Math::Recti area_, double interval_, const char* key_) : area(area_), interval(interval_) {
			if (key_)
				key = key_;
		}
	};
	typedef std::deque<Frame> Frames;

public:
	BITTY_PROPERTY_READONLY(std::string, animation)
	BITTY_PROPERTY_READONLY(int, index)

	BITTY_PROPERTY_READONLY(bool, filled)
	BITTY_PROPERTY(Frames, old)

public:
	CutAnimation();
	virtual ~CutAnimation() override;

	BITTY_CLASS_TYPE('S', 'C', 'T', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	CutAnimation* with(const char* anim);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class PasteAnimation : public AddAnimation {
public:
	PasteAnimation();
	virtual ~PasteAnimation() override;

	BITTY_CLASS_TYPE('S', 'P', 'T', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteAnimation : public CutAnimation {
public:
	DeleteAnimation();
	virtual ~DeleteAnimation() override;

	BITTY_CLASS_TYPE('S', 'D', 'L', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class RenameAnimation : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(std::string, animation)
	BITTY_PROPERTY_READONLY(int, index)

	BITTY_PROPERTY_READONLY(bool, filled)
	BITTY_PROPERTY_READONLY(std::string, old)

public:
	RenameAnimation();
	virtual ~RenameAnimation() override;

	BITTY_CLASS_TYPE('S', 'R', 'N', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	RenameAnimation* with(const char* anim, const char* newAnim);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class AddFrame : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(Math::Recti, area)
	BITTY_PROPERTY_READONLY(double, interval)
	BITTY_PROPERTY_READONLY(bool, append)

public:
	AddFrame();
	virtual ~AddFrame() override;

	BITTY_CLASS_TYPE('S', 'A', 'D', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	AddFrame* with(int idx, const Math::Recti &area, double interval, bool append);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class CutFrame : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(Math::Recti, area)
	BITTY_PROPERTY_READONLY(double, interval)
	BITTY_PROPERTY_READONLY(bool, append)

	BITTY_PROPERTY_READONLY(bool, filled)

public:
	CutFrame();
	virtual ~CutFrame() override;

	BITTY_CLASS_TYPE('S', 'C', 'T', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	CutFrame* with(int idx, bool append);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class PasteFrame : public AddFrame {
public:
	PasteFrame();
	virtual ~PasteFrame() override;

	BITTY_CLASS_TYPE('S', 'P', 'T', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class DeleteFrame : public CutFrame {
public:
	DeleteFrame();
	virtual ~DeleteFrame() override;

	BITTY_CLASS_TYPE('S', 'D', 'L', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeArea : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(Math::Recti, area)

	BITTY_PROPERTY_READONLY(bool, filled)
	BITTY_PROPERTY_READONLY(Math::Recti, old)

public:
	ChangeArea();
	virtual ~ChangeArea() override;

	BITTY_CLASS_TYPE('S', 'C', 'G', 'A')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	ChangeArea* with(int idx, const Math::Recti &area);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class ChangeInterval : public Cursorial {
public:
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(double, interval)

	BITTY_PROPERTY_READONLY(bool, filled)
	BITTY_PROPERTY_READONLY(double, old)

public:
	ChangeInterval();
	virtual ~ChangeInterval() override;

	BITTY_CLASS_TYPE('S', 'C', 'G', 'I')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	ChangeInterval* with(int idx, double interval);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_SPRITE_H__ */
