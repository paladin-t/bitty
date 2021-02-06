/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __ENTRY_H__
#define __ENTRY_H__

#include "bitty.h"
#include "text.h"

/*
** {===========================================================================
** Entry
*/

/**
 * @brief Entry.
 */
class Entry {
public:
	typedef std::map<Entry, std::string> Dictionary;

	struct Stub {
		const Text::Array &parts;

		Stub(const Text::Array &data);
		Stub(const Entry &data);
	};

private:
	std::string _name;
	Text::Array _parts;

public:
	Entry();
	Entry(const char* name);
	Entry(const std::string &name);

	bool operator < (const Entry &other) const;

	bool empty(void) const;
	void clear(void);

	const std::string &name(void) const;
	const Text::Array &parts(void) const;

	const char* c_str(void) const;

	static int compare(const Stub &left, const Stub &right, const std::string* priority = nullptr);
};

/* ===========================================================================} */

#endif /* __ENTRY_H__ */
