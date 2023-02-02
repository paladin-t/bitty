/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include <vector>

/*
** {===========================================================================
** Utilities
*/

template<typename T> union BytesUnion {
	T data = 0;
	Byte bytes[sizeof(T)];
};

/* ===========================================================================} */

/*
** {===========================================================================
** Bytes
*/

class BytesImpl : public Bytes {
private:
	typedef std::vector<Byte> Collection;

private:
	Collection _collection;
	size_t _cursor = 0;

public:
	BytesImpl() {
	}
	virtual ~BytesImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual const Byte* pointer(void) const override {
		if (empty())
			return nullptr;

		return _collection.data();
	}
	virtual Byte* pointer(void) override {
		if (empty())
			return nullptr;

		return _collection.data();
	}

	virtual size_t peek(void) const override {
		return _cursor;
	}
	virtual bool poke(size_t pos) override {
		if (pos > _collection.size())
			return false;

		_cursor = pos;

		return true;
	}
	virtual size_t count(void) const override {
		return _collection.size();
	}
	virtual bool empty(void) const override {
		return _collection.empty();
	}
	virtual bool endOfStream(void) const override {
		return _cursor >= _collection.size();
	}

	virtual Byte readByte(void) override {
		return read<Byte>();
	}
	virtual Int16 readInt16(void) override {
		return read<Int16>();
	}
	virtual UInt16 readUInt16(void) override {
		return read<UInt16>();
	}
	virtual Int32 readInt32(void) override {
		return read<Int32>();
	}
	virtual UInt32 readUInt32(void) override {
		return read<UInt32>();
	}
	virtual Int64 readInt64(void) override {
		return read<Int64>();
	}
	virtual UInt64 readUInt64(void) override {
		return read<UInt64>();
	}
	virtual Single readSingle(void) override {
		return read<Single>();
	}
	virtual Double readDouble(void) override {
		return read<Double>();
	}
	virtual size_t readBytes(Byte* buf, size_t expSize) override {
		if (!buf)
			return 0;

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			memcpy(buf, _collection.data() + _cursor, len);
			_cursor += len;
		}
		if (len < expSize)
			memset(&buf[len], 0, expSize - len);

		return len;
	}
	virtual size_t readBytes(class Bytes* buf, size_t expSize) override {
		if (!buf)
			return 0;

		buf->clear();

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			buf->resize(len);
			memcpy(buf->pointer(), _collection.data() + _cursor, len);
			_cursor += len;
		}

		return len;
	}
	virtual size_t readBytes(class Bytes* buf) override {
		if (!buf)
			return 0;

		buf->clear();

		const size_t len = rest();
		if (len > 0) {
			buf->resize(len);
			memcpy(buf->pointer(), _collection.data() + _cursor, len);
			_cursor += len;
		}

		return len;
	}
	virtual bool readString(char* buf, size_t expSize) override {
		if (endOfStream())
			return false;

		if (expSize == 0)
			return true;

		if (buf)
			*buf = '\0';

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			memcpy(buf, _collection.data() + _cursor, len);
			_cursor += len;
		}
		if (len < expSize)
			memset(&buf[len], '\0', expSize - len);

		return true;
	}
	virtual bool readString(std::string &buf) override {
		buf.clear();

		if (endOfStream())
			return false;

		const size_t len = rest();
		if (len > 0) {
			buf.resize(len);
			memcpy(&buf.front(), _collection.data() + _cursor, len);
			_cursor += len;
		}

		return true;
	}
	virtual bool readLine(char** buf, size_t* readSize) override {
#define _STEP 64

		if (buf)
			*buf = nullptr;
		if (readSize)
			*readSize = 0;

		if (endOfStream())
			return false;

		Byte* ln = nullptr;
		size_t size = 0;
		size_t capacity = 0;
		while (!endOfStream()) {
			++size;
			if (capacity == 0) {
				capacity = _STEP;
				ln = (Byte*)realloc(ln, capacity);
			} else if (capacity < size) {
				capacity += _STEP;
				ln = (Byte*)realloc(ln, capacity);
			}
			ln[size - 1] = readByte();
			if (ln[size - 1] == '\r') {
				break;
			} else if (ln[size - 1] == '\n') {
				const size_t l = peek();
				if (readByte() != '\r')
					poke(l);

				break;
			}
		}
		if (ln)
			ln[size - 1] = '\0';
		if (buf) {
			if (size) {
				*buf = new char[size];
				memcpy(*buf, ln, size);
			} else {
				*buf = new char[1];
				**buf = '\0';
			}
		}
		free(ln);
		if (readSize)
			*readSize = size ? size - 1 : 0;

		return true;

#undef _STEP
	}
	virtual bool readLine(std::string &buf) override {
		char* tmp = nullptr;
		size_t len = 0;
		const bool result = readLine(&tmp, &len);
		if (tmp) {
			buf.assign(tmp, len);
			delete [] tmp;
		}

		return result;
	}
	virtual bool readLine(void) override {
		return readLine(nullptr, nullptr);
	}

	virtual int writeByte(Byte val) override {
		advance(sizeof(val));
		_collection[_cursor] = val;
		_cursor += sizeof(val);

		return (int)sizeof(val);
	}
	virtual int writeInt16(Int16 val) override {
		return write(val);
	}
	virtual int writeUInt16(UInt16 val) override {
		return write(val);
	}
	virtual int writeInt32(Int32 val) override {
		return write(val);
	}
	virtual int writeUInt32(UInt32 val) override {
		return write(val);
	}
	virtual int writeInt64(Int64 val) override {
		return write(val);
	}
	virtual int writeUInt64(UInt64 val) override {
		return write(val);
	}
	virtual int writeSingle(Single val) override {
		return write(val);
	}
	virtual int writeDouble(Double val) override {
		return write(val);
	}
	virtual int writeBytes(const Byte* val, size_t len) override {
		if (val && len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val, len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeBytes(const class Bytes* val, size_t len) override {
		if (val && !val->empty() && len > 0) {
			len = std::min(len, val->count());
			advance(len);
			memcpy(_collection.data() + _cursor, val->pointer(), len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeBytes(const class Bytes* val) override {
		if (val && !val->empty()) {
			const size_t len = val->count();
			advance(len);
			memcpy(_collection.data() + _cursor, val->pointer(), len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeString(const char* val, size_t len) override {
		if (val && len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val, len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeString(const char* val) override {
		const size_t len = val ? strlen(val) : 0;
		if (val && len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val, len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeString(const std::string &val) override {
		const size_t len = val.length();
		if (len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val.c_str(), len);
			_cursor += len;

			return (int)len;
		}

		return 0;
	}
	virtual int writeLine(const char* val, size_t len) override {
		int result = 0;
		if (val && len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val, len);
			_cursor += len;
			result += (int)len;
		}

		advance(1);
		_collection[_cursor++] = '\n';
		++result;

		return result;
	}
	virtual int writeLine(const char* val) override {
		int result = 0;
		const size_t len = val ? strlen(val) : 0;
		if (val && len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val, len);
			_cursor += len;
			result += (int)len;
		}

		advance(1);
		_collection[_cursor++] = '\n';
		++result;

		return result;
	}
	virtual int writeLine(const std::string &val) override {
		int result = 0;
		const size_t len = val.length();
		if (len > 0) {
			advance(len);
			memcpy(_collection.data() + _cursor, val.c_str(), len);
			_cursor += len;
			result += (int)len;
		}

		advance(1);
		_collection[_cursor++] = '\n';
		++result;

		return result;
	}
	virtual int writeLine(void) override {
		advance(1);
		_collection[_cursor++] = '\n';

		return 1;
	}

	virtual const Byte &get(size_t index) const override {
		return _collection.at(index);
	}
	virtual void set(size_t index, Byte val) override {
		_collection.at(index) = val;
	}

	virtual Bytes* resize(size_t expSize) override {
		const size_t oldSize = _collection.size();
		_collection.resize(expSize);
		if (expSize <= oldSize / 2)
			_collection.shrink_to_fit();

		if (empty())
			_cursor = 0;
		else if (_cursor > _collection.size())
			_cursor = _collection.size();

		return this;
	}
	virtual void clear(void) override {
		_collection.clear();
		_cursor = 0;
	}

private:
	size_t rest(void) const {
		const size_t len = count();
		if (_cursor >= len)
			return 0;

		return len - _cursor;
	}

	template<typename T> T read(void) {
		if (_cursor + sizeof(T) > _collection.size())
			return 0;

		T* result = (T*)&_collection.at(_cursor);
		_cursor += sizeof(*result);

		return *result;
	}
	template<typename T> int write(T val) {
		BytesUnion<decltype(val)> u;
		u.data = val;
		advance(sizeof(T));
		memcpy(_collection.data() + _cursor, u.bytes, sizeof(T));
		_cursor += sizeof(T);

		return (int)sizeof(T);
	}

	void advance(size_t len) {
		if (_cursor + len > _collection.size()) {
			const size_t diff = (_cursor + len) - _collection.size();
			_collection.resize(_collection.size() + diff);
		}
	}
};

Bytes* Bytes::create(void) {
	BytesImpl* result = new BytesImpl();

	return result;
}

void Bytes::destroy(Bytes* ptr) {
	BytesImpl* impl = static_cast<BytesImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
