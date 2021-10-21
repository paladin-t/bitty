/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __SCRIPTING_H__
#define __SCRIPTING_H__

#include "bitty.h"
#include "executable.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef SCRIPTING_SETUP_FUNCTION_NAME
#	define SCRIPTING_SETUP_FUNCTION_NAME "setup"
#endif /* SCRIPTING_SETUP_FUNCTION_NAME */
#ifndef SCRIPTING_UPDATE_FUNCTION_NAME
#	define SCRIPTING_UPDATE_FUNCTION_NAME "update"
#endif /* SCRIPTING_UPDATE_FUNCTION_NAME */
#ifndef SCRIPTING_QUIT_FUNCTION_NAME
#	define SCRIPTING_QUIT_FUNCTION_NAME "quit"
#endif /* SCRIPTING_QUIT_FUNCTION_NAME */
#ifndef SCRIPTING_FOCUS_LOST_FUNCTION_NAME
#	define SCRIPTING_FOCUS_LOST_FUNCTION_NAME "focusLost"
#endif /* SCRIPTING_FOCUS_LOST_FUNCTION_NAME */
#ifndef SCRIPTING_FOCUS_GAINED_FUNCTION_NAME
#	define SCRIPTING_FOCUS_GAINED_FUNCTION_NAME "focusGained"
#endif /* SCRIPTING_FOCUS_GAINED_FUNCTION_NAME */
#ifndef SCRIPTING_RENDERER_RESET_FUNCTION_NAME
#	define SCRIPTING_RENDERER_RESET_FUNCTION_NAME "rendererReset"
#endif /* SCRIPTING_RENDERER_RESET_FUNCTION_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Scripting
*/

/**
 * @brief Scripting driven executable object.
 */
class Scripting : public Executable, public NonCopyable, public virtual Object {
public:
	typedef std::shared_ptr<Scripting> Ptr;

public:
	BITTY_PROPERTY_READONLY_PTR(Observer, observer) // Foreign.

protected:
	bool _opened = false;

	bool _effectsEnabled = false;

	const class Project* _project = nullptr; // Foreign.
	const class Project* _editing = nullptr; // Foreign.
	class Primitives* _primitives = nullptr; // Foreign.

public:
	Scripting();
	virtual ~Scripting() override;

	BITTY_CLASS_TYPE('S', 'C', 'P', 'T')

	virtual unsigned type(void) const override;

	virtual bool open(
		Observer* obsvr,
		const class Project* project,
		const class Project* editing,
		class Primitives* primitives,
		bool effectsEnabled
	) override;
	virtual bool close(void) override;

	virtual bool effectsEnabled(void) const override;

	virtual const class Project* project(void) const override;
	virtual const class Project* editing(void) const override;
	virtual class Primitives* primitives(void) const override;

	static Scripting* create(Languages language);
	static void destroy(Executable* ptr);
};

/* ===========================================================================} */

#endif /* __SCRIPTING_H__ */
