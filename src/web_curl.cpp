/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "json.h"
#include "platform.h"
#include "web_curl.h"
#include "../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Fetch implementation with the cURL backend
*/

#if BITTY_WEB_ENABLED

FetchCurl::FetchCurl() {
	_state = IDLE;

	_curl = curl_easy_init();

	_response = Bytes::create();

#if defined BITTY_DEBUG
	fprintf(stdout, "Fetch (cURL) created.\n");
#endif /* BITTY_DEBUG */
}

FetchCurl::~FetchCurl() {
	LockGuard<decltype(_lock)> guard(_lock);

	reset();

	Bytes::destroy(_response);
	_response = nullptr;

	curl_easy_cleanup(_curl);
	_curl = nullptr;

#if defined BITTY_DEBUG
	fprintf(stdout, "Fetch (cURL) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned FetchCurl::type(void) const {
	return TYPE();
}

bool FetchCurl::open(void) {
	LockGuard<decltype(_lock)> guard(_lock);

	return true;
}

bool FetchCurl::close(void) {
	LockGuard<decltype(_lock)> guard(_lock);

	reset();

	_response->clear();
	_error.clear();

	_rspHandler = nullptr;
	_errHandler = nullptr;

	curl_easy_cleanup(_curl);
	_curl = curl_easy_init();

	return true;
}

Fetch::DataTypes FetchCurl::dataType(void) const {
	LockGuard<decltype(_lock)> guard(_lock);

	return _responseHint;
}

void FetchCurl::dataType(DataTypes y) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	_responseHint = y;
}

void FetchCurl::url(const char* url) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	curl_easy_setopt(_curl, CURLOPT_URL, url);

	if (Text::startsWith(url, "https://", true)) {
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}
}

void FetchCurl::options(const Variant &options) {
	Json::Ptr json(Json::create());
	if (!json->fromAny(options))
		return;
	rapidjson::Document doc;
	if (!json->toJson(doc))
		return;

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

void FetchCurl::headers(const Text::Array &headers) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	_headers = headers;
	if (_headersOpt) {
		curl_slist_free_all(_headersOpt);
		_headersOpt = nullptr;
	}
	for (const std::string &h : _headers)
		_headersOpt = curl_slist_append(_headersOpt, h.c_str());
	curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headersOpt);
}

void FetchCurl::method(const char* method) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	if (method)
		curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, method);
	else
		curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "");
}

void FetchCurl::body(const char* body) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	if (body)
		curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, body);
	else
		curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, "");
}

void FetchCurl::timeout(long t, long conn) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	_timeout = t;
	_connTimeout = conn;
}

bool FetchCurl::perform(void) {
	if (_state == BUSY)
		return false;

	LockGuard<decltype(_lock)> guard(_lock);

	curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, receive);

	curl_easy_setopt(_curl, CURLOPT_TIMEOUT, _timeout);
	curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, _connTimeout);

	curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);

	_state = BUSY;

	_response->clear();
	_error.clear();

#if BITTY_MULTITHREAD_ENABLED
	auto proc = [] (FetchCurl* self, CURL* curl) -> bool {
		LockGuard<decltype(self->_lock)> guard(self->_lock);

		Platform::threadName("WEB");

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			const char* err = curl_easy_strerror(res);
			fprintf(stderr, "CURL failed: %s\n", err);

			self->_error = err;

			self->_state = RESPONDED;

			return false;
		}

		self->_state = RESPONDED;

		return true;
	};
	_thread = std::thread(proc, this, _curl);
#endif /* BITTY_MULTITHREAD_ENABLED */

	return true;
}

void FetchCurl::clear(void) {
	if (_state == BUSY)
		return;

	LockGuard<decltype(_lock)> guard(_lock);

	reset();

	_response->clear();
	_error.clear();

	_rspHandler = nullptr;
	_errHandler = nullptr;

	curl_easy_reset(_curl);
}

bool FetchCurl::update(double) {
	if (_state == IDLE)
		return true;
	if (_state == BUSY)
		return true;

	LockGuard<decltype(_lock)> guard(_lock);

	assert(_state == RESPONDED);
#if BITTY_MULTITHREAD_ENABLED
	if (_thread.joinable())
		_thread.join();
#endif /* BITTY_MULTITHREAD_ENABLED */
	if (_error.empty()) {
		if (!_rspHandler.empty())
			_rspHandler(&_rspHandler, _response->pointer(), _response->count());

		_state = IDLE;
	} else {
		if (!_errHandler.empty())
			_errHandler(&_errHandler, _error.c_str());

		_state = IDLE;
	}

	return true;
}

const Fetch::RespondedHandler &FetchCurl::respondedCallback(void) const {
	return _rspHandler;
}

const Fetch::ErrorHandler &FetchCurl::errorCallback(void) const {
	return _errHandler;
}

void FetchCurl::callback(const RespondedHandler &cb) {
	_rspHandler = cb;
}

void FetchCurl::callback(const ErrorHandler &cb) {
	_errHandler = cb;
}

void FetchCurl::reset(void) {
	LockGuard<decltype(_lock)> guard(_lock);

#if BITTY_MULTITHREAD_ENABLED
	if (_thread.joinable())
		_thread.join();
#endif /* BITTY_MULTITHREAD_ENABLED */

	_state = IDLE;

	_headers.clear();
	if (_headersOpt) {
		curl_slist_free_all(_headersOpt);
		_headersOpt = nullptr;
	}
	_timeout = WEB_FETCH_TIMEOUT_SECONDS;
	_connTimeout = WEB_FETCH_CONNECTION_TIMEOUT_SECONDS;
	_responseHint = STRING;
}

size_t FetchCurl::receive(void* ptr, size_t size, size_t nmemb, void* stream) {
	FetchCurl* self = (FetchCurl*)stream;

	LockGuard<decltype(self->_lock)> guard(self->_lock);

	size_t len = size * nmemb;
	if (ptr && len)
		self->_response->writeBytes((Byte*)ptr, len);

	return size * nmemb;
}

#endif /* BITTY_WEB_ENABLED */

/* ===========================================================================} */
