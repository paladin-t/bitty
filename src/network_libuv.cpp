/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#define NOMINMAX
#include "bytes.h"
#include "datetime.h"
#include "encoding.h"
#include "json.h"
#include "network_libuv.h"
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

#ifndef NETWORK_DEFAULT_BACKLOG
#	define NETWORK_DEFAULT_BACKLOG 128
#endif /* NETWORK_DEFAULT_BACKLOG */

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

static void networkAddressToString(const uv_tcp_t* tcp, Network::AddressName &addr) {
	assert(BITTY_COUNTOF(addr.text) >= strlen(NETWORK_NULL_STRING) + 1);
	memset(&addr, 0, sizeof(Network::AddressName));
	memcpy(addr.text, NETWORK_NULL_STRING, strlen(NETWORK_NULL_STRING));

	struct sockaddr_storage addr_;
	memset(&addr_, 0, sizeof(sockaddr_storage));
	int alen = sizeof(sockaddr_storage);
	int ret = uv_tcp_getpeername(tcp, (struct sockaddr*)&addr_, &alen);
	if (ret) {
		fprintf(stderr, "Get peer name error %s.\n", uv_strerror(ret));

		return;
	}

	int port = 0;
	if (addr_.ss_family == AF_INET) {
		struct sockaddr_in* addr_i4 = (struct sockaddr_in*)&addr_;
		uv_ip4_name(addr_i4, addr.text, sizeof(Network::AddressName));
		port = addr_i4->sin_port;
	} else if (addr_.ss_family == AF_INET6) {
		struct sockaddr_in6* addr_i6 = (struct sockaddr_in6*)&addr_;
		uv_ip6_name(addr_i6, addr.text, sizeof(Network::AddressName));
		port = addr_i6->sin6_port;
	}
	const std::string str = addr.text + std::string(":") + Text::toString(port);
	memcpy(addr.text, str.c_str(), std::min(sizeof(Network::AddressName), str.length()));
}

static void networkWrite(uv_stream_t* handle, const Byte* buf, size_t len) {
	typedef struct {
		uv_write_t req;
		uv_buf_t buf;
	} write_req_t;

	write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
	req->buf = uv_buf_init((char*)buf, (unsigned int)len);
	uv_write(
		(uv_write_t*)req, handle, &req->buf, 1,
		[] (uv_write_t* req, int status) -> void {
			if (status)
				fprintf(stderr, "Write error %s.\n", uv_strerror(status));

			write_req_t* wr = (write_req_t*)req;
			free(wr);
		}
	);
}

static bool networkSend(
	uv_stream_t* handle, bool limitedSize,
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
		networkWrite(handle, (const Byte*)&head, 4);
	}
	networkWrite(handle, buf, len);
	if (withEos) {
		constexpr const char eos[1] = { '\0' };
		networkWrite(handle, (const Byte*)eos, 1);
	}

	return true;
}

static bool networkBroadcast(
	NetworkLibuv::TcpClientHandles &clients, bool limitedSize,
	const Byte* buf, size_t len, bool bytesWithSize, bool withEos
) {
	bool ret = true;
	for (uv_tcp_t* tcp : clients) {
		if (!networkSend((uv_stream_t*)tcp, limitedSize, buf, len, bytesWithSize, withEos))
			ret = false;
	}

	return ret;
};

static Bytes* networkReceiveBytes(uv_stream_t* /* handle */, ssize_t /* nread */, const uv_buf_t* /* buf */, bool bytesWithSize, Bytes* cached, Bytes* receiving) {
	if (bytesWithSize) {
		if (receiving->count() >= sizeof(Network::BytesSize)) {
			Network::BytesSize* up = (Network::BytesSize*)receiving->pointer();
			if (receiving->count() >= *up) {
				if (cached) {
					cached->clear();
					cached->writeBytes(
						(const Byte*)(receiving->pointer() + sizeof(Network::BytesSize)),
						(size_t)(*up - sizeof(Network::BytesSize))
					);
				} else {
					assert(false && "Wrong data.");
				}
				receiving->removeFront((size_t)(*up));

				return cached;
			}
		}
	} else {
		if (receiving->count() > 0) {
			if (cached) {
				cached->clear();
				cached->writeBytes(receiving->pointer(), receiving->count());
			} else {
				assert(false && "Wrong data.");
			}
			receiving->removeFront(receiving->count());

			return cached;
		}
	}

	return nullptr;
}

