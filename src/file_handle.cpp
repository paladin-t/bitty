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
#include "encoding.h"
#include "file_handle.h"

/*
** {===========================================================================
** File
*/

class FileImpl : public File {
private:
	FILE* _file = nullptr;

public:
	FileImpl() {
	}
	virtual ~FileImpl() override {
		close();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual const FILE* pointer(void) const override {
		return _file;
	}
	virtual FILE* pointer(void) override {
		return _file;
	}

	virtual bool open(const char* path, Accesses access) override {
		close();

		std::string m;
		switch (access) {
		case READ: m = "rb"; break;
		case WRITE: m = "wb"; break;
		case APPEND: m = "ab"; break;
		case READ_WRITE: m = "rb+"; break;
		}

		if (m.empty())
			return false;

		const std::string osstr = Unicode::toOs(path);
		_file = fopen(osstr.c_str(), m.c_str());

		return !!_file;
	}
	virtual bool close(void) override {
		if (_file) {
			fclose(_file);
			_file = nullptr;

			return true;
		}

		return false;
	}

	virtual size_t peek(void) const override {
		long ft = 0;
		if (_file)
			ft = ftell(_file);

		return ft >= 0 ? (size_t)ft : 0;
	}
	virtual bool poke(size_t pos) override {
		if (!_file)
			return false;

		return fseek(_file, (long)pos, SEEK_SET) == 0;
	}
	virtual size_t count(void) const override {
		if (!_file)
			return 0;

		const long curPos = ftell(_file);
		fseek(_file, 0L, SEEK_END);
		const long len = ftell(_file);
		fseek(_file, curPos, SEEK_SET);

		return len >= 0 ? (size_t)len : 0;
	}
	virtual bool empty(void) const override {
		return count() == 0;
	}
	virtual bool endOfStream(void) const override {
		return peek() >= count();
	}

	virtual Byte readByte(void) override {
		if (!_file)
			return 0;

		return (Byte)fgetc(_file);
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
		if (!_file)
			return 0;

		if (!buf)
			return 0;

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			fread(buf, sizeof(Byte), len, _file);
		}
		if (len < expSize)
			memset(&buf[len], 0, expSize - len);

		return len;
	}
	virtual size_t readBytes(class Bytes* buf, size_t expSize) override {
		if (!buf)
			return 0;

		buf->clear();

		if (!_file)
			return 0;

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			buf->resize(len);
			fread(buf->pointer(), sizeof(Byte), len, _file);
		}

