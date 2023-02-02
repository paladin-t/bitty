/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WEB_MONGOOSE_H__
#define __WEB_MONGOOSE_H__

#include "web.h"

/*
** {===========================================================================
** Web implementation with the Mongoose backend
*/

#if BITTY_WEB_ENABLED

class WebMongoose : public Web {
private:
	/**< States. */

	bool _opened = false;
	int _shutting = 0;
	std::string _root;

	bool _polling = false;

	/**< Options. */

	int _timeoutMs = 1;

	/**< Connection. */

	struct mg_mgr* _mgr = nullptr;
	struct mg_connection* _conn = nullptr;
	struct mg_serve_http_opts* _options = nullptr;

	/**< Callbacks. */

	RequestedHandler _rspdHandler;

	struct mg_connection* _pollingConn = nullptr;

public:
	WebMongoose();
	virtual ~WebMongoose() override;

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

private:
	void doOpen(unsigned short port);
	void doClose(void);

	void doPoll(int timeoutMs);

	bool onHttp(struct mg_connection* nc, int ev, void* evData);
};

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

#endif /* __WEB_MONGOOSE_H__ */
