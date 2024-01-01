/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "editable.h"

/*
** {===========================================================================
** Editor
*/

class Editor : public Editable {
protected:
	struct Ref {
	private:
		bool _verticalScrollBarVisible = false;

	public:
		float windowWidth(float exp);
		int windowFlags(void) const;
		void windowResized(void);
	};

	typedef std::pair<float, float> Splitter;

protected:
	static Splitter split(void);
};

/* ===========================================================================} */

#endif /* __EDITOR_H__ */