static bool networkReceiveUntilEos(uv_stream_t* /* handle */, ssize_t /* nread */, const uv_buf_t* /* buf */, std::string &str, Bytes* receiving) {
	for (size_t i = 0; i < receiving->count(); ++i) {
		if (i >= NETWORK_MESSAGE_MAX_SIZE || receiving->get(i) == '\0') {
			str.clear();
			str.assign((const char*)receiving->pointer(), i);
			receiving->removeFront(i + 1);

			return true;
		}
	}

	return false;
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Network implementation with the libuv backend
*/

#if BITTY_NETWORK_ENABLED

NetworkLibuv::NetworkLibuv() {
	_loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
	memset(_loop, 0, sizeof(uv_loop_t));

#if defined BITTY_DEBUG
	fprintf(stdout, "Network (libuv) created.\n");
#endif /* BITTY_DEBUG */
}

NetworkLibuv::~NetworkLibuv() {
	if (_opened)
		close();

	if (_connect) {
		free(_connect);
		_connect = nullptr;
	}
	for (uv_tcp_t* tcp : _tcpClients) {
		free(tcp);
	}
	_tcpClients.clear();
	if (_udp) {
		free(_udp);
		_udp = nullptr;
	}
	if (_tcp) {
		free(_tcp);
		_tcp = nullptr;
	}

	free(_loop);
	_loop = nullptr;

#if defined BITTY_DEBUG
	fprintf(stdout, "Network (libuv) destroyed.\n");
#endif /* BITTY_DEBUG */
}

unsigned NetworkLibuv::type(void) const {
	return TYPE();
}

bool NetworkLibuv::open(const char* addr, Protocols protocol, bool* toconn_, bool* tobind_) {
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
	uv_loop_init(_loop);
	_loop->data = this;

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
	assert((withudp || withtcp || withws) && "Unknown protocol.");

	bool toconn = false, tobind = false;
	std::string ipaddr;
	int port = 0;
	if (strop == ">") {
		toconn = true;
		if (toconn_)
			*toconn_ = true;
		const Text::Array parts = Text::split(strdirt, ":");
		if (parts.size() == 1) {
			ipaddr = "127.0.0.1";
			char* conv_suc = nullptr;
			const long lval = Text::strtol(strdirt.c_str(), &conv_suc, 0);
			port = (int)lval;
		} else if (parts.size() == 2) {
			ipaddr = parts[0];
			char* conv_suc = nullptr;
			const long lval = Text::strtol(parts[1].c_str(), &conv_suc, 0);
			port = (int)lval;
		}
	} else if (strop == "<") {
		tobind = true;
		if (tobind_)
			*tobind_ = true;
		ipaddr = "0.0.0.0";
		char* conv_suc = nullptr;
		const long lval = Text::strtol(strdirt.c_str(), &conv_suc, 0);
		port = (int)lval;
	} else /* if (strop == "*") */ {
		const Text::Array parts = Text::split(strdirt, ":");
		if (parts.size() == 1) {
			tobind = true;
			if (tobind_)
				*tobind_ = true;
			ipaddr = "0.0.0.0";
			char* conv_suc = nullptr;
			const long lval = Text::strtol(strdirt.c_str(), &conv_suc, 0);
			port = (int)lval;
		} else if (parts.size() == 2) {
			toconn = true;
			if (toconn_)
				*toconn_ = true;
			ipaddr = parts[0];
			char* conv_suc = nullptr;
			const long lval = Text::strtol(parts[1].c_str(), &conv_suc, 0);
			port = (int)lval;
		}
	}
	assert((toconn || tobind) && "Unknown operation.");

	// Open.
	doOpen(withudp, withtcp, withws, toconn, tobind, ipaddr.c_str(), port);

	// Finish.
	return true;
}

bool NetworkLibuv::close(void) {
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

	// Dispose.
	uv_walk(
		_loop,
		[] (uv_handle_t* handle, void* /* arg */) -> void {
			uv_close(handle, nullptr);
		},
		0
	);
	uv_run(_loop, UV_RUN_DEFAULT);
	uv_loop_close(_loop);

	if (_recvCache) {
		Bytes::destroy(_recvCache);
		_recvCache = nullptr;
	}

	// Clear options.
	_options.clear();
	_dataType = JSON;
	_bytesWithSize = true;

	// Call polymorphic.
	doClose();

	// Finish.
	return true;
}

Network::DataTypes NetworkLibuv::dataType(void) const {
	return _dataType;
}

void NetworkLibuv::dataType(DataTypes y) {
	_dataType = y;
}

std::string NetworkLibuv::option(const std::string &key) const {
	std::string skey = key;
	Text::toLowerCase(skey);
	OptionDictionary::const_iterator it = _options.find(skey);
	if (it != _options.end())
		return it->second;

	return "";
}

void NetworkLibuv::option(const std::string &key, const std::string &val) {
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

bool NetworkLibuv::ready(void) const {
	return _opened && (_ready == READY) && !_shutting;
}

bool NetworkLibuv::polling(void) const {
	return _polling;
}

bool NetworkLibuv::connective(void) const {
	return (_protocol & (TCP | WEBSOCKET)) != NONE;
}

void NetworkLibuv::poll(int timeoutMs) {
	if (!_opened && !_shutting)
		return;

	doPoll(timeoutMs);
}

void NetworkLibuv::establish(void) {
	const long long now = DateTime::ticks();
	while (_ready == IDLE) {
		doPoll(1);

		const double diff = DateTime::toSeconds(DateTime::ticks() - now);
		if (diff > (1.0 * (NETWORK_TIMEOUT_SECONDS)))
			break;
	}
}

void NetworkLibuv::disconnect(void) {
	if (!_opened)
		return;

	auto disconnect_ = [this] (uv_tcp_t* tcp) -> void {
		//uv_stream_t* stream = _connect->handle;
		uv_stream_t* stream = (uv_stream_t*)tcp;
		if (_connect) {
			onClosed(stream);
		} else {
			TcpClientHandles tcpClients = _tcpClients;
			for (uv_tcp_t* tcp_ : tcpClients)
				onClosed((uv_stream_t*)tcp_);
		}

		const size_t writeQueueSize = stream->write_queue_size;
		if (uv_is_writable(stream) && writeQueueSize > 0) {
			uv_shutdown_t* shutdownReq = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
			uv_shutdown(
				shutdownReq, stream,
				[] (uv_shutdown_t* req, int /* status */) -> void{
					if (!uv_is_closing((uv_handle_t*)req->handle)) {
						uv_close((uv_handle_t*)req->handle, [] (uv_handle_t* handle) -> void { free(handle); });
					}
					free(req);
				}
			);
		} else if (uv_is_readable(stream)) {
			uv_read_stop(stream);

			uv_close((uv_handle_t*)stream, [] (uv_handle_t* handle) -> void { free(handle); });

			for (; ; ) {
				if (!uv_run(_loop, UV_RUN_NOWAIT))
					break;
			}
		} else {
			uv_close((uv_handle_t*)stream, [] (uv_handle_t* handle) -> void { free(handle); });

			for (; ; ) {
				if (!uv_run(_loop, UV_RUN_NOWAIT))
					break;
			}
		}
	};

	if (_tcp) {
		disconnect_(_tcp);
		_tcp = nullptr;
	}

	if (!_wasBinded) {
		_ready = IDLE;

		fprintf(stdout, "Network (0x%p) incoming shutdown.\n", (Network*)this);
	}
}

bool NetworkLibuv::send(void* ptr, size_t sz, DataTypes y) {
	if (!_connect)
		return false;

	PushHandler pusher = std::bind(
		networkSend, _connect->handle, false,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);

	return doPush(ptr, sz, y, pusher);
}

bool NetworkLibuv::broadcast(void* ptr, size_t sz, DataTypes y) {
	if (!_binded)
		return false;
	if (!connective())
		return false;

	if (_tcpClients.empty())
		return true;

	PushHandler pusher = std::bind(
		networkBroadcast, _tcpClients, false,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);

	return doPush(ptr, sz, y, pusher);
}

bool NetworkLibuv::update(double /* delta */) {
	if (!ready())
		return true;

	doPoll(_timeoutMs);

	if (_shutting)
		close();

	return _opened;
}

const Network::ReceivedHandler &NetworkLibuv::receivedCallback(void) const {
	return _recvHandler;
}

const Network::EstablishedHandler &NetworkLibuv::establishedCallback(void) const {
	return _stblHandler;
}

const Network::DisconnectedHandler &NetworkLibuv::disconnectedCallback(void) const {
	return _dscnHandler;
}

void NetworkLibuv::callback(const ReceivedHandler &cb) {
	_recvHandler = cb;
}

void NetworkLibuv::callback(const EstablishedHandler &cb) {
	_stblHandler = cb;
}

void NetworkLibuv::callback(const DisconnectedHandler &cb) {
	_dscnHandler = cb;
}

void NetworkLibuv::callback(const ReceivedHandler &recvCb, const EstablishedHandler &stbCb, const DisconnectedHandler &dscnCb) {
	_recvHandler = recvCb;
	_stblHandler = stbCb;
	_dscnHandler = dscnCb;
}

void NetworkLibuv::onAccepted(uv_stream_t* handle, int status) {
	if (status < 0)
		fprintf(stderr, "Accept error %s.\n", uv_strerror(status));

	if (!connective())
		return;

	AddressName addr;

	uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(client, 0, sizeof(uv_tcp_t));
	_tcpClients.push_back(client);
	uv_tcp_init(_loop, client);
	client->data = this;
	if (uv_accept(handle, (uv_stream_t*)client) == 0) {
		networkAddressToString(client, addr);
		fprintf(stdout, "Network (0x%p) incoming established: %s.\n", (Network*)this, addr.text);

		uv_read_start(
			(uv_stream_t*)client,
			[] (uv_handle_t* /* handle */, size_t suggested_size, uv_buf_t* buf) -> void {
				buf->base = (char*)malloc(suggested_size);
				buf->len = (decltype(buf->len))suggested_size;
			},
			[] (uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) -> void {
				NetworkLibuv* self = (NetworkLibuv*)handle->data;
				if (nread > 0) {
					self->onReceived(handle, nread, buf);
					free(buf->base);
				} else if (nread == UV_EOF) {
					self->onClosed(handle);
					if (buf->base)
						free(buf->base);
				}
			}
		);
	} else {
		uv_close((uv_handle_t*)client, nullptr);
	}

	if (!establishedCallback().empty()) {
		EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
		handler(&handler, addr.text);
	}
}

void NetworkLibuv::onConnected(uv_connect_t* svr, int status) {
	if (status == 0) {
		_ready = READY;

		AddressName addr;
		networkAddressToString((const uv_tcp_t*)svr->handle, addr);
		fprintf(stdout, "Network (0x%p) outcoming established: %s.\n", (Network*)this, addr.text);
		if (!connective())
			return;

		uv_read_start(
			svr->handle,
			[] (uv_handle_t* /* handle */, size_t suggested_size, uv_buf_t* buf) -> void {
				buf->base = (char*)malloc(suggested_size);
				buf->len = (decltype(buf->len))suggested_size;
			},
			[] (uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) -> void {
				NetworkLibuv* self = (NetworkLibuv*)handle->data;
				if (nread > 0) {
					self->onReceived(handle, nread, buf);
					free(buf->base);
				} else if (nread == UV_EOF) {
					self->onClosed(handle);
					if (buf->base)
						free(buf->base);
				}
			}
		);

		if (!establishedCallback().empty()) {
			EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
			handler(&handler, addr.text);
		}
	} else {
		if (_ready == FAILED)
			return;

		if (_connect) {
			free(_connect);
			_connect = nullptr;
		}

		_ready = FAILED;

		fprintf(stdout, "Network (0x%p) outcoming establishing error.\n", (Network*)this);

#if !NETWORK_NONCONNECTIVE_CLOSING_ENABLED
		if (!connective())
			return;
#endif /* NETWORK_NONCONNECTIVE_CLOSING_ENABLED */

		if (!establishedCallback().empty()) {
			EstablishedHandler &handler = const_cast<EstablishedHandler &>(establishedCallback());
			handler(&handler, nullptr);
		}
	}
}

void NetworkLibuv::onReceived(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
	if (!ready())
		return;

	AddressName addr;
	networkAddressToString((const uv_tcp_t*)handle, addr);
	switch (_dataType) {
	case STREAM: {
			if (!receivedCallback().empty()) {
				Bytes* receiving = recvCache();
				receiving->clear();
				receiving->writeBytes((const Byte*)buf->base, (size_t)nread);

				Bytes* cached = (!receivedCallback().empty()) ? bytesCache() : nullptr;
				Bytes* bytes = networkReceiveBytes(handle, nread, buf, false, cached, receiving);
				while (bytes) {
					if (!receivedCallback().empty()) {
						ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
						handler(&handler, cached, cached->count(), addr.text);
					}

					bytes = networkReceiveBytes(handle, nread, buf, false, cached, receiving);
				}
			}
		}

		break;
	case BYTES: {
			if (!receivedCallback().empty()) {
				Bytes* receiving = recvCache();
				receiving->clear();
				receiving->writeBytes((const Byte*)buf->base, (size_t)nread);

				Bytes* cached = (!receivedCallback().empty()) ? bytesCache() : nullptr;
				Bytes* bytes = networkReceiveBytes(handle, nread, buf, _bytesWithSize, cached, receiving);
				while (bytes) {
					if (!receivedCallback().empty()) {
						ReceivedHandler &handler = const_cast<ReceivedHandler &>(receivedCallback());
						handler(&handler, cached, cached->count(), addr.text);
					}

					bytes = networkReceiveBytes(handle, nread, buf, _bytesWithSize, cached, receiving);
				}
			}
		}

		break;
	case STRING: {
			if (!receivedCallback().empty()) {
				Bytes* receiving = recvCache();
				receiving->clear();
				receiving->writeBytes((const Byte*)buf->base, (size_t)nread);

				while (networkReceiveUntilEos(handle, nread, buf, _stringCache, receiving)) {
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
				Bytes* receiving = recvCache();
				receiving->clear();
				receiving->writeBytes((const Byte*)buf->base, (size_t)nread);

				while (networkReceiveUntilEos(handle, nread, buf, _stringCache, receiving)) {
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
		assert(false && "Unknown data type.");

		break;
	}
}

void NetworkLibuv::onClosed(uv_stream_t* handle) {
	if (_wasBinded) {
		AddressName addr;
		networkAddressToString((const uv_tcp_t*)handle, addr);

		fprintf(stdout, "Network (0x%p) incoming disconnected: %s.\n", (Network*)this, addr.text);

		if (!connective())
			return;

		if (!disconnectedCallback().empty()) {
			DisconnectedHandler &handler = const_cast<DisconnectedHandler &>(disconnectedCallback());
			handler(&handler, addr.text);
		}

		TcpClientHandles::iterator it = std::find_if(
			_tcpClients.begin(), _tcpClients.end(),
			[&] (uv_tcp_t* &ptr) -> bool {
				return (uv_stream_t*)ptr == handle;
			}
		);
		if (it != _tcpClients.end()) {
			uv_close((uv_handle_t*)(*it), [] (uv_handle_t* handle) -> void { free(handle); });
			_tcpClients.erase(it);
		}
	} else {
		if (!_connect)
			return;

		free(_connect);
		_connect = nullptr;

		_ready = IDLE;

		AddressName addr;
		networkAddressToString((const uv_tcp_t*)handle, addr);

		fprintf(stdout, "Network (0x%p) outcoming disconnected: %s.\n", (Network*)this, addr.text);

#if !NETWORK_NONCONNECTIVE_CLOSING_ENABLED
		if (!connective())
			return;
#endif /* NETWORK_NONCONNECTIVE_CLOSING_ENABLED */

		if (!disconnectedCallback().empty()) {
			DisconnectedHandler &handler = const_cast<DisconnectedHandler &>(disconnectedCallback());
			handler(&handler, addr.text);
		}
	}
}

void NetworkLibuv::doOpen(bool withudp, bool withtcp, bool withws, bool toconn, bool tobind, const char* ipaddr, int port) {
	_wasBinded = _binded = tobind;
	if (withudp || withtcp) {
		if (toconn) {
			memset(&_address, 0, sizeof(sockaddr_in));
			uv_ip4_addr(ipaddr, port, &_address);

			int ret = 0;
			if (withudp) {
				if (!_udp)
					_udp = (uv_udp_t*)malloc(sizeof(uv_udp_t));
				memset(_udp, 0, sizeof(uv_udp_t));
				_udp->data = this;
				uv_udp_init(_loop, _udp);

				ret = uv_udp_connect(_udp, (const struct sockaddr*)&_address);
			} else if (withtcp) {
				if (!_tcp)
					_tcp = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
				memset(_tcp, 0, sizeof(uv_tcp_t));
				_tcp->data = this;
				uv_tcp_init(_loop, _tcp);

				if (!_connect)
					_connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
				memset(_connect, 0, sizeof(uv_connect_t));
				_connect->data = this;
				ret = uv_tcp_connect(_connect, _tcp, (const struct sockaddr*)&_address, [] (uv_connect_t* req, int status) -> void {
					if (status < 0)
						fprintf(stderr, "Connect error %s.\n", uv_strerror(status));

					NetworkLibuv* self = (NetworkLibuv*)req->data;
					self->onConnected(req, status);
				});
			}

			if (ret)
				fprintf(stderr, "Network (0x%p) connecting to %s:%d error: %s.\n", (Network*)this, ipaddr, port, uv_strerror(ret));
			else
				fprintf(stdout, "Network (0x%p) opened for connecting: %s:%d.\n", (Network*)this, ipaddr, port);

			if (withudp)
				_ready = ret ? IDLE : READY;
		} else if (tobind) {
			memset(&_address, 0, sizeof(sockaddr_in));
			uv_ip4_addr(ipaddr, port, &_address);

			int ret = 0;
			if (withudp) {
				if (!_udp)
					_udp = (uv_udp_t*)malloc(sizeof(uv_udp_t));
				memset(_udp, 0, sizeof(uv_udp_t));
				_udp->data = this;
				uv_udp_init(_loop, _udp);

				uv_udp_bind(_udp, (const struct sockaddr*)&_address, 0);
			} else if (withtcp) {
				if (!_tcp)
					_tcp = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
				memset(_tcp, 0, sizeof(uv_tcp_t));
				_tcp->data = this;
				uv_tcp_init(_loop, _tcp);

				uv_tcp_bind(_tcp, (const struct sockaddr*)&_address, 0);

				ret = uv_listen((uv_stream_t*)_tcp, NETWORK_DEFAULT_BACKLOG, [] (uv_stream_t* server, int status) -> void {
					if (status < 0)
						fprintf(stderr, "Listen error %s.\n", uv_strerror(status));

					NetworkLibuv* self = (NetworkLibuv*)server->data;
					self->onAccepted(server, status);
				});
			}

			if (ret)
				fprintf(stderr, "Network (0x%p) binding to %s:%d error: %s.\n", (Network*)this, ipaddr, port, uv_strerror(ret));
			else
				fprintf(stdout, "Network (0x%p) opened for binding: %s:%d.\n", (Network*)this, ipaddr, port);

			_ready = ret ? IDLE : READY;
		}
	} else if (withws) {
		assert(false && "Not implemented.");
	}
}

void NetworkLibuv::doClose(void) {
	// Do nothing.
}

void NetworkLibuv::doPoll(int /* timeoutMs */) {
	NETWORK_STATE(_polling, false, true, return)

	for (int i = 0; i < 2000; ++i) {
		if (!uv_run(_loop, UV_RUN_NOWAIT))
			break;
	}
}

bool NetworkLibuv::doPush(void* ptr, size_t sz, DataTypes y, PushHandler pusher) const {
	if (!ready())
		return false;

	Json* json = nullptr;
	Bytes* bytes = nullptr;
	std::string str;
	switch (y) {
	case STREAM:
		bytes = (Bytes*)ptr;
		assert(bytes->count() == sz && "Wrong data.");

		break;
	case BYTES:
		bytes = (Bytes*)ptr;
		assert(bytes->count() == sz && "Wrong data.");

		break;
	case STRING:
		str.assign((const char*)ptr, sz);
		assert(str.length() == sz && "Wrong data.");

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
		assert(false && "Unknown data type.");

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

class Bytes* NetworkLibuv::recvCache(void) {
	if (!_recvCache)
		_recvCache = Bytes::create();

	return _recvCache;
}

class Bytes* NetworkLibuv::bytesCache(void) {
	if (!_bytesCache)
		_bytesCache = Bytes::create();

	return _bytesCache;
}

class Json* NetworkLibuv::jsonCache(void) {
	if (!_jsonCache)
		_jsonCache = Json::create();

	return _jsonCache;
}

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */
