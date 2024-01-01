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
#include "platform.h"
#include "web_html.h"
#include "../lib/jpath/jpath.hpp"
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#endif /* BITTY_OS_HTML */

/*
** {===========================================================================
** Utilities
*/

static unsigned webGetId(void) {
	static unsigned seed = 1;
	while (seed == 0)
		++seed;

	return seed;
}

#if defined BITTY_OS_HTML
EM_JS(
	void, webFetchPerform, (unsigned id, const char* url, const char* options), {
		if (!window.__BITTY_WEB_FETCH__)
			window.__BITTY_WEB_FETCH__ = { };

		var res = UTF8ToString(url);
		var opt = JSON.parse(UTF8ToString(options));
		var obj = {
			promise: fetch(res, opt)
				.then(function (rsp) {
					if (rsp.type == 'opaque') {
						obj.state = 'responded';
						obj.error = 'Opaque';

						return;
					}
					rsp.text()
						.then(function (data) {
							obj.state = 'responded';
							obj.response = data;
						});
				})
				.catch(function (err) {
					var data = err.message;
					obj.state = 'responded';
					obj.error = data;
				}),
			state: 'busy',
			response: null,
			error: null,
			cache: [ ]
		};
		window.__BITTY_WEB_FETCH__[id] = obj;
	}
);
EM_JS(
	bool, webFetchUpdate, (unsigned id), {
		var obj = window.__BITTY_WEB_FETCH__[id];
		if (!obj) {
			return false;
		}
		if (obj.state != 'responded') {
			return false;
		}

		return true;
	}
);
EM_JS(
	unsigned, webFetchGetRespondedCount, (unsigned id), {
		var ret = null;
		var obj = window.__BITTY_WEB_FETCH__[id];
		if (!obj) {
			ret = null;
		} else if (obj.state != 'responded') {
			ret = null;
		} else {
			ret = obj.response;
		}

		if (!ret || typeof ret != 'string')
			return 0;
		var lengthBytes = lengthBytesUTF8(ret) + 1;

		return lengthBytes;
	}
);
EM_JS(
	const char*, webFetchGetResponded, (unsigned id), {
		var ret = null;
		var obj = window.__BITTY_WEB_FETCH__[id];
		if (!obj) {
			ret = null;
		} else if (obj.state != 'responded') {
			ret = null;
		} else {
			ret = obj.response;
		}

		if (!ret || typeof ret != 'string')
			ret = '';
		var lengthBytes = lengthBytesUTF8(ret) + 1;
		var stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes + 1);
		obj.cache.push(stringOnWasmHeap);

		return stringOnWasmHeap;
	}
);
EM_JS(
	const char*, webFetchGetError, (unsigned id), {
		var ret = null;
		var obj = window.__BITTY_WEB_FETCH__[id];
		if (!obj) {
			ret = null;
		} else if (obj.state != 'responded') {
			ret = null;
		} else {
			ret = obj.error;
		}

		if (!ret || typeof ret != 'string')
			ret = '';
		var lengthBytes = lengthBytesUTF8(ret) + 1;
		var stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(ret, stringOnWasmHeap, lengthBytes + 1);
		obj.cache.push(stringOnWasmHeap);

		return stringOnWasmHeap;
	}
);
EM_JS(
	void, webFetchRemove, (unsigned id), {
		var obj = window.__BITTY_WEB_FETCH__[id];
		if (!obj)
			return;

		obj.cache.forEach(function (buf) {
			_free(buf);
		});
		obj.cache = null;

		delete window.__BITTY_WEB_FETCH__[id];
	}
);
#else /* BITTY_OS_HTML */
static void webFetchPerform(unsigned /* id */, const char* /* url */, const char* /* options */) {
	assert(false && "Not implemented.");
}
static bool webFetchUpdate(unsigned /* id */) {
	assert(false && "Not implemented.");

	return false;
}
static unsigned webFetchGetRespondedCount(unsigned /* id */) {
	assert(false && "Not implemented.");

	return 0;
}
static const char* webFetchGetResponded(unsigned /* id */) {
	assert(false && "Not implemented.");

	return "";
}
static const char* webFetchGetError(unsigned /* id */) {
	assert(false && "Not implemented.");

	return "";
}
static bool webFetchRemove(unsigned /* id */) {
	assert(false && "Not implemented.");

	return false;
}
#endif /* BITTY_OS_HTML */

/* ===========================================================================} */

/*
** {===========================================================================
** Fetch implementation with the HTML backend
*/

#if BITTY_WEB_ENABLED

FetchHtml::FetchHtml() {
	_id = webGetId();

#if defined BITTY_DEBUG
	fprintf(stdout, "Fetch (HTML) created.\n");
#endif /* BITTY_DEBUG */
}

