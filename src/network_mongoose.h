/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __NETWORK_MONGOOSE_H__
#define __NETWORK_MONGOOSE_H__

#include "mathematics.h"
#include "network.h"
#include <map>

/*
** {===========================================================================
** Network implementation with the Mongoose backend
*/

#if BITTY_NETWORK_ENABLED

class NetworkMongoose : public Network {
private:
	typedef std::map<std::string, std::string> OptionDictionary;

	typedef std::function<bool(const Byte*, size_t, bool, bool)> PushHandler;

private:
	/**< States. */

	bool _opened = false;
	States _ready = IDLE;
	int _shutting = 0;
	bool _binded = false, _wasBinded = false;
	Protocols _protocol = NONE;

	bool _polling = false;

	/**< Options. */

	OptionDictionary _options;
	DataTypes _dataType = BYTES;
	bool _bytesWithSize = true;
	int _timeoutMs = 1;

	/**< Connection. */

	struct mg_mgr* _mgr = nullptr;
	struct mg_connection* _conn = nullptr;

	/**< Callbacks. */

	ReceivedHandler _recvHandler;
	EstablishedHandler _stblHandler;
	DisconnectedHandler _dscnHandler;

	std::string _stringCache;
	class Bytes* _bytesCache = nullptr;
	class Json* _jsonCache = nullptr;

	struct mg_connection* _pollingConn = nullptr;

public:
	NetworkMongoose();
	virtual ~NetworkMongoose() override;

	virtual unsigned type(void) const override;

	virtual bool open(const char* addr, Protocols protocol, bool* toconn, bool* tobind) override;
	virtual bool close(void) override;

	virtual DataTypes dataType(void) const override;
	virtual void dataType(DataTypes y) override;

	virtual std::string option(const std::string &key) const override;
	virtual void option(const std::string &key, const std::string &val) override;

	virtual bool ready(void) const override;

	virtual bool polling(void) const override;
	virtual bool connective(void) const override;

	virtual void poll(int timeoutMs) override;
	virtual void establish(void) override;
	virtual void disconnect(void) override;

	virtual bool send(void* ptr, size_t sz, DataTypes y) override;
	virtual bool broadcast(void* ptr, size_t sz, DataTypes y, bool filterPolling) override;

	virtual bool update(double delta) override;

	virtual const ReceivedHandler &receivedCallback(void) const override;
	virtual const EstablishedHandler &establishedCallback(void) const override;
	virtual const DisconnectedHandler &disconnectedCallback(void) const override;
	virtual void callback(const ReceivedHandler &cb) override;
	virtual void callback(const EstablishedHandler &cb) override;
	virtual void callback(const DisconnectedHandler &cb) override;
	virtual void callback(const ReceivedHandler &recvCb, const EstablishedHandler &stbCb, const DisconnectedHandler &dscnCb) override;
	void callback(struct mg_connection* nc, int ev, void* evData);

private:
	void doOpen(bool withudp, bool withtcp, bool withws, bool toconn, bool tobind, const char* addr);
	void doClose(void);

	void doPoll(int timeoutMs);
	bool doPush(void* ptr, size_t sz, DataTypes y, PushHandler pusher) const;

	bool onSocket(struct mg_connection* nc, int ev, void* evData);
	bool onWebsocket(struct mg_connection* nc, int ev, void* evData);

	class Bytes* bytesCache(void);
	class Json* jsonCache(void);
};

#endif /* BITTY_NETWORK_ENABLED */

/* ===========================================================================} */

#endif /* __NETWORK_MONGOOSE_H__ */
