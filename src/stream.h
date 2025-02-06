/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __STREAM_H__
#define __STREAM_H__

#include "bitty.h"
#include "mathematics.h"

/*
** {===========================================================================
** Stream
*/

/**
 * @brief Streaming interface.
 */
class Stream {
public:
	enum Accesses {
		READ = 1,
		WRITE = 2,
		APPEND = 3,
		READ_WRITE = 4
	};

public:
	virtual size_t peek(void) const = 0;
	virtual bool poke(size_t pos) = 0;
	virtual size_t count(void) const = 0;
	virtual bool empty(void) const = 0;
	virtual bool endOfStream(void) const = 0;

	virtual Byte readByte(void) = 0;
	virtual Int16 readInt16(void) = 0;
	virtual UInt16 readUInt16(void) = 0;
	virtual Int32 readInt32(void) = 0;
	virtual UInt32 readUInt32(void) = 0;
	virtual Int64 readInt64(void) = 0;
	virtual UInt64 readUInt64(void) = 0;
	virtual Single readSingle(void) = 0;
	virtual Double readDouble(void) = 0;
	/**
	 * @param[out] buf
	 */
	virtual size_t readBytes(Byte* buf, size_t expSize) = 0;
	/**
	 * @param[out] buf
	 */
	virtual size_t readBytes(class Bytes* buf, size_t expSize) = 0;
	/**
	 * @param[out] buf
	 */
	virtual size_t readBytes(class Bytes* buf) = 0;
	/**
	 * @param[out] buf
	 */
	virtual bool readString(char* buf, size_t expSize) = 0;
	/**
	 * @param[out] buf
	 */
	virtual bool readString(std::string &buf) = 0;
	/**
	 * @param[out] buf
	 * @param[out] readSize
	 */
	virtual bool readLine(char** buf /* nullable */, size_t* readSize /* nullable */) = 0;
	/**
	 * @param[out] buf
	 */
	virtual bool readLine(std::string &buf) = 0;
	virtual bool readLine(void) = 0;

	virtual int writeByte(Byte val) = 0;
	virtual int writeInt16(Int16 val) = 0;
	virtual int writeUInt16(UInt16 val) = 0;
	virtual int writeInt32(Int32 val) = 0;
	virtual int writeUInt32(UInt32 val) = 0;
	virtual int writeInt64(Int64 val) = 0;
	virtual int writeUInt64(UInt64 val) = 0;
	virtual int writeSingle(Single val) = 0;
	virtual int writeDouble(Double val) = 0;
	virtual int writeBytes(const Byte* val, size_t len) = 0;
	virtual int writeBytes(const class Bytes* val, size_t len) = 0;
	virtual int writeBytes(const class Bytes* val) = 0;
	virtual int writeString(const char* val, size_t len) = 0;
	virtual int writeString(const char* val) = 0;
	virtual int writeString(const std::string &val) = 0;
	virtual int writeLine(const char* val, size_t len) = 0;
	virtual int writeLine(const char* val) = 0;
	virtual int writeLine(const std::string &val) = 0;
	virtual int writeLine(void) = 0;
};

/* ===========================================================================} */

#endif /* __STREAM_H__ */
