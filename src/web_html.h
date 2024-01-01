/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WEB_HTML_H__
#define __WEB_HTML_H__

#include "web.h"

/*
** {===========================================================================
** Fetch implementation with the HTML backend
*/

#if BITTY_WEB_ENABLED

class FetchHtml : public Fetch {
private:
	/**< States. */

	unsigned _id = 0;

	/**< Options. */

	std::string _url;
	std::string _options;
	Text::Array _headers;
	std::string _method;
	std::string _body;
	DataTypes _responseHint = STRING;

	/**< Callbacks. */

	std::string _response;
	std::string _error;

	RespondedHandler _rspHandler;
	ErrorHandler _errHandler;

public:
	FetchHtml();
	virtual ~FetchHtml() override;

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
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web polyfill with the HTML backend
*/

#if BITTY_WEB_ENABLED

class WebHtml : public Web {
public:
	WebHtml();
	virtual ~WebHtml() override;

	virtual unsigned type(void) const override;

	virtual bool open(unsigned short port, const char* root) override;
	virtual bool close(void) override;

	virtual bool ready(void) const override;

	virtual bool polling(void) const override;

	virtual void poll(int timeoutMs) override;

	virtual bool update(double delta) override;

	virtual bool respond(unsigned code) override;
	virtual bool respond(const char* data, const char* mimeType) override;
	virtual bool respond(const class Json* data, const char* mimeType) override;
	virtual bool respond(const class Bytes* data, const char* mimeType) override;

	virtual const RequestedHandler &requestedCallback(void) const override;
	virtual void callback(const RequestedHandler &cb) override;
	void callback(struct mg_connection* nc, int ev, void* evData);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

#endif /* __WEB_HTML_H__ */
