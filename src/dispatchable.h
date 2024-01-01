/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __DISPATCHABLE_H__
#define __DISPATCHABLE_H__

#include "bitty.h"
#include "object.h"

/*
** {===========================================================================
** Dispatchable
*/

/**
 * @brief Dispatchable interface.
 */
class Dispatchable {
public:
	virtual Variant post(unsigned msg, int argc, const Variant* argv) = 0;
	Variant post(unsigned msg) {
		return post(msg, 0, (const Variant*)nullptr);
	}
	template<typename ...Args> Variant post(unsigned msg, const Args &...args) {
		constexpr const size_t n = sizeof...(Args);
		const Variant argv[n] = { Variant(args)... };

		return post(msg, (int)n, argv);
	}

protected:
	template<typename Arg> static Arg unpack(int argc, const Variant* argv, int idx, Arg default_) {
		return (0 <= idx && idx < argc && argv) ? (Arg)argv[idx] : default_;
	}
};

/* ===========================================================================} */

#endif /* __DISPATCHABLE_H__ */
