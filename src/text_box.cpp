/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "text_box.h"

/*
** {===========================================================================
** Text box
*/

class TextBoxImpl : public TextBox {
public:
	virtual ~TextBoxImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}
};

TextBox* TextBox::create(void) {
	TextBoxImpl* result = new TextBoxImpl();

	return result;
}

void TextBox::destroy(TextBox* ptr) {
	TextBoxImpl* impl = static_cast<TextBoxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
