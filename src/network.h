/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "bitty.h"
#include "object.h"
#include "plus.h"
#include "updatable.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef NETWORK_TIMEOUT_SECONDS
#	define NETWORK_TIMEOUT_SECONDS 5
#endif /* NETWORK_TIMEOUT_SECONDS */

#ifndef NETWORK_MESSAGE_MAX_SIZE
#	define NETWORK_MESSAGE_MAX_SIZE (512 * 1024)
#endif /* NETWORK_MESSAGE_MAX_SIZE */

/* ===========================================================================} */

/*
** {===========================================================================
** Network
*/

#if BITTY_NETWORK_ENABLED

/**
 * @brief Network object.
 */
class Network : public Updatable, public virtual Object {
public:
	typedef std::shared_ptr<Network> Ptr;

	enum DataTypes {
		STREAM,
		BYTES,
		STRING,
		JSON,
		RAW
	};

	enum Protocols {
		NONE = 0,
		UDP = 1 << 0,
		TCP = 1 << 1,
		WEBSOCKET = 1 << 2,
		ALL = UDP | TCP | WEBSOCKET
	};

	enum States {
		IDLE,
		READY,
		FAILED
	};

	typedef uint32_t BytesSize;

	typedef struct AddressName {
		char text[64]; // Enough for even IPv6 address and port.
	} AddressName;

	struct ReceivedHandler : public Handler<ReceivedHandler, void, ReceivedHandler*, void*, size_t, const char*> {
		using Handler::Handler;
	};
	struct EstablishedHandler : public Handler<EstablishedHandler, void, EstablishedHandler*, const char* /* nullable */> {
		using Handler::Handler;
	};
	struct DisconnectedHandler : public Handler<DisconnectedHandler, void, DisconnectedHandler*, const char*> {
		using Handler::Handler;
	};

public:
	BITTY_CLASS_TYPE('N', 'E', 'T', 'W')

	virtual bool open(const char* addr, Protocols protocol, bool* toconn /* nullable */, bool* tobind /* nullable */) = 0;
	virtual bool close(void) = 0;

	virtual DataTypes dataType(void) const = 0;
	virtual void dataType(DataTypes y) = 0;

	virtual std::string option(const std::string &key) const = 0;
	virtual void option(const std::string &key, const std::string &val) = 0;

	virtual bool ready(void) const = 0;

	virtual bool polling(void) const = 0;
	virtual bool connective(void) const = 0;

	virtual void poll(int timeoutMs) = 0;
	virtual void establish(void) = 0;
	virtual void disconnect(void) = 0;

	virtual bool send(void* ptr, size_t sz, DataTypes y) = 0;
	virtual bool broadcast(void* ptr, size_t sz, DataTypes y, bool filterPolling) = 0;

	virtual const ReceivedHandler &receivedCallback(void) const = 0;
	virtual const EstablishedHandler &establishedCallback(void) const = 0;
	virtual const DisconnectedHandler &disconnectedCallback(void) const = 0;
	virtual void callback(const ReceivedHandler &cb /* nullable */) = 0;
	virtual void callback(const EstablishedHandler &cb /* nullable */) = 0;
	virtual void callback(const DisconnectedHandler &cb /* nullable */) = 0;
	virtual void callback(const ReceivedHandler &recvCb /* nullable */, const EstablishedHandler &stbCb /* nullable */, const DisconnectedHandler &dscnCb /* nullable */) = 0;

	static Network* create(void);
	static void destroy(Network* ptr);
};

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

#endif /* __NETWORK_H__ */
