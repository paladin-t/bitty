/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WEB_CIVETWEB_H__
#define __WEB_CIVETWEB_H__

#include "web.h"

/*
** {===========================================================================
** Web implementation with the CivetWeb backend
*/

#if BITTY_WEB_ENABLED

class WebCivetWeb : public Web {
private:
	/**< States. */

	Atomic<bool> _opened;
	Atomic<int> _shutting;
	std::string _root;

	bool _polling = false;

	/**< Options. */

	int _timeoutMs = 10000;

	/**< Connection. */

	struct mg_context* _ctx = nullptr;
	struct mg_callbacks* _callbacks = nullptr;

	/**< Callbacks. */

	RequestedHandler _rspdHandler = nullptr;
	mutable Mutex _rspdHandlerLock;

	struct mg_connection* _pollingConn = nullptr;

public:
	WebCivetWeb();
	virtual ~WebCivetWeb() override;

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
	void callback(struct mg_connection* nc, void* cbdata);

private:
	void doOpen(unsigned short port);
	void doClose(void);

	void doPoll(void);

	bool onHttp(struct mg_connection* nc, void* cbdata);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

#endif /* __WEB_CIVETWEB_H__ */
