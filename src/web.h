/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WEB_H__
#define __WEB_H__

#include "bitty.h"
#include "plus.h"
#include "text.h"
#include "updatable.h"

/*
** {===========================================================================
** Fetch
*/

#if BITTY_WEB_ENABLED

/**
 * @brief Fetch object as HTTP client.
 */
class Fetch : public Updatable, public virtual Object {
public:
	typedef std::shared_ptr<Fetch> Ptr;

	enum DataTypes {
		BYTES,
		STRING,
		JSON
	};

	struct RespondedHandler : public Handler<RespondedHandler, void, RespondedHandler*, const Byte*, size_t> {
		using Handler::Handler;
	};
	struct ErrorHandler : public Handler<ErrorHandler, void, ErrorHandler*, const char*> {
		using Handler::Handler;
	};

public:
	BITTY_CLASS_TYPE('F', 'E', 'C', 'H')

	virtual bool open(void) = 0;
	virtual bool close(void) = 0;

	virtual DataTypes dataType(void) const = 0;
	virtual void dataType(DataTypes y) = 0;

	virtual void url(const char* url) = 0;
	virtual void options(const Variant &options) = 0;
	virtual void headers(const Text::Array &headers) = 0;
	virtual void method(const char* method /* nullable */) = 0;
	virtual void body(const char* body /* nullable */) = 0;
	virtual void timeout(long t, long conn) = 0;

	virtual bool perform(void) = 0;

	virtual void clear(void) = 0;

	virtual const RespondedHandler &respondedCallback(void) const = 0;
	virtual const ErrorHandler &errorCallback(void) const = 0;
	virtual void callback(const RespondedHandler &cb /* nullable */) = 0;
	virtual void callback(const ErrorHandler &cb /* nullable */) = 0;

	static Fetch* create(void);
	static void destroy(Fetch* ptr);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web
*/

#if BITTY_WEB_ENABLED

/**
 * @brief Web object as HTTP server.
 */
class Web : public Updatable, public virtual Object {
public:
	typedef std::shared_ptr<Web> Ptr;

	struct RequestedHandler : public Handler<RequestedHandler, bool, RequestedHandler*, const char*, const char*, const char*, const char*, const char*> {
		using Handler::Handler;
	};

public:
	BITTY_CLASS_TYPE('W', 'E', 'B', 'C')

	virtual bool open(unsigned short port, const char* root) = 0;
	virtual bool close(void) = 0;

	virtual bool ready(void) const = 0;

	virtual bool polling(void) const = 0;

	virtual void poll(int timeoutMs) = 0;

	virtual bool respond(unsigned code) = 0;
	virtual bool respond(const char* data, const char* mimeType) = 0;
	virtual bool respond(const class Json* data, const char* mimeType /* nullable */) = 0;
	virtual bool respond(const class Bytes* data, const char* mimeType /* nullable */) = 0;

	virtual const RequestedHandler &requestedCallback(void) const = 0;
	virtual void callback(const RequestedHandler &cb /* nullable */) = 0;

	static Web* create(void);
	static void destroy(Web* ptr);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

#endif /* __WEB_H__ */
