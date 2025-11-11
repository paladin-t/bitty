/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COMMANDS_PAINTABLE_H__
#define __COMMANDS_PAINTABLE_H__

#include "command.h"
#include "editing.h"

/*
** {===========================================================================
** Paintable commands
*/

namespace Commands {

namespace Paintable {

class Paint : public Command {
public:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> Setter;

	typedef std::function<void(int, int)> Plotter;

public:
	BITTY_PROPERTY_READONLY(Getter, get)
	BITTY_PROPERTY_READONLY(Setter, set)
	BITTY_PROPERTY_READONLY(int, width)
	BITTY_PROPERTY(Editing::Point::Set, points)

	BITTY_PROPERTY(Editing::Point::Set, old)

public:
	Paint();
	virtual ~Paint() override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Paint* with(Getter get, Setter set, int width);
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter);
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

protected:
	void plot(int x, int y, Plotter proc);
};

class Pencil : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, lastPosition)

public:
	Pencil();
	virtual ~Pencil() override;

	BITTY_CLASS_TYPE('P', 'P', 'C', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Line : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Line();
	virtual ~Line() override;

	BITTY_CLASS_TYPE('P', 'L', 'N', 'E')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Box : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Box();
	virtual ~Box() override;

	BITTY_CLASS_TYPE('P', 'B', 'O', 'X')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class BoxFill : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	BoxFill();
	virtual ~BoxFill() override;

	BITTY_CLASS_TYPE('P', 'B', 'X', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Ellipse : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	Ellipse();
	virtual ~Ellipse() override;

	BITTY_CLASS_TYPE('P', 'L', 'P', 'S')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class EllipseFill : public Paint {
public:
	BITTY_PROPERTY_READONLY(Math::Vec2i, firstPosition)

public:
	EllipseFill();
	virtual ~EllipseFill() override;

	BITTY_CLASS_TYPE('P', 'L', 'P', 'F')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, const Color &col, Plotter extraPlotter /* nullable */) override;
	virtual Paint* with(const Math::Vec2i &validSize, const Math::Vec2i &pos, int idx, Plotter extraPlotter /* nullable */) override;
	using Paint::with;
};

class Fill : public Paint {
public:
	Fill();
	virtual ~Fill() override;

	BITTY_CLASS_TYPE('P', 'F', 'I', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	Paint* with(Getter get, Setter set);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, const Color &col);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, int idx);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Paint::exec;
};

class Replace : public Paint {
public:
	Replace();
	virtual ~Replace() override;

	BITTY_CLASS_TYPE('P', 'R', 'P', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	Paint* with(Getter get, Setter set);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, const Color &col);
	Paint* with(const Math::Vec2i &validSize, const Math::Recti* selection /* nullable */, const Math::Vec2i &pos, int idx);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Paint::exec;
};

class Blip : public Command {
public:
	typedef std::function<bool(const Math::Vec2i &, Editing::Dot &)> Getter;
	typedef std::function<bool(const Math::Vec2i &, const Editing::Dot &)> Setter;

public:
	BITTY_PROPERTY_READONLY(Getter, get)
	BITTY_PROPERTY_READONLY(Setter, set)
	BITTY_PROPERTY_READONLY(Math::Recti, area)
	BITTY_PROPERTY(Editing::Dot::Array, dots)

	BITTY_PROPERTY(Editing::Dot::Array, old)

public:
	Blip();
	virtual ~Blip() override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	virtual Blip* with(Getter get, Setter set);
	virtual Blip* with(int dir);
	virtual Blip* with(const Math::Recti &area);
	virtual Blip* with(const Math::Recti &area, const Editing::Dot::Array &dots);
	virtual Blip* with(const Math::Recti &area, const Color* col);
	virtual Blip* with(const Math::Recti &area, const int* idx);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;
};

class Stamp : public Blip {
public:
	Stamp();
	virtual ~Stamp() override;

	BITTY_CLASS_TYPE('P', 'S', 'T', 'M')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Rotate : public Blip {
public:
	BITTY_PROPERTY_READONLY(int, direction)

public:
	Rotate();
	virtual ~Rotate() override;

	BITTY_CLASS_TYPE('P', 'R', 'O', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Blip* with(int dir) override;
	virtual Blip* with(const Math::Recti &area) override;
	using Blip::with;

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Blip::exec;
};

class Flip : public Blip {
public:
	BITTY_PROPERTY_READONLY(int, direction)

public:
	Flip();
	virtual ~Flip() override;

	BITTY_CLASS_TYPE('P', 'F', 'L', 'P')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual Blip* with(int dir) override;
	virtual Blip* with(const Math::Recti &area) override;
	using Blip::with;

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Blip::exec;
};

class Cut : public Blip {
public:
	Cut();
	virtual ~Cut() override;

	BITTY_CLASS_TYPE('P', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Paste : public Blip {
public:
	Paste();
	virtual ~Paste() override;

	BITTY_CLASS_TYPE('P', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

class Delete : public Blip {
public:
	Delete();
	virtual ~Delete() override;

	BITTY_CLASS_TYPE('P', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_PAINTABLE_H__ */
