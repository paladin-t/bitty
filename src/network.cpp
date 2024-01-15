/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "network.h"
#include "network_libuv.h"

/*
** {===========================================================================
** Macros and constants
*/

#if !BITTY_NETWORK_ENABLED
#	pragma message("Network disabled.")
#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Network
*/

#if BITTY_NETWORK_ENABLED

Network* Network::create(const char* type) {
	Network* result = nullptr;
	if (strcmp(type, "libuv") == 0 || strcmp(type, "default") == 0) {
		result = new NetworkLibuv();
	} else {
		assert(false && "Unknown backend type.");
	}

	return result;
}

void Network::destroy(Network* ptr) {
	switch (ptr->type()) {
	case NetworkLibuv::TYPE(): {
			NetworkLibuv* impl = static_cast<NetworkLibuv*>(ptr);
			delete impl;
		}

		break;
	default:
		assert(false && "Unknown backend type.");

		break;
	}
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */
