/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "network.h"
#include "network_mongoose.h"

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

Network* Network::create(void) {
	NetworkMongoose* result = new NetworkMongoose();

	return result;
}

void Network::destroy(Network* ptr) {
	NetworkMongoose* impl = static_cast<NetworkMongoose*>(ptr);
	delete impl;
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */
