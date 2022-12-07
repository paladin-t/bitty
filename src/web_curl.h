/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WEB_CURL_H__
#define __WEB_CURL_H__

#include "web.h"
#if BITTY_WEB_ENABLED
#	if !defined BITTY_OS_HTML
#		include <curl/curl.h>
#	endif /* BITTY_OS_HTML */
#endif /* BITTY_WEB_ENABLED */
#if BITTY_MULTITHREAD_ENABLED
#	include <thread>
#endif /* BITTY_MULTITHREAD_ENABLED */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WEB_FETCH_TIMEOUT_SECONDS
#	define WEB_FETCH_TIMEOUT_SECONDS 20l
#endif /* WEB_FETCH_TIMEOUT_SECONDS */
#ifndef WEB_FETCH_CONNECTION_TIMEOUT_SECONDS
#	define WEB_FETCH_CONNECTION_TIMEOUT_SECONDS 10l
#endif /* WEB_FETCH_CONNECTION_TIMEOUT_SECONDS */

/* ===========================================================================} */

/*
** {===========================================================================
** Fetch implementation with the cURL backend
*/

#if BITTY_WEB_ENABLED

class FetchCurl : public Fetch {
private:
	enum States {
		IDLE,
		BUSY,
		RESPONDED
	};

private:
	/**< States. */

	Atomic<States> _state;

	/**< Options. */

	Text::Array _headers;
	struct curl_slist* _headersOpt = nullptr;
	long _timeout = WEB_FETCH_TIMEOUT_SECONDS;
	long _connTimeout = WEB_FETCH_CONNECTION_TIMEOUT_SECONDS;
	DataTypes _responseHint = STRING;

	/**< Connection. */

	CURL* _curl = nullptr;

	/**< Callbacks. */

	class Bytes* _response = nullptr;
	std::string _error;

	RespondedHandler _rspHandler;
	ErrorHandler _errHandler;

	/**< Threading. */

#if BITTY_MULTITHREAD_ENABLED
	std::thread _thread;
#endif /* BITTY_MULTITHREAD_ENABLED */
	mutable RecursiveMutex _lock;

public:
	FetchCurl();
	virtual ~FetchCurl() override;

	virtual unsigned type(void) const override;

	virtual bool open(void) override;
	virtual bool close(void) override;

	virtual DataTypes dataType(void) const override;
	virtual void dataType(DataTypes y) override;

	virtual void url(const char* url) override;
	virtual void options(const Variant &options) override;
	virtual void headers(const Text::Array &headers) override;
	virtual void method(const char* method) override;
	virtual void body(const char* body) override;
	virtual void timeout(long t, long conn) override;

	virtual bool perform(void) override;

	virtual void clear(void) override;

	virtual bool update(double delta) override;

	virtual const RespondedHandler &respondedCallback(void) const override;
	virtual const ErrorHandler &errorCallback(void) const override;
	virtual void callback(const RespondedHandler &cb) override;
	virtual void callback(const ErrorHandler &cb) override;

private:
	void reset(void);

	static size_t receive(void* ptr, size_t size, size_t nmemb, void* stream);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

#endif /* __WEB_CURL_H__ */
