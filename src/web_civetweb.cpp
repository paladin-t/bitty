/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "json.h"
#include "text.h"
#include "web_civetweb.h"
#include "../lib/civetweb/include/civetweb.h"

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

static int webEventHandler(struct mg_connection* nc, void* cbdata) {
	struct mg_context* ctx = mg_get_context(nc);
	WebCivetWeb* web = (WebCivetWeb*)mg_get_user_data(ctx);

	web->callback(nc, cbdata);

	return 1;
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web implementation with the CivetWeb backend
*/

#if BITTY_WEB_ENABLED

WebCivetWeb::WebCivetWeb() {
	_opened = false;
	_shutting = 0;

#if defined BITTY_DEBUG
	fprintf(stdout, "Web (CivetWeb) created.\n");
#endif /* BITTY_DEBUG */
}

WebCivetWeb::~WebCivetWeb() {
	if (_opened)
		close();

#if defined BITTY_DEBUG
	fprintf(stdout, "Web (CivetWeb) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned WebCivetWeb::type(void) const {
	return TYPE();
}

bool WebCivetWeb::open(unsigned short port, const char* root) {
	// Prepare.
	if (_opened)
		return false;
	_opened = true;

	// Initialize.
	mg_init_library(0);
	_callbacks = (struct mg_callbacks*)malloc(sizeof(struct mg_callbacks));
	memset(_callbacks, 0, sizeof(struct mg_callbacks));

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

bool WebCivetWeb::close(void) {
	// Closing during callback?
	if (polling()) {
		_shutting = _shutting + 1;

		return true;
	}

	// Prepare.
	if (!_opened)
		return false;
	_opened = false;

	_shutting = 0;

	// Clear callback variables.
	do {
		LockGuard<decltype(_rspdHandlerLock)> guard(_rspdHandlerLock);

		_rspdHandler.clear();
	} while (false);

	_pollingConn = nullptr;

	// Clear options.
	_root.clear();

	// Call polymorphic.
	doClose();

	// Dispose.
	free(_callbacks);
	_callbacks = nullptr;
	mg_exit_library();

	// Finish.
	return true;
}

bool WebCivetWeb::ready(void) const {
	return _opened && !_shutting;
}

bool WebCivetWeb::polling(void) const {
	return _polling;
}

void WebCivetWeb::poll(int timeoutMs) {
	(void)timeoutMs;

	if (!_opened && !_shutting)
		return;

	doPoll();
}

bool WebCivetWeb::update(double) {
	if (!ready())
		return true;

	doPoll();

	if (_shutting)
		close();

	return _opened;
}

bool WebCivetWeb::respond(unsigned code) {
	struct mg_connection* conn = _pollingConn; // Using it.
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

bool WebCivetWeb::respond(const char* data, const char* mimeType_) {
	if (!data || !*data)
		return false;

	struct mg_connection* conn = _pollingConn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "text/plain";
	char currentTime[50];
	time_t t;
	time(&t);
	webGetGmtTimeString(currentTime, sizeof(currentTime), &t);
	const size_t len = strlen(data);

	mg_printf(
		conn,
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Date: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n",
		currentTime,
		mimeType.c_str(),
		len
	);
	mg_write(conn, "\r\n", 2);
	mg_write(conn, data, (int)len);
	mg_write(conn, "\r\n", 2);

	return true;
}

bool WebCivetWeb::respond(const class Json* data, const char* mimeType_) {
	if (!data)
		return false;

	struct mg_connection* conn = _pollingConn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "application/json";
	char currentTime[50];
	time_t t;
	time(&t);
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
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n",
		currentTime,
		mimeType.c_str(),
		content.length()
	);
	mg_write(conn, "\r\n", 2);
	mg_write(conn, content.c_str(), (int)content.length());
	mg_write(conn, "\r\n", 2);

	return true;
}

bool WebCivetWeb::respond(const class Bytes* data, const char* mimeType_) {
	if (!data)
		return false;

	struct mg_connection* conn = _pollingConn; // Using it.

	const std::string mimeType = mimeType_ ? mimeType_ : "application/octet-stream";
	char currentTime[50];
	time_t t;
	time(&t);
	webGetGmtTimeString(currentTime, sizeof(currentTime), &t);

	mg_printf(
		conn,
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Date: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %zu\r\n",
		currentTime,
		mimeType.c_str(),
		data->count()
	);
	mg_write(conn, "\r\n", 2);
	mg_write(conn, (const char*)data->pointer(), (int)data->count());
	mg_write(conn, "\r\n", 2);

	return true;
}

const Web::RequestedHandler &WebCivetWeb::requestedCallback(void) const {
	return _rspdHandler;
}

void WebCivetWeb::callback(const RequestedHandler &cb) {
	LockGuard<decltype(_rspdHandlerLock)> guard(_rspdHandlerLock);

	_rspdHandler = cb;
}

void WebCivetWeb::callback(struct mg_connection* nc, void* cbdata) {
	if (onHttp(nc, cbdata))
		return;
}

void WebCivetWeb::doOpen(unsigned short port) {
	std::string portstr = Text::toString(port);
	if (portstr.empty())
		portstr = "8080";
	std::string timeoutstr = Text::toString(_timeoutMs);
	if (timeoutstr.empty())
		timeoutstr = "10000";

	const char* options[] = {
		"document_root",            _root.c_str(),
		"enable_directory_listing", "yes",
		"listening_ports",          portstr.c_str(),
		"num_threads",              "1",
		"request_timeout_ms",       timeoutstr.c_str(),
		0
	};

	_ctx = mg_start(_callbacks, this, options);
	if (_ctx == nullptr) {
		fprintf(stderr, "Web (0x%p) setup error.\n", (Web*)this);

		return;
	}

	mg_set_request_handler(_ctx, "/*", webEventHandler, 0);
}

void WebCivetWeb::doClose(void) {
	mg_stop(_ctx);
}

void WebCivetWeb::doPoll(void) {
	WEB_STATE(_polling, false, true, return)

	// Do nothing.
}

bool WebCivetWeb::onHttp(struct mg_connection* nc, void* cbdata) {
	(void)cbdata;

	const struct mg_request_info* ri = mg_get_request_info(nc);
	const char* url = ri->local_uri;

	do {
		if (!ready())
			break;

		WEB_STATE(_pollingConn, nullptr, nc, break)

		LockGuard<decltype(_rspdHandlerLock)> guard(_rspdHandlerLock);

		if (requestedCallback().empty()) {
			if (strcmp(ri->request_method, "GET") == 0) {
				mg_send_file(nc, url);
			}
		} else {
			const std::string method(ri->request_method ? ri->request_method : "");
			const std::string uri(url);
			const std::string query(ri->query_string ? ri->query_string : "");

			char bodybuff[1024];
			const int bodylen = mg_read(nc, bodybuff, sizeof(bodybuff) - 1);
			const std::string body(bodybuff, (size_t)bodylen);

			Text::Dictionary headers;
			for (int i = 0; i < ri->num_headers; ++i) {
				const struct mg_header &h = ri->http_headers[i];
				if (h.name) {
					headers[h.name] = h.value ? h.value : "";
				}
			}

			RequestedHandler &handler = const_cast<RequestedHandler &>(requestedCallback());
			const bool ret = handler(&handler, method.c_str(), uri.c_str(), query.c_str(), body.c_str(), headers);
			if (!ret) {
				mg_send_file(nc, url);
			}
		}
	} while (false);

	return true;
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */
