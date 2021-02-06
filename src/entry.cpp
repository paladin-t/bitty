/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "entry.h"
#include "generic.h"

/*
** {===========================================================================
** Asset
*/

Entry::Stub::Stub(const Text::Array &data) : parts(data) {
}

Entry::Stub::Stub(const Entry &data) : parts(data.parts()) {
}

Entry::Entry() {
}

Entry::Entry(const char* name) {
	_name = name;

	_parts = Text::split(_name, "/");
}

Entry::Entry(const std::string &name) {
	_name = name;

	_parts = Text::split(_name, "/");
}

bool Entry::operator < (const Entry &other) const {
	return compare(*this, other, nullptr) < 0;
}

bool Entry::empty(void) const {
	return _name.empty();
}

void Entry::clear(void) {
	_name.clear();
	_parts.clear();
}

const std::string &Entry::name(void) const {
	return _name;
}

const Text::Array &Entry::parts(void) const {
	return _parts;
}

const char* Entry::c_str(void) const {
	return _name.c_str();
}

int Entry::compare(const Stub &left_, const Stub &right_, const std::string* priority) {
	const Text::Array &left = left_.parts;
	const Text::Array &right = right_.parts;
	if (priority) {
		if ((left.size() == 1 && left.front() == *priority) && (right.size() == 1 && right.front() == *priority))
			return 0;
		if (left.size() == 1 && left.front() == *priority)
			return -1;
		else if (right.size() == 1 && right.front() == *priority)
			return 1;
	}

	return Compare::doc(
		left.begin(), left.end(), right.begin(), right.end(),
		[] (std::string lstr, std::string rstr) -> int {
			Text::toLowerCase(lstr); // Case-insensitive comparison.
			Text::toLowerCase(rstr);

			return Compare::lex(
				lstr.begin(), lstr.end(), rstr.begin(), rstr.end(),
				[] (char lch, char rch) -> int {
					if (lch < rch)
						return -1;
					else if (lch > rch)
						return 1;

					return 0;
				}
			);
		}
	);
}

/* ===========================================================================} */