		return len;
	}
	virtual size_t readBytes(class Bytes* buf) override {
		if (!buf)
			return 0;

		buf->clear();

		if (!_file)
			return 0;

		const size_t len = rest();
		if (len > 0) {
			buf->resize(len);
			fread(buf->pointer(), sizeof(Byte), len, _file);
		}

		return len;
	}
	virtual bool readString(char* buf, size_t expSize) override {
		if (!_file)
			return false;

		if (feof(_file))
			return false;

		if (expSize == 0)
			return true;

		if (buf)
			*buf = '\0';

		const size_t len = std::min(rest(), expSize);
		if (len > 0) {
			fread(buf, sizeof(char), len, _file);
		}
		if (len < expSize)
			memset(&buf[len], '\0', expSize - len);

		return true;
	}
	virtual bool readString(std::string &buf) override {
		buf.clear();

		if (!_file)
			return false;

		if (feof(_file))
			return false;

		const size_t len = rest();
		if (len > 0) {
			buf.resize(len);
			fread(&buf.front(), sizeof(char), len, _file);
		}

		return true;
	}
	virtual bool readLine(char** buf, size_t* readSize) override {
#define _STEP 64

		if (buf)
			*buf = nullptr;
		if (readSize)
			*readSize = 0;

		if (!_file)
			return false;

		if (feof(_file))
			return false;

		char* ln = nullptr;
		size_t size = 0;
		size_t capacity = 0;
		while (!feof(_file)) {
			++size;
			if (capacity == 0) {
				capacity = _STEP;
				ln = (char*)realloc(ln, capacity);
			} else if (capacity < size) {
				capacity += _STEP;
				ln = (char*)realloc(ln, capacity);
			}
			ln[size - 1] = (char)fgetc(_file);
			if (ln[size - 1] == '\r') {
				break;
			} else if (ln[size - 1] == '\n') {
				const long len = ftell(_file);
				if (fgetc(_file) != '\r')
					fseek(_file, len, SEEK_SET);

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
		buf.clear();

		if (!_file)
			return false;

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
		if (!_file)
			return 0;

		fputc(val, _file);

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
		if (!_file)
			return 0;

		if (val && len > 0) {
			fwrite(val, sizeof(Byte), len, _file);

			return (int)len;
		}

		return 0;
	}
	virtual int writeBytes(const class Bytes* val, size_t len_) override {
		if (!_file)
			return 0;

		if (val && !val->empty() && len_ > 0) {
			const size_t len = std::min(len_, val->count());
			fwrite(val->pointer(), sizeof(Byte), len, _file);

			return (int)len;
		}

		return 0;
	}
	virtual int writeBytes(const class Bytes* val) override {
		if (!_file)
			return 0;

		if (val && !val->empty()) {
			const size_t len = val->count();
			fwrite(val->pointer(), sizeof(Byte), len, _file);

			return (int)len;
		}

		return 0;
	}
	virtual int writeString(const char* val, size_t len) override {
		if (!_file)
			return 0;

		if (val && len > 0) {
			fwrite(val, sizeof(char), len, _file);

			return (int)len;
		}

		return 0;
	}
	virtual int writeString(const char* val) override {
		if (!_file)
			return 0;

		int result = 0;
		if (val) {
			if (fputs(val, _file) != EOF)
				result += (int)strlen(val);
		}

		return result;
	}
	virtual int writeString(const std::string &val) override {
		if (!_file)
			return 0;

		if (!val.empty()) {
			const size_t len = val.length();
			fwrite(val.c_str(), sizeof(char), len, _file);

			return (int)len;
		}

		return 0;
	}
	virtual int writeLine(const char* val, size_t len) override {
		if (!_file)
			return 0;

		int result = 0;
		if (val && len > 0) {
			fwrite(val, sizeof(char), len, _file);
			result += (int)len;
		}
		fputc('\n', _file);
		++result;

		return result;
	}
	virtual int writeLine(const char* val) override {
		if (!_file)
			return 0;

		int result = 0;
		if (val) {
			if (fputs(val, _file) != EOF)
				result += (int)strlen(val);
		}
		fputc('\n', _file);
		++result;

		return result;
	}
	virtual int writeLine(const std::string &val) override {
		if (!_file)
			return 0;

		int result = 0;
		if (!val.empty()) {
			const size_t len = val.length();
			fwrite(val.c_str(), sizeof(char), len, _file);
			result += (int)len;
		}
		fputc('\n', _file);
		++result;

		return result;
	}
	virtual int writeLine(void) override {
		if (!_file)
			return 0;

		fputc('\n', _file);

		return 1;
	}

private:
	size_t rest(void) const {
		const size_t pos = peek();
		const size_t len = count();
		if (pos >= len)
			return 0;

		return len - pos;
	}

	template<typename T> T read(void) {
		if (!_file)
			return 0;

		T ret = 0;
		fread(&ret, sizeof(T), 1, _file);

		return ret;
	}
	template<typename T> int write(T val) {
		if (!_file)
			return 0;

		if (!fwrite(&val, sizeof(T), 1, _file))
			return 0;

		return (int)sizeof(T);
	}
};

File* File::create(void) {
	FileImpl* result = new FileImpl();

	return result;
}

void File::destroy(File* ptr) {
	FileImpl* impl = static_cast<FileImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
