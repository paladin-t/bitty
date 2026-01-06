/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __COMMANDS_PALETTE_H__
#define __COMMANDS_PALETTE_H__

#include "color.h"
#include "command.h"

/*
** {===========================================================================
** Palette commands
*/

namespace Commands {

namespace Palette {

class Change : public Command {
public:
	BITTY_PROPERTY_READONLY(int, index)
	BITTY_PROPERTY_READONLY(Color, color)

	BITTY_PROPERTY_READONLY(Color, old)

public:
	Change();
	virtual ~Change() override;

	BITTY_CLASS_TYPE('P', 'C', 'N', 'G')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	virtual int redo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::redo;
	virtual int undo(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::undo;

	Change* with(int idx, const Color &old, const Color &val);

	virtual int exec(Object::Ptr obj, int argc, const Variant* argv) override;
	using Command::exec;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Cut : public Change {
public:
	Cut();
	virtual ~Cut() override;

	BITTY_CLASS_TYPE('P', 'C', 'U', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Paste : public Change {
public:
	Paste();
	virtual ~Paste() override;

	BITTY_CLASS_TYPE('P', 'P', 'S', 'T')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

class Delete : public Change {
public:
	Delete();
	virtual ~Delete() override;

	BITTY_CLASS_TYPE('P', 'D', 'E', 'L')

	virtual unsigned type(void) const override;

	virtual const char* toString(void) const override;

	static Command* create(void);
	static void destroy(Command* ptr);
};

}

}

/* ===========================================================================} */

#endif /* __COMMANDS_PALETTE_H__ */
