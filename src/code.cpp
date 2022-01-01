/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "code.h"

/*
** {===========================================================================
** Code
*/

class CodeImpl : public Code {
private:
	std::string _text;

public:
	CodeImpl() {
	}
	virtual ~CodeImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual const char* text(size_t* len) const override {
		if (len)
			*len = _text.length();

		if (_text.empty())
			return "";

		return _text.c_str();
	}
	virtual void text(const char* txt, size_t len) override {
		if (txt && len)
			_text.assign(txt, len);
		else
			_text.assign(txt);
	}
};

Code* Code::create(void) {
	CodeImpl* result = new CodeImpl();

	return result;
}

void Code::destroy(Code* ptr) {
	CodeImpl* impl = static_cast<CodeImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