FetchHtml::~FetchHtml() {
	reset();

#if defined BITTY_DEBUG
	fprintf(stdout, "Fetch (HTML) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned FetchHtml::type(void) const {
	return TYPE();
}

bool FetchHtml::open(void) {
	return true;
}

bool FetchHtml::close(void) {
	reset();

	_rspHandler = nullptr;
	_errHandler = nullptr;

	return true;
}

Fetch::DataTypes FetchHtml::dataType(void) const {
	return _responseHint;
}

void FetchHtml::dataType(DataTypes y) {
	_responseHint = y;
}

void FetchHtml::url(const char* url) {
	_url = url;
}

void FetchHtml::options(const Variant &options) {
	Json::Ptr json(Json::create());
	if (!json->fromAny(options))
		return;
	rapidjson::Document doc;
	if (!json->toJson(doc))
		return;

	json->toString(_options);

	std::string method_;
	if (Jpath::get(doc, method_, "method")) {
		method(method_.c_str());
	}

	const rapidjson::Value* headers_ = nullptr;
	if (Jpath::get(doc, headers_, "headers") && headers_ && headers_->IsObject()) {
		Text::Array heads;
		for (rapidjson::Value::ConstMemberIterator it = headers_->MemberBegin(); it != headers_->MemberEnd(); ++it) {
			const rapidjson::Value &jk = it->name;
			const rapidjson::Value &jv = it->value;

			std::string head = jk.GetString();
			head += ": ";
			head += jv.GetString();
			heads.push_back(head);
		}
		headers(heads);
	}

	std::string body_;
	if (Jpath::get(doc, body_, "body")) {
		body(body_.c_str());
	}

	std::string hint_;
	if (Jpath::get(doc, hint_, "hint")) {
		DataTypes y = STRING;
		if (!hint_.empty()) {
			Text::toLowerCase(hint_);
			if (hint_ == "bytes")
				y = BYTES;
			else if (hint_ == "string")
				y = STRING;
			else if (hint_ == "json")
				y = JSON;
		}
		dataType(y);
	}
}

void FetchHtml::headers(const Text::Array &headers) {
	_headers = headers;
}

void FetchHtml::method(const char* method) {
	_method = method;
}

void FetchHtml::body(const char* body) {
	_body = body;
}

void FetchHtml::timeout(long, long) {
	// Do nothing.
}

bool FetchHtml::perform(void) {
	webFetchPerform(_id, _url.c_str(), _options.c_str());

	return true;
}

void FetchHtml::clear(void) {
	reset();

	_response.clear();

	_error.clear();
}

bool FetchHtml::update(double) {
	if (!webFetchUpdate(_id))
		return true;

	const unsigned len = webFetchGetRespondedCount(_id);
	const char* rsp = webFetchGetResponded(_id);
	const char* err = webFetchGetError(_id);

	if (len && rsp)
		_response.assign(rsp, (size_t)len);
	if (err)
		_error.assign(err);

	if (_error.empty()) {
		if (!_rspHandler.empty())
			_rspHandler(&_rspHandler, (Byte*)_response.c_str(), _response.length());
	} else {
		if (!_errHandler.empty())
			_errHandler(&_errHandler, _error.c_str());
	}

	if ((len && rsp) || err)
		webFetchRemove(_id);

	return true;
}

const Fetch::RespondedHandler &FetchHtml::respondedCallback(void) const {
	return _rspHandler;
}

const Fetch::ErrorHandler &FetchHtml::errorCallback(void) const {
	return _errHandler;
}

void FetchHtml::callback(const RespondedHandler &cb) {
	_rspHandler = cb;
}

void FetchHtml::callback(const ErrorHandler &cb) {
	_errHandler = cb;
}

void FetchHtml::reset(void) {
	_options.clear();
	_headers.clear();
	_method.clear();
	_body.clear();
	_responseHint = STRING;
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Web polyfill with the HTML backend
*/

#if BITTY_WEB_ENABLED

WebHtml::WebHtml() {
#if defined BITTY_DEBUG
	fprintf(stdout, "Web (HTML) created.\n");
	fprintf(stderr, "Web (HTML) doesn't implement anything.\n");
#endif /* BITTY_DEBUG */
}

WebHtml::~WebHtml() {
#if defined BITTY_DEBUG
	fprintf(stdout, "Web (HTML) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned WebHtml::type(void) const {
	return TYPE();
}

bool WebHtml::open(unsigned short, const char*) {
	return false;
}

bool WebHtml::close(void) {
	return false;
}

bool WebHtml::ready(void) const {
	return false;
}

bool WebHtml::polling(void) const {
	return false;
}

void WebHtml::poll(int) {
}

bool WebHtml::update(double) {
	return false;
}

bool WebHtml::respond(unsigned) {
	return false;
}

bool WebHtml::respond(const char*, const char*) {
	return false;
}

bool WebHtml::respond(const class Json*, const char*) {
	return false;
}

bool WebHtml::respond(const class Bytes*, const char*) {
	return false;
}

const Web::RequestedHandler &WebHtml::requestedCallback(void) const {
	static Web::RequestedHandler placeholder;

	return placeholder;
}

void WebHtml::callback(const RequestedHandler &) {
}

void WebHtml::callback(struct mg_connection*, int, void*) {
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */
