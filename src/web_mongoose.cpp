/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#define NOMINMAX
#include "../lib/mongoose/mongoose.h" // Need to be above all to avoid compile error.
#include "bytes.h"
#include "json.h"
#include "web_mongoose.h"

/*
** {===========================================================================
** Macros and constants
*/

#if BITTY_WEB_ENABLED

#ifndef WEB_STATE
#	define WEB_STATE(P, I, W, O) \
	VariableGuard<decltype(P)> __PROC__(&(P), (I), (W)); \
	if (!(__PROC__).changed()) { \
		O; \
	}
#endif /* WEB_STATE */

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#if BITTY_WEB_ENABLED

static void webGetGmtTimeString(char* buf, size_t bufLen, time_t* t) {
	strftime(buf, bufLen, "%a, %d %b %Y %H:%M:%S GMT", gmtime(t));
}

static void webEventHandler(struct mg_connection* nc, int ev, void* evData) {
	WebMongoose* web = (WebMongoose*)nc->user_data;

	web->callback(nc, ev, evData);
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web implementation with the Mongoose backend
*/

#if BITTY_WEB_ENABLED

WebMongoose::WebMongoose() {
	_mgr = new struct mg_mgr;
	memset(_mgr, 0, sizeof(struct mg_mgr));

	_options = new struct mg_serve_http_opts;
	memset(_options, 0, sizeof(struct mg_serve_http_opts));

#if defined BITTY_DEBUG
	fprintf(stdout, "Web (Mongoose) created.\n");
#endif /* BITTY_DEBUG */
}

WebMongoose::~WebMongoose() {
	if (_opened)
		close();

	if (_mgr) {
		delete _mgr;
		_mgr = nullptr;
	}

	if (_options) {
		delete _options;
		_options = nullptr;
	}

#if defined BITTY_DEBUG
	fprintf(stdout, "Web (Mongoose) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned WebMongoose::type(void) const {
	return TYPE();
}

bool WebMongoose::open(unsigned short port, const char* root) {
	// Prepare.
	if (_opened)
		return false;
	_opened = true;

	// Initialize.
	mg_mgr_init(_mgr, this);

	// Parse data.
	if (root)
		_root = root;
	else
		_root = ".";

	// Open.
	doOpen(port);

	// Finish.
	return true;
}

bool WebMongoose::close(void) {
	// Closing during callback?
	if (polling()) {
		++_shutting;

		return true;
	}

	// Prepare.
	if (!_opened)
		return false;
	_opened = false;

	_shutting = 0;

	// Clear callback variables.
	_rspdHandler = RequestedHandler();

	_pollingConn = nullptr;

	// Dispose.
	if (_mgr)
		mg_mgr_free(_mgr);

	// Clear options.
	_root.clear();

	// Call polymorphic.
	doClose();

	// Finish.
	return true;
}

bool WebMongoose::ready(void) const {
	return _opened && !_shutting;
}

bool WebMongoose::polling(void) const {
	return _polling;
}

void WebMongoose::poll(int timeoutMs) {
	if (!_opened && !_shutting)
		return;

	doPoll(timeoutMs);
}

bool WebMongoose::update(double) {
	if (!ready())
		return true;

	doPoll(_timeoutMs);

	if (_shutting)
		close();

	return _opened;
}

bool WebMongoose::respond(unsigned code) {
	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using it.
	switch (code) {
	case 400:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 400 Bad Request\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 401:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 401 Unauthorized\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 403:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 403 Forbidden\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 405:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 405 Method Not Allowed\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 406:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 406 Not Acceptable\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 414:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 414 URI Too Long\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 415:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 415 Unsupported Media Type\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 500:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 500 Internal Server Error\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 501:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 501 Not Implemented\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 503:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 503 Service Unavailable\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 505:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 505 HTTP Version Not Supported\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	case 404: // Fall through.
	default:
		mg_printf(
			conn, "%s",
			"HTTP/1.0 404 Not Found\r\n"
			"Content-Length: 0\r\n\r\n"
		);

		break;
	}

	return true;
}

bool WebMongoose::respond(const char* data, const char* mimeType_) {
	if (!data || !*data)
		return false;

	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "text/plain";
	char currentTime[50];
	time_t t = (time_t)mg_time();
	webGetGmtTimeString(currentTime, sizeof(currentTime), &t);
	const size_t len = strlen(data);

	mg_printf(
		conn,
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Date: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"Content-Type: %.*s\r\n"
		"Content-Length: %" SIZE_T_FMT
		"\r\n",
		currentTime,
		(int)mimeType.length(), mimeType.c_str(),
		len
	);
	mg_send(conn, "\r\n", 2);
	mg_send(conn, data, (int)len);
	mg_send(conn, "\r\n", 2);
	//mg_printf(conn, "%s", data);

	return true;
}

bool WebMongoose::respond(const class Json* data, const char* mimeType_) {
	if (!data)
		return false;

	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "application/json";
	char currentTime[50];
	time_t t = (time_t)mg_time();
	webGetGmtTimeString(currentTime, sizeof(currentTime), &t);
	std::string content;
	data->toString(content, false);

	mg_printf(
		conn,
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Date: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"Content-Type: %.*s\r\n"
		"Content-Length: %" SIZE_T_FMT
		"\r\n",
		currentTime,
		(int)mimeType.length(), mimeType.c_str(),
		content.length()
	);
	mg_send(conn, "\r\n", 2);
	mg_send(conn, content.c_str(), (int)content.length());
	mg_send(conn, "\r\n", 2);

	return true;
}

bool WebMongoose::respond(const class Bytes* data, const char* mimeType_) {
	if (!data)
		return false;

	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "application/octet-stream";
	char currentTime[50];
	time_t t = (time_t)mg_time();
	webGetGmtTimeString(currentTime, sizeof(currentTime), &t);

	mg_printf(
		conn,
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Date: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"Content-Type: %.*s\r\n"
		"Content-Length: %" SIZE_T_FMT
		"\r\n",
		currentTime,
		(int)mimeType.length(), mimeType.c_str(),
		data->count()
	);
	mg_send(conn, "\r\n", 2);
	mg_send(conn, data->pointer(), (int)data->count());
	mg_send(conn, "\r\n", 2);

	return true;
}

const Web::RequestedHandler &WebMongoose::requestedCallback(void) const {
	return _rspdHandler;
}

void WebMongoose::callback(const RequestedHandler &cb) {
	_rspdHandler = cb;
}

void WebMongoose::callback(struct mg_connection* nc, int ev, void* evData) {
	if (onHttp(nc, ev, evData))
		return;
}

void WebMongoose::doOpen(unsigned short port) {
	std::string portstr = Text::toString(port);
	if (portstr.empty())
		portstr = "8080";

	_conn = mg_bind(_mgr, portstr.c_str(), webEventHandler);
	_conn->user_data = this;
	if (_conn == nullptr) {
		fprintf(stdout, "Web (0x%p) setup error.\n", (Web*)this);

		return;
	}

	mg_set_protocol_http_websocket(_conn);

	_options->document_root = _root.c_str();
	_options->enable_directory_listing = "yes";
}

void WebMongoose::doClose(void) {
	memset(_options, 0, sizeof(struct mg_serve_http_opts));
}

void WebMongoose::doPoll(int timeoutMs) {
	WEB_STATE(_polling, false, true, return)

	for (int i = 0; i < 4000; ++i) {
		if (!mg_mgr_poll(_mgr, timeoutMs))
			break;
	}
}

bool WebMongoose::onHttp(struct mg_connection* nc, int ev, void* evData) {
	switch (ev) {
	case MG_EV_HTTP_REQUEST: {
			struct http_message* hm = (struct http_message*)evData;

			if (!ready())
				break;

			WEB_STATE(_pollingConn, nullptr, nc, break)

			if (requestedCallback().empty()) {
				mg_serve_http(nc, hm, *_options);
			} else {
				const std::string method(hm->method.p, hm->method.len);
				const std::string uri(hm->uri.p, hm->uri.len);
				const std::string query(hm->query_string.p, hm->query_string.len);
				const std::string body(hm->body.p, hm->body.len);
				const std::string message(hm->message.p, hm->message.len);
				RequestedHandler &handler = const_cast<RequestedHandler &>(requestedCallback());
				const bool ret = handler(&handler, method.c_str(), uri.c_str(), query.c_str(), body.c_str(), message.c_str());
				if (!ret)
					mg_serve_http(nc, hm, *_options);
			}

			nc->flags |= MG_F_SEND_AND_CLOSE;
		}

		break;
	case MG_EV_SEND: {
			//if (!nc->send_mbuf.buf || !nc->send_mbuf.len)
			//	nc->flags |= MG_F_SEND_AND_CLOSE;
		}

		break;
	default:
		return false;
	}

	return true;
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */
