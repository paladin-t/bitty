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
#include "datetime.h"
#include "encoding.h"
#include "json.h"
#include "network_mongoose.h"
#include "text.h"
#if defined BITTY_OS_HTML
	/* Do nothing. */
#elif defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
#	ifdef __cplusplus
		extern "C" {
#	endif /* __cplusplus */
#		include "../lib/network_info/cbits/network.h"
#	ifdef __cplusplus
		}
#	endif /* __cplusplus */
#elif defined BITTY_OS_ANDROID
	/* Do nothing. */
#else /* Platform macro. */
#	error "Not implemented."
#endif /* Platform macro. */

/*
** {===========================================================================
** Macros and constants
*/

#if BITTY_NETWORK_ENABLED

#ifndef NETWORK_NULL_STRING
#	define NETWORK_NULL_STRING "(EMPTY)"
#endif /* NETWORK_NULL_STRING */

#ifndef NETWORK_STATE
#	define NETWORK_STATE(P, I, W, O) \
	VariableGuard<decltype(P)> __PROC__(&(P), (I), (W)); \
	if (!(__PROC__).changed()) { \
		O; \
	}
#endif /* NETWORK_STATE */

static_assert(sizeof(Network::BytesSize) == 4, "Wrong size.");

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#if BITTY_NETWORK_ENABLED

static void networkEventHandler(struct mg_connection* nc, int ev, void* evData) {
	NetworkMongoose* net = (NetworkMongoose*)nc->user_data;

	net->callback(nc, ev, evData);
}

static void networkAddressToString(union socket_address* sa, Network::AddressName &addr) {
	assert(BITTY_COUNTOF(addr.text) >= strlen(NETWORK_NULL_STRING) + 1);
	memset(&addr, 0, sizeof(Network::AddressName));
	memcpy(addr.text, NETWORK_NULL_STRING, strlen(NETWORK_NULL_STRING));
	if (sa) {
		if (mg_sock_addr_to_str(sa, addr.text, BITTY_COUNTOF(addr.text), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT)) {
		}
	}
}

static std::string networkGetInterfaces(void) {
	rapidjson::Document jdoc;
	jdoc.SetObject();
	rapidjson::Value jarr;
	jarr.SetArray();

#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
	constexpr const size_t BYTE_COUNT = std::min(sizeof(ipv4), (size_t)4);
	static_assert(BYTE_COUNT >= 4, "Wrong size.");
	union {
		ipv4 ip;
		Byte bytes[BYTE_COUNT];
	} lo;
	lo.bytes[0] = 127;
	lo.bytes[1] = 0;
	lo.bytes[2] = 0;
	lo.bytes[3] = 1;

	struct network_interface iface[16];
	int n = c_get_network_interfaces(iface, BITTY_COUNTOF(iface));
	for (int i = 0; i < n; ++i) {
		union {
			ipv4 ip;
			Byte bytes[BYTE_COUNT];
		} u;
		u.ip = iface[i].ip_address;
		if (u.ip == 0 || u.ip == lo.ip)
			continue;

		std::string addr;
		for (int j = 0; j < (int)BYTE_COUNT; ++j) {
			addr += Text::toString(u.bytes[j]);
			if (j != (int)BYTE_COUNT - 1)
				addr.push_back('.');
		}

		std::string name;
		if (iface[i].name[0]) {
			name = Unicode::fromWide(iface[i].name);
		}

		rapidjson::Value jobj;
		jobj.SetObject();
		rapidjson::Value jkaddr;
		rapidjson::Value jvaddr;
		jkaddr.SetString("address", jdoc.GetAllocator());
		jvaddr.SetString(addr.c_str(), jdoc.GetAllocator());
		jobj.AddMember(jkaddr, jvaddr, jdoc.GetAllocator());
		rapidjson::Value jkname;
		rapidjson::Value jvname;
		jkname.SetString("name", jdoc.GetAllocator());
		jvname.SetString(name.c_str(), jdoc.GetAllocator());
		jobj.AddMember(jkname, jvname, jdoc.GetAllocator());
		jarr.PushBack(jobj, jdoc.GetAllocator());
	}

	rapidjson::Value jiface;
	jiface.SetString("interfaces", jdoc.GetAllocator());
	jdoc.AddMember(jiface, jarr, jdoc.GetAllocator());
#endif /* Platform macro. */

	std::string json;
	Json::toString(jdoc, json, false);

	return json;
}

static bool networkSend(
	struct mg_connection* nc, bool limitedSize,
	const Byte* buf, size_t len, bool bytesWithSize, bool withEos
) {
	size_t sz = len;
	if (bytesWithSize)
		sz += sizeof(Network::BytesSize);
	if (withEos)
		++sz;
	if (limitedSize && sz > NETWORK_MESSAGE_MAX_SIZE)
		return false;

	if (bytesWithSize) {
		Network::BytesSize head = (Network::BytesSize)(len + sizeof(Network::BytesSize));
		mg_send(nc, &head, 4);
	}
	mg_send(nc, buf, (int)len);
	if (withEos) {
		char eos[1] = { '\0' };
		mg_send(nc, eos, 1);
	}

	return true;
}

static bool networkBroadcast(
	struct mg_mgr &mgr, struct mg_connection* head, struct mg_connection* conn, bool limitedSize,
	const Byte* buf, size_t len, bool bytesWithSize, bool withEos
) {
	bool ret = true;
	for (struct mg_connection* c = mg_next(&mgr, nullptr); !!c; c = mg_next(&mgr, c)) {
		if (c == head || c == conn)
			continue;

		if (!networkSend(c, limitedSize, buf, len, bytesWithSize, withEos))
			ret = false;
	}

	return ret;
};

static Bytes* networkReceiveBytes(struct mg_connection* nc, bool bytesWithSize, Bytes* cached) {
	if (bytesWithSize) {
		if (nc->recv_mbuf.len >= sizeof(Network::BytesSize)) {
			Network::BytesSize* up = (Network::BytesSize*)nc->recv_mbuf.buf;
			if (nc->recv_mbuf.len >= *up) {
				if (cached) {
					cached->clear();
					cached->writeBytes(
						(const Byte*)(nc->recv_mbuf.buf + sizeof(Network::BytesSize)),
						(size_t)(*up - sizeof(Network::BytesSize))
					);
				} else {
					assert(false);
				}
				mbuf_remove(&nc->recv_mbuf, (size_t)(*up));

				return cached;
			}
		}
	} else {
		if (nc->recv_mbuf.len > 0) {
			if (cached) {
				cached->clear();
				cached->writeBytes((const Byte*)nc->recv_mbuf.buf, nc->recv_mbuf.len);
			} else {
				assert(false);
			}
			mbuf_remove(&nc->recv_mbuf, nc->recv_mbuf.len);

			return cached;
		}
	}

	return nullptr;
}

static bool networkReceiveUntilEos(struct mg_connection* nc, std::string &str) {
	struct mbuf &buf = nc->recv_mbuf;
	for (size_t i = 0; i < buf.len; ++i) {
		if (i >= NETWORK_MESSAGE_MAX_SIZE || buf.buf[i] == '\0') {
			str.clear();
			str.assign(buf.buf, i);
			mbuf_remove(&buf, i + 1);

			return true;
		}
	}

	return false;
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Network implementation with the Mongoose backend
*/

#if BITTY_NETWORK_ENABLED

NetworkMongoose::NetworkMongoose() {
	_mgr = new struct mg_mgr;
	memset(_mgr, 0, sizeof(struct mg_mgr));

#if defined BITTY_DEBUG
	fprintf(stdout, "Network (Mongoose) created.\n");
#endif /* BITTY_DEBUG */
}

NetworkMongoose::~NetworkMongoose() {
	if (_opened)
		close();

	if (_mgr) {
		delete _mgr;
		_mgr = nullptr;
	}

#if defined BITTY_DEBUG
	fprintf(stdout, "Network (Mongoose) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned NetworkMongoose::type(void) const {
	return TYPE();
}

bool NetworkMongoose::open(const char* addr, Protocols protocol, bool* toconn_, bool* tobind_) {
	// Prepare.
	if (toconn_)
		*toconn_ = false;
	if (tobind_)
		*tobind_ = false;

	if (_opened)
		return false;
	_opened = true;

	// Get valid network interfaces.
	std::string ifaces = networkGetInterfaces();
	option("interfaces", ifaces);

	// Initialize.
	mg_mgr_init(_mgr, this);

	// Parse data.
	std::string straddr = addr;
	std::string strop = "*";
	if (!straddr.empty()) {
		if (straddr.front() == '>') {
			strop = ">"; // Connect to an address; as a client.
			straddr = straddr.substr(1);
		} else if (straddr.front() == '<') {
			strop = "<"; // Bind from an address; as a server.
			straddr = straddr.substr(1);
		}
	}

	std::string strdirt;
	bool withudp = false, withtcp = false, withws = false;
	if (Text::startsWith(straddr.c_str(), "udp://", true)) {
		if ((protocol & UDP) == NONE)
			return false;

		withudp = true;
		strdirt = straddr.substr(6);
		_protocol = UDP;
	} else if (Text::startsWith(straddr.c_str(), "tcp://", true)) {
		if ((protocol & TCP) == NONE)
			return false;

		withtcp = true;
		strdirt = straddr.substr(6);
		_protocol = TCP;
	} else if (Text::startsWith(straddr.c_str(), "ws://", true)) {
		if ((protocol & WEBSOCKET) == NONE)
			return false;

		withws = true;
		strdirt = straddr.substr(5);
		_protocol = WEBSOCKET;
	} else {
		if ((protocol & UDP) != NONE)
			withudp = true;
		if ((protocol & TCP) != NONE)
			withtcp = true;
		if ((protocol & WEBSOCKET) != NONE)
			withws = true;

		strdirt = straddr;
		_protocol = protocol;
	}
	assert(withudp || withtcp || withws);

	bool toconn = false, tobind = false;
	if (strop == ">") {
		toconn = true;
		if (toconn_)
			*toconn_ = true;
	} else if (strop == "<") {
		tobind = true;
		if (tobind_)
			*tobind_ = true;
	} else /* if (strop == "*") */ {
		char* conv_suc = nullptr;
		Text::strtol(strdirt.c_str(), &conv_suc, 0);
		if (*conv_suc == '\0')
			tobind = true;
		else
			toconn = true;
	}
	assert(toconn || tobind);

	// Open.
	doOpen(withudp, withtcp, withws, toconn, tobind, straddr.c_str());

	// Finish.
	return true;
}

bool NetworkMongoose::close(void) {
	// Closing during callback?
	if (polling()) {
		++_shutting;

		return true;
	}

	// Prepare.
	if (!_opened)
		return false;
	_opened = false;

	_ready = IDLE;
	_shutting = 0;
	_binded = false; _wasBinded = false;
	_protocol = NONE;

	// Clear callback variables.
	_recvHandler = ReceivedHandler();
	_stblHandler = EstablishedHandler();
	_dscnHandler = DisconnectedHandler();

	if (_bytesCache) {
		Bytes::destroy(_bytesCache);
		_bytesCache = nullptr;
	}
	if (_jsonCache) {
		Json::destroy(_jsonCache);
		_jsonCache = nullptr;
	}

	_pollingConn = nullptr;

	// Dispose.
	if (_mgr)
		mg_mgr_free(_mgr);

	// Clear options.
	_options.clear();
	_dataType = JSON;
	_bytesWithSize = true;

	// Call polymorphic.
	doClose();

	// Finish.
	return true;
}

Network::DataTypes NetworkMongoose::dataType(void) const {
	return _dataType;
}

void NetworkMongoose::dataType(DataTypes y) {
	_dataType = y;
}

std::string NetworkMongoose::option(const std::string &key) const {
	std::string skey = key;
	Text::toLowerCase(skey);
	OptionDictionary::const_iterator it = _options.find(skey);
	if (it != _options.end())
		return it->second;

	return "";
}

void NetworkMongoose::option(const std::string &key, const std::string &val) {
	if (_opened)
		return;

	std::string skey = key;
	Text::toLowerCase(skey);
	auto it = _options.insert(std::make_pair(key, val));
	if (it.second) {
		if (skey == "data_type") { // String constants.
			std::string sval = val;
			Text::toLowerCase(sval);
			if (sval == "stream") {
				_dataType = STREAM;
				_bytesWithSize = false;
			} else if (sval == "bytes") {
				_dataType = BYTES;
				_bytesWithSize = true;
			} else if (sval == "string") {
				_dataType = STRING;
				_bytesWithSize = true;
			} else if (sval == "json") {
				_dataType = JSON;
				_bytesWithSize = true;
			}
		}
	}
}

bool NetworkMongoose::ready(void) const {
	return _opened && (_ready == READY) && !_shutting;
}

bool NetworkMongoose::polling(void) const {
	return _polling;
}

bool NetworkMongoose::connective(void) const {
	return (_protocol & (TCP | WEBSOCKET)) != NONE;
}

void NetworkMongoose::callback(struct mg_connection* nc, int ev, void* evData) {
	if (onSocket(nc, ev, evData))
		return;
	if (onWebsocket(nc, ev, evData))
		return;
}

void NetworkMongoose::poll(int timeoutMs) {
	if (!_opened && !_shutting)
		return;

	doPoll(timeoutMs);
}

void NetworkMongoose::establish(void) {
	const long long now = DateTime::ticks();
	while (_ready == IDLE) {
		doPoll(1);
		const double diff = DateTime::toSeconds(DateTime::ticks() - now);
		if (diff > (1.0 * (NETWORK_TIMEOUT_SECONDS)))
			break;
	}
}

void NetworkMongoose::disconnect(void) {
	if (!_opened)
		return;

	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using either.
	if (!conn)
		return;

	conn->flags |= MG_F_CLOSE_IMMEDIATELY;
	mg_if_poll(conn, 0);
}

bool NetworkMongoose::send(void* ptr, size_t sz, DataTypes y) {
	struct mg_connection* conn = _pollingConn ? _pollingConn : _conn; // Using either.
	PushHandler pusher = std::bind(
		networkSend, conn, false,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);

	return doPush(ptr, sz, y, pusher);
}

bool NetworkMongoose::broadcast(void* ptr, size_t sz, DataTypes y, bool filterPolling) {
	if (!_binded)
		return false;
	if (!connective())
		return false;

	struct mg_connection* conn = filterPolling ? _pollingConn : nullptr; // Except for polling.
	PushHandler pusher = std::bind(
		networkBroadcast, *_mgr, _conn, conn, false,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);

	return doPush(ptr, sz, y, pusher);
}

bool NetworkMongoose::update(double /* delta */) {
	if (!ready())
		return true;

	doPoll(_timeoutMs);

	if (_shutting)
		close();

	return _opened;
}

const Network::ReceivedHandler &NetworkMongoose::receivedCallback(void) const {
	return _recvHandler;
}

const Network::EstablishedHandler &NetworkMongoose::establishedCallback(void) const {
	return _stblHandler;
}

const Network::DisconnectedHandler &NetworkMongoose::disconnectedCallback(void) const {
	return _dscnHandler;
}

void NetworkMongoose::callback(const ReceivedHandler &cb) {
	_recvHandler = cb;
}

void NetworkMongoose::callback(const EstablishedHandler &cb) {
	_stblHandler = cb;
}

void NetworkMongoose::callback(const DisconnectedHandler &cb) {
	_dscnHandler = cb;
}

void NetworkMongoose::callback(const ReceivedHandler &recvCb, const EstablishedHandler &stbCb, const DisconnectedHandler &dscnCb) {
	_recvHandler = recvCb;
	_stblHandler = stbCb;
	_dscnHandler = dscnCb;
}

void NetworkMongoose::doOpen(bool withudp, bool withtcp, bool withws, bool toconn, bool tobind, const char* addr) {
	_wasBinded = _binded = tobind;
	if (withudp || withtcp) {
		if (toconn) {
			struct mg_connect_opts opt;
			memset(&opt, 0, sizeof(decltype(opt)));
			opt.user_data = this;
			_conn = mg_connect_opt(_mgr, addr, networkEventHandler, opt);
			fprintf(stdout, "Network (0x%p) opened for connecting: %s.\n", (Network*)this, addr);
			if (withudp)
				_ready = _conn ? READY : IDLE;
		} else if (tobind) {
			struct mg_bind_opts opt;
			memset(&opt, 0, sizeof(decltype(opt)));
			opt.user_data = this;
			_conn = mg_bind_opt(_mgr, addr, networkEventHandler, opt);
			_ready = _conn ? READY : IDLE;
			fprintf(stdout, "Network (0x%p) opened for binding: %s.\n", (Network*)this, addr);
		}
	} else if (withws) {
		if (toconn) {
			struct mg_connect_opts opt;
			memset(&opt, 0, sizeof(decltype(opt)));
			opt.user_data = this;
			_conn = mg_connect_ws_opt(_mgr, networkEventHandler, opt, addr, nullptr, nullptr);
			fprintf(stdout, "Network (0x%p) opened for connecting: %s.\n", (Network*)this, addr);
		} else if (tobind) {
			struct mg_bind_opts opt;
			memset(&opt, 0, sizeof(decltype(opt)));
			opt.user_data = this;
			std::string stdaddr = addr;
			if (Text::startsWith(stdaddr.c_str(), "ws://", true)) {
				stdaddr = stdaddr.substr(5);
			}
			_conn = mg_bind_opt(_mgr, stdaddr.c_str(), networkEventHandler, opt);
			if (_conn)
				mg_set_protocol_http_websocket(_conn);
			_ready = _conn ? READY : IDLE;
			fprintf(stdout, "Network (0x%p) opened for binding: %s.\n", (Network*)this, addr);
		}
	}
}

void NetworkMongoose::doClose(void) {
	// Do nothing.
}

void NetworkMongoose::doPoll(int timeoutMs) {
	NETWORK_STATE(_polling, false, true, return)

	for (int i = 0; i < 2000; ++i) {
		if (!mg_mgr_poll(_mgr, timeoutMs))
			break;
	}
}

bool NetworkMongoose::doPush(void* ptr, size_t sz, DataTypes y, PushHandler pusher) const {
	if (!ready())
		return false;

	Json* json = nullptr;
	Bytes* bytes = nullptr;
	std::string str;
	switch (y) {
	case STREAM:
		bytes = (Bytes*)ptr;
		assert(bytes->count() == sz);

		break;
	case BYTES:
		bytes = (Bytes*)ptr;
		assert(bytes->count() == sz);

		break;
	case STRING:
		str.assign((const char*)ptr, sz);
		assert(str.length() == sz);

		break;
	case JSON:
		json = (Json*)ptr;

		break;
	case RAW:
		// Push raw.
		if (pusher((const Byte*)ptr, sz, _bytesWithSize, false))
			return true;

		break;
	default:
		assert(false);

		return false;
	}
	if (bytes && y == STREAM) {
		// Push stream.
		if (bytes->empty())
			return false;
		if (!pusher(bytes->pointer(), bytes->count(), false, false)) // Without size head, without EOS.
			return false;
	} else if (bytes && y == BYTES) {
		// Push bytes.
		if (bytes->empty())
			return false;
		if (!pusher(bytes->pointer(), bytes->count(), _bytesWithSize, false)) // With/without size head, without EOS.
			return false;
	} else if (!str.empty()) {
		// Push string.
		if (!pusher((const Byte*)str.c_str(), str.length(), false, true)) // Without size head, with EOS.
			return false;
	} else if (json) {
		// Push JSON.
		if (!json->toString(str) || str.empty())
			return false;
		if (!pusher((const Byte*)str.c_str(), str.length(), false, true)) // Without size head, with EOS.
			return false;
	} else {
		return false;
	}

	return true;
}

bool NetworkMongoose::onSocket(struct mg_connection* nc, int ev, void* evData) {
	switch (ev) {
	case MG_EV_POLL: {
		}

		break;
	case MG_EV_ACCEPT: {
			union socket_address* sa = (union socket_address*)evData;
			AddressName addr;
			networkAddressToString(sa, addr);
			fprintf(stdout, "Network (0x%p) incoming established: %s.\n", (Network*)this, addr.text);
			if (!connective())
				break;

			NETWORK_STATE(_pollingConn, nullptr, nc, break)

			if (!establishedCallback().empty()) {
				EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
				handler(&handler, addr.text);
			}
		}

		break;
	case MG_EV_CONNECT: {
			/*if (nc->flags & MG_F_CLOSE_IMMEDIATELY) {
				fprintf(stdout, "%s\n", "Error connecting to server!\n");
				exit(EXIT_FAILURE);
			}*/
			int status = *(int*)evData;
			if (status == 0) {
				_ready = READY;
				AddressName addr;
				networkAddressToString(&nc->sa, addr);
				fprintf(stdout, "Network (0x%p) outcoming established: %s.\n", (Network*)this, addr.text);
				if (!connective())
					break;

				NETWORK_STATE(_pollingConn, nullptr, nc, break)

				if (!establishedCallback().empty()) {
					EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
					handler(&handler, addr.text);
				}
			} else {
				if (_ready == FAILED)
					break;

				_conn = nullptr;
				_ready = FAILED;
				fprintf(stdout, "Network (0x%p) outcoming establishing error.\n", (Network*)this);
#if !NETWORK_NONCONNECTIVE_CLOSING_ENABLED
				if (!connective())
					break;
#endif /* NETWORK_NONCONNECTIVE_CLOSING_ENABLED */

				NETWORK_STATE(_pollingConn, nullptr, nc, break)

				if (!establishedCallback().empty()) {
					EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
					handler(&handler, nullptr);
				}
			}
		}

		break;
	case MG_EV_RECV: {
			/*int num_bytes = *(int*)evData;*/
			if (!ready())
				break;

			NETWORK_STATE(_pollingConn, nullptr, nc, break)

			AddressName addr;
			networkAddressToString(&nc->sa, addr);
			switch (_dataType) {
			case STREAM: {
					if (!receivedCallback().empty()) {
						Bytes* cached = (!receivedCallback().empty()) ? bytesCache() : nullptr;
						Bytes* bytes = networkReceiveBytes(nc, false, cached);
						while (bytes) {
							if (!receivedCallback().empty()) {
								ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
								handler(&handler, cached, cached->count(), addr.text);
							}

							bytes = networkReceiveBytes(nc, false, cached);
						}
					}
				}

				break;
			case BYTES: {
					if (!receivedCallback().empty()) {
						Bytes* cached = (!receivedCallback().empty()) ? bytesCache() : nullptr;
						Bytes* bytes = networkReceiveBytes(nc, _bytesWithSize, cached);
						while (bytes) {
							if (!receivedCallback().empty()) {
								ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
								handler(&handler, cached, cached->count(), addr.text);
							}

							bytes = networkReceiveBytes(nc, _bytesWithSize, cached);
						}
					}
				}

				break;
			case STRING: {
					if (!receivedCallback().empty()) {
						while (networkReceiveUntilEos(nc, _stringCache)) {
							if (!receivedCallback().empty()) {
								ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
								handler(&handler, (void*)_stringCache.c_str(), _stringCache.length(), addr.text);
							}
						}
					}
				}

				break;
			case JSON: {
					if (!receivedCallback().empty()) {
						while (networkReceiveUntilEos(nc, _stringCache)) {
							if (!receivedCallback().empty()) {
								Json* cached = jsonCache();
								cached->fromString(_stringCache);

								ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
								handler(&handler, (void*)cached, 0, addr.text);
							}
						}
					}
				}

				break;
			default:
				assert(false);

				break;
			}
		}

		break;
	case MG_EV_SEND: {
			/*const int numBytes = *(int*)evData;*/
		}

		break;
	case MG_EV_CLOSE: {
			if (_wasBinded) {
				if (nc->mgr->active_connections) {
					AddressName addr;
					networkAddressToString(&nc->sa, addr);
					fprintf(stdout, "Network (0x%p) incoming disconnected: %s.\n", (Network*)this, addr.text);
					if (!connective())
						break;

					NETWORK_STATE(_pollingConn, nullptr, nc, break)

					if (!disconnectedCallback().empty()) {
						DisconnectedHandler &handler = const_cast<DisconnectedHandler &>(disconnectedCallback());
						handler(&handler, addr.text);
					}
				} else {
					_conn = nullptr;
					_ready = IDLE;
					fprintf(stdout, "Network (0x%p) incoming shutdown.\n", (Network*)this);
				}
			} else {
				if (!_wasBinded && !_conn)
					break;

				_conn = nullptr;
				_ready = IDLE;
				AddressName addr;
				networkAddressToString(&nc->sa, addr);
				fprintf(stdout, "Network (0x%p) outcoming disconnected: %s.\n", (Network*)this, addr.text);
#if !NETWORK_NONCONNECTIVE_CLOSING_ENABLED
				if (!connective())
					break;
#endif /* NETWORK_NONCONNECTIVE_CLOSING_ENABLED */

				NETWORK_STATE(_pollingConn, nullptr, nc, break)

				if (!disconnectedCallback().empty()) {
					DisconnectedHandler &handler = const_cast<DisconnectedHandler &>(disconnectedCallback());
					handler(&handler, addr.text);
				}
			}
		}

		break;
	case MG_EV_TIMER: {
			/*double now = *(double*)evData;*/
		}

		break;
	default:
		return false;
	}

	return true;
}

bool NetworkMongoose::onWebsocket(struct mg_connection* /* nc */, int ev, void* evData) {
	switch (ev) {
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: {
		}

		break;
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
		}

		break;
	case MG_EV_WEBSOCKET_FRAME: {
			struct websocket_message* wm = (struct websocket_message*)evData;
			struct mg_str d = { (char*)wm->data, wm->size };
			(void)d;
		}

		break;
	case MG_EV_WEBSOCKET_CONTROL_FRAME: {
		}

		break;
	default:
		return false;
	}

	return true;
}

Bytes* NetworkMongoose::bytesCache(void) {
	if (!_bytesCache)
		_bytesCache = Bytes::create();

	return _bytesCache;
}

Json* NetworkMongoose::jsonCache(void) {
	if (!_jsonCache)
		_jsonCache = Json::create();

	return _jsonCache;
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */
