/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive_txt.h"
#include "bytes.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ARCHIVE_DATA_MEDIA_HEAD
#	define ARCHIVE_DATA_MEDIA_HEAD "data"
#endif /* ARCHIVE_DATA_MEDIA_HEAD */

#ifndef ARCHIVE_PALETTE_MEDIA_TYPE
#	define ARCHIVE_PALETTE_MEDIA_TYPE "model/pal"
#endif /* ARCHIVE_PALETTE_MEDIA_TYPE */
#ifndef ARCHIVE_IMAGE_MEDIA_TYPE
#	define ARCHIVE_IMAGE_MEDIA_TYPE "image/" BITTY_IMAGE_EXT
#endif /* ARCHIVE_IMAGE_MEDIA_TYPE */
#ifndef ARCHIVE_PNG_MEDIA_TYPE
#	define ARCHIVE_PNG_MEDIA_TYPE "image/png"
#endif /* ARCHIVE_PNG_MEDIA_TYPE */
#ifndef ARCHIVE_JPG_MEDIA_TYPE
#	define ARCHIVE_JPG_MEDIA_TYPE "image/jpg"
#endif /* ARCHIVE_JPG_MEDIA_TYPE */
#ifndef ARCHIVE_BMP_MEDIA_TYPE
#	define ARCHIVE_BMP_MEDIA_TYPE "image/bmp"
#endif /* ARCHIVE_BMP_MEDIA_TYPE */
#ifndef ARCHIVE_TGA_MEDIA_TYPE
#	define ARCHIVE_TGA_MEDIA_TYPE "image/tga"
#endif /* ARCHIVE_TGA_MEDIA_TYPE */
#ifndef ARCHIVE_SPRITE_MEDIA_TYPE
#	define ARCHIVE_SPRITE_MEDIA_TYPE "model/spr"
#endif /* ARCHIVE_SPRITE_MEDIA_TYPE */
#ifndef ARCHIVE_MAP_MEDIA_TYPE
#	define ARCHIVE_MAP_MEDIA_TYPE "model/map"
#endif /* ARCHIVE_MAP_MEDIA_TYPE */
#ifndef ARCHIVE_MP3_MEDIA_TYPE
#	define ARCHIVE_MP3_MEDIA_TYPE "audio/mp3"
#endif /* ARCHIVE_MP3_MEDIA_TYPE */
#ifndef ARCHIVE_OGG_MEDIA_TYPE
#	define ARCHIVE_OGG_MEDIA_TYPE "audio/ogg"
#endif /* ARCHIVE_OGG_MEDIA_TYPE */
#ifndef ARCHIVE_WAV_MEDIA_TYPE
#	define ARCHIVE_WAV_MEDIA_TYPE "audio/wav"
#endif /* ARCHIVE_WAV_MEDIA_TYPE */
#ifndef ARCHIVE_MID_MEDIA_TYPE
#	define ARCHIVE_MID_MEDIA_TYPE "audio/mid"
#endif /* ARCHIVE_MID_MEDIA_TYPE */
#ifndef ARCHIVE_AIFF_MEDIA_TYPE
#	define ARCHIVE_AIFF_MEDIA_TYPE "audio/aiff"
#endif /* ARCHIVE_AIFF_MEDIA_TYPE */
#ifndef ARCHIVE_VOC_MEDIA_TYPE
#	define ARCHIVE_VOC_MEDIA_TYPE "audio/voc"
#endif /* ARCHIVE_VOC_MEDIA_TYPE */
#ifndef ARCHIVE_MOD_MEDIA_TYPE
#	define ARCHIVE_MOD_MEDIA_TYPE "audio/mod"
#endif /* ARCHIVE_MOD_MEDIA_TYPE */
#ifndef ARCHIVE_XM_MEDIA_TYPE
#	define ARCHIVE_XM_MEDIA_TYPE "audio/xm"
#endif /* ARCHIVE_XM_MEDIA_TYPE */
#ifndef ARCHIVE_S3M_MEDIA_TYPE
#	define ARCHIVE_S3M_MEDIA_TYPE "audio/s3m"
#endif /* ARCHIVE_S3M_MEDIA_TYPE */
#ifndef ARCHIVE_669_MEDIA_TYPE
#	define ARCHIVE_669_MEDIA_TYPE "audio/669"
#endif /* ARCHIVE_669_MEDIA_TYPE */
#ifndef ARCHIVE_IT_MEDIA_TYPE
#	define ARCHIVE_IT_MEDIA_TYPE "audio/it"
#endif /* ARCHIVE_IT_MEDIA_TYPE */
#ifndef ARCHIVE_MED_MEDIA_TYPE
#	define ARCHIVE_MED_MEDIA_TYPE "audio/med"
#endif /* ARCHIVE_MED_MEDIA_TYPE */
#ifndef ARCHIVE_OPUS_MEDIA_TYPE
#	define ARCHIVE_OPUS_MEDIA_TYPE "audio/opus"
#endif /* ARCHIVE_OPUS_MEDIA_TYPE */
#ifndef ARCHIVE_FLAC_MEDIA_TYPE
#	define ARCHIVE_FLAC_MEDIA_TYPE "audio/flac"
#endif /* ARCHIVE_FLAC_MEDIA_TYPE */
#ifndef ARCHIVE_LUA_MEDIA_TYPE
#	define ARCHIVE_LUA_MEDIA_TYPE "text/lua"
#endif /* ARCHIVE_LUA_MEDIA_TYPE */
#ifndef ARCHIVE_JSON_MEDIA_TYPE
#	define ARCHIVE_JSON_MEDIA_TYPE "text/json"
#endif /* ARCHIVE_JSON_MEDIA_TYPE */
#ifndef ARCHIVE_TEXT_MEDIA_TYPE
#	define ARCHIVE_TEXT_MEDIA_TYPE "text/txt"
#endif /* ARCHIVE_TEXT_MEDIA_TYPE */
#ifndef ARCHIVE_BINARY_MEDIA_TYPE
#	define ARCHIVE_BINARY_MEDIA_TYPE "binary/octet"
#endif /* ARCHIVE_BINARY_MEDIA_TYPE */

#ifndef ARCHIVE_PATH_MEDIA_ATTRIBUTE
#	define ARCHIVE_PATH_MEDIA_ATTRIBUTE "path"
#endif /* ARCHIVE_PATH_MEDIA_ATTRIBUTE */
#ifndef ARCHIVE_COUNT_MEDIA_ATTRIBUTE
#	define ARCHIVE_COUNT_MEDIA_ATTRIBUTE "count"
#endif /* ARCHIVE_COUNT_MEDIA_ATTRIBUTE */

#ifndef ARCHIVE_BASE64_MEDIA_ENCODING
#	define ARCHIVE_BASE64_MEDIA_ENCODING "base64"
#endif /* ARCHIVE_BASE64_MEDIA_ENCODING */

#ifndef ARCHIVE_DATA_MEDIA_END
#	define ARCHIVE_DATA_MEDIA_END "end"
#endif /* ARCHIVE_DATA_MEDIA_END */

/* ===========================================================================} */

/*
** {===========================================================================
** Text-based archive
*/

class ArchiveImplTxt : public Archive {
private:
	struct Entry {
		typedef std::list<Entry> List;

		const char* type;
		std::string path;
		size_t count = 0;
		size_t size = 0;
		std::string encoding;

		size_t begin = npos;
		size_t body = npos;
		size_t end = npos;

		const static decltype(std::string::npos) npos;
	};

private:
	Stream::Accesses _accessibility = Stream::READ_WRITE;
	bool _forWriting = true;

	std::string _file;

	Entry::List _entries;

public:
	ArchiveImplTxt() {
	}
	virtual ~ArchiveImplTxt() override {
		close();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool open(const char* path, Stream::Accesses access) override {
		close();

		if (!path)
			return false;

		_accessibility = access;
		_forWriting = access == Stream::WRITE || access == Stream::APPEND || access == Stream::READ_WRITE;
		_file = path;

		bool opened = true;
		File* file = File::create();
		if (_forWriting) {
			switch (access) {
			case Stream::WRITE:
				opened = file->open(_file.c_str(), Stream::WRITE);

				break;
			case Stream::APPEND:
				opened = file->open(_file.c_str(), Stream::APPEND);

				break;
			case Stream::READ_WRITE:
				opened = file->open(_file.c_str(), Stream::READ_WRITE);

				break;
			default: // Do nothing.
				break;
			}
		} else {
			opened = file->open(_file.c_str(), Stream::READ);
		}
		if (opened)
			file->close();
		File::destroy(file);

		getEntries(_entries);

		return opened;
	}
	virtual bool close(void) override {
		_accessibility = Stream::READ_WRITE;
		_forWriting = true;

		_file.clear();

		_entries.clear();

		return true;
	}

	virtual Formats format(void) const override {
		return TXT;
	}

	virtual Stream::Accesses accessibility(void) const override {
		return _accessibility;
	}

	virtual const char* password(void) const override {
		return nullptr;
	}
	virtual bool password(const char*) override {
		return false;
	}

	virtual bool all(Text::Array &entries) const override {
		entries.clear();

		if (_forWriting)
			return false;

		for (const Entry &ent : _entries)
			entries.push_back(ent.path);

		return true;
	}

	virtual bool exists(const char* nameInArchive) const override {
		if (_forWriting)
			return false;

		Entry tmp;
		if (!findEntry(tmp, nameInArchive))
			return false;

		return true;
	}
	virtual bool make(const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		Entry entry;
		entry.type = typeOf(nameInArchive);
		entry.path = nameInArchive;
		if (!makeEntry(entry, nullptr)) // Without data.
			return false;

		return true;
	}
	virtual bool removable(void) const override {
		return true;
	}
	virtual bool remove(const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		return removeEntry(nameInArchive);
	}
	virtual bool renamable(void) const override {
		return false;
	}
	virtual bool rename(const char* /* nameInArchive */, const char* /* newNameInArchive */) override {
		if (!_forWriting)
			return false;

		return false;
	}

	virtual bool toBytes(class Bytes* val, const char* nameInArchive) const override {
		if (_forWriting)
			return false;

		if (!val)
			return false;

		val->clear();

		Entry ent;
		if (!findEntry(ent, nameInArchive))
			return false;

		File* file = File::create();
		if (file->open(_file.c_str(), Stream::READ)) {
			file->poke(ent.body);
			file->readBytes(val, ent.count);
			file->readLine();
			if (ent.encoding == ARCHIVE_BASE64_MEDIA_ENCODING) {
				std::string str;
				str.assign((const char*)val->pointer(), val->count());
				val->clear();
				Base64::toBytes(val, str);
			}

			file->close();
		}
		File::destroy(file);

		return true;
	}
	virtual bool fromBytes(const class Bytes* val, const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		if (!val)
			return false;

		Bytes* buf = Bytes::create();
		buf->writeBytes(val->pointer(), val->count());

		Entry ent;
		ent.type = typeOf(nameInArchive);
		ent.path = nameInArchive;
		if (!isTextBased(val->pointer(), val->count())) {
			ent.encoding = ARCHIVE_BASE64_MEDIA_ENCODING;
			std::string tmp;
			Base64::fromBytes(tmp, val);
			buf->clear();
			buf->writeString(tmp);
		}
		ent.count = buf->count();
		ent.size = buf->count();
		buf->writeLine();

		const bool result = makeEntry(ent, buf); // With data.

		Bytes::destroy(buf);

		return result;
	}

	virtual bool toFile(const char* path, const char* nameInArchive) const override {
		if (_forWriting)
			return false;

		if (!path)
			return false;

		bool result = false;

		File* file = File::create();
		Bytes* bytes = Bytes::create();
		{
			if (toBytes(bytes, nameInArchive)) {
				if (file->open(path, Stream::WRITE)) {
					if (!bytes->empty())
						file->writeBytes(bytes);
					file->close();

					result = true;
				}
			}
		}
		Bytes::destroy(bytes);
		File::destroy(file);

		return result;
	}
	virtual bool fromFile(const char* path, const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		if (!path)
			return false;

		bool result = false;

		File* file = File::create();
		Bytes* bytes = Bytes::create();
		{
			if (file->open(path, Stream::READ)) {
				size_t l = file->count();
				if (l > 0)
					file->readBytes(bytes);
				file->close();

				fromBytes(bytes, nameInArchive);

				result = true;
			}
		}
		Bytes::destroy(bytes);
		File::destroy(file);

		return result;
	}

	virtual bool toDirectory(const char* dir) const override {
		if (_forWriting)
			return false;

		if (!dir)
			return false;

		File* file = File::create();
		Bytes* bytes = Bytes::create();
		for (const Entry &ent : _entries) {
			std::string sfile = dir;
			if (sfile.back() != '/' && sfile.back() != '\\')
				sfile += "/";
			sfile += ent.path;

			bytes->clear();
			toBytes(bytes, ent.path.c_str());

			FileInfo::Ptr fileInfo = FileInfo::make(sfile.c_str());
			DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(fileInfo->parentPath().c_str());
			if (!dirInfo->exists())
				Path::touchDirectory(dirInfo->fullPath().c_str());

			file->open(sfile.c_str(), Stream::WRITE);
			if (!bytes->empty())
				file->writeBytes(bytes);
			file->close();
		}
		Bytes::destroy(bytes);
		File::destroy(file);

		return true;
	}
	virtual bool fromDirectory(const char* dir) override {
		if (!_forWriting)
			return false;

		if (!dir)
			return false;

		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(dir);
		if (!dirInfo->exists())
			return false;

		std::function<void(DirectoryInfo::Ptr, const std::string &)> pack;
		pack = [this, &pack] (DirectoryInfo::Ptr dirInfo, const std::string &root) -> void {
			FileInfos::Ptr fileInfos = dirInfo->getFiles("*;*.*", false);
			IEnumerator::Ptr enumerator = fileInfos->enumerate();
			while (enumerator->next()) {
				Variant::Pair pair = enumerator->current();
				Object::Ptr val = (Object::Ptr)pair.second;
				if (!val)
					continue;
				FileInfo::Ptr fileInfo = Object::as<FileInfo::Ptr>(val);
				if (!fileInfo)
					continue;

				std::string filePath = fileInfo->fileName();
				if (!fileInfo->extName().empty()) {
					filePath += ".";
					filePath += fileInfo->extName();
				}
				filePath = Path::combine(root.c_str(), filePath.c_str());
				fromFile(fileInfo->fullPath().c_str(), filePath.c_str());
			}

			DirectoryInfos::Ptr dirInfos = dirInfo->getDirectories(false);
			enumerator = fileInfos->enumerate();
			while (enumerator->next()) {
				Variant::Pair pair = enumerator->current();
				Object::Ptr val = (Object::Ptr)pair.second;
				if (!val)
					continue;
				DirectoryInfo::Ptr subDirInfo = Object::as<DirectoryInfo::Ptr>(val);
				if (!subDirInfo)
					continue;

				const std::string subDir = Path::combine(root.c_str(), subDirInfo->dirName().c_str());
				pack(subDirInfo, subDir);
			}
		};

		pack(dirInfo, "");

		return true;
	}

private:
	bool getEntries(Entry::List &entries) const {
		auto ignoreBlank = [] (File* file) -> void {
			while (!file->endOfStream()) {
				const size_t p = file->peek();
				std::string ln;
				if (!file->readLine(ln) || !Text::trim(ln).empty()) {
					file->poke(p);

					break;
				}
			}
		};
		auto startsWith = [] (std::string &str, const std::string &what) -> bool {
			if (!Text::startsWith(str, what, false))
				return false;

			str = str.substr(what.length());

			return true;
		};

		bool result = false;

		entries.clear();

		File* file = File::create();
		do {
			if (!file->open(_file.c_str(), Stream::READ))
				break;

			std::string ln;
			if (!file->readLine(ln) || ln != ARCHIVE_PACKAGE_MEDIA_HEAD ":" ARCHIVE_ARCHIVE_MEDIA_TYPE ";")
				break;

			ignoreBlank(file);

			while (!file->endOfStream()) {
				Entry entry;

				entry.begin = entry.body = entry.end =
					file->peek();
				bool valid = file->readLine(ln);
				valid &= startsWith(ln, ARCHIVE_DATA_MEDIA_HEAD ":");
				if (!valid) {
					if (ln.empty())
						fprintf(stderr, "Wrong text archive media data.\n");
					else
						fprintf(stderr, "Wrong text archive media data: \"%s\".\n", ln.c_str());

					break;
				}

				const Text::Array parts = Text::split(ln, ";");
				if (parts.size() >= 1) {
					constexpr const char* const TYPES[] = {
						ARCHIVE_PALETTE_MEDIA_TYPE,
						ARCHIVE_IMAGE_MEDIA_TYPE, ARCHIVE_PNG_MEDIA_TYPE, ARCHIVE_JPG_MEDIA_TYPE, ARCHIVE_BMP_MEDIA_TYPE, ARCHIVE_TGA_MEDIA_TYPE,
						ARCHIVE_SPRITE_MEDIA_TYPE,
						ARCHIVE_MAP_MEDIA_TYPE,
						ARCHIVE_MP3_MEDIA_TYPE, ARCHIVE_OGG_MEDIA_TYPE, ARCHIVE_WAV_MEDIA_TYPE,
							ARCHIVE_MID_MEDIA_TYPE, ARCHIVE_AIFF_MEDIA_TYPE, ARCHIVE_VOC_MEDIA_TYPE,
							ARCHIVE_MOD_MEDIA_TYPE, ARCHIVE_XM_MEDIA_TYPE, ARCHIVE_S3M_MEDIA_TYPE, ARCHIVE_669_MEDIA_TYPE, ARCHIVE_IT_MEDIA_TYPE, ARCHIVE_MED_MEDIA_TYPE,
							ARCHIVE_OPUS_MEDIA_TYPE,
							ARCHIVE_FLAC_MEDIA_TYPE,
						ARCHIVE_LUA_MEDIA_TYPE,
						ARCHIVE_TEXT_MEDIA_TYPE, ARCHIVE_JSON_MEDIA_TYPE,
						ARCHIVE_BINARY_MEDIA_TYPE
					};
					const std::string &type = parts[0];
					for (int i = 0; i < BITTY_COUNTOF(TYPES); ++i) {
						if (type == TYPES[i]) {
							entry.type = TYPES[i];

							break;
						}
					}
				}
				for (size_t i = 1; i < parts.size(); ++i) {
					std::string part = Text::trim(parts[i]);
					if (parts.empty())
						continue;

					if (startsWith(part, ARCHIVE_PATH_MEDIA_ATTRIBUTE "=")) {
						entry.path = Text::trim(part);
					} else if (startsWith(part, ARCHIVE_COUNT_MEDIA_ATTRIBUTE "=")) {
						unsigned count = 0;
						part = Text::trim(part);
						Text::fromString(part, count);
						entry.count = (size_t)count;
					} else if (part == ARCHIVE_BASE64_MEDIA_ENCODING) {
						entry.encoding = ARCHIVE_BASE64_MEDIA_ENCODING;
					} else {
						fprintf(stderr, "Ignored unknown archive media attribute: \"%s\".\n", part.c_str());
					}
				}

				entry.body = entry.end =
					file->peek();
				if (entry.count >= 0 && entry.count != Entry::npos) {
					entry.end = entry.body + entry.count;
					entry.size = entry.count;

					file->poke(entry.end);
				} else {
					constexpr const char DATA_END[] = ARCHIVE_DATA_MEDIA_HEAD ":" ARCHIVE_DATA_MEDIA_END ";";
					while (!file->endOfStream()) {
						if (!file->readLine(ln) || ln == DATA_END)
							break;
					}
					const size_t pos = file->peek();
					entry.end = pos - BITTY_COUNTOF(DATA_END) - 1;
					entry.count = entry.end - entry.body;
					entry.size = entry.count + BITTY_COUNTOF(DATA_END);
				}

				entries.push_back(entry);

				ignoreBlank(file);
			}

			file->close();

			result = true;
		} while (false);
		File::destroy(file);

		return result;
	}
	bool findEntry(Entry &entry, const char* path) const {
		for (const Entry &ent : _entries) {
			if (ent.path == path) {
				entry = ent;

				return true;
			}
		}

		return false;
	}
	bool makeEntry(const Entry &entry, const Bytes* val) {
		Entry ent;
		if (findEntry(ent, entry.path.c_str()))
			return false;

		bool result = false;

		ent = entry;

		File* file = File::create();
		if (file->open(_file.c_str(), Stream::APPEND)) {
			const size_t len = file->count();
			if (len)
				file->poke(len);
			else
				file->writeLine(ARCHIVE_PACKAGE_MEDIA_HEAD ":" ARCHIVE_ARCHIVE_MEDIA_TYPE ";");

			ent.begin = file->peek();

			file->writeString(ARCHIVE_DATA_MEDIA_HEAD ":");
			file->writeString(entry.type);
			file->writeString(";");

			if (!entry.encoding.empty()) {
				file->writeString(entry.encoding);
				file->writeString(";");
			}

			file->writeString(ARCHIVE_COUNT_MEDIA_ATTRIBUTE "=");
			file->writeString(Text::toString((int)entry.count));
			file->writeString(";");

			file->writeString(ARCHIVE_PATH_MEDIA_ATTRIBUTE "=");
			file->writeString(entry.path);
			file->writeString(";");

			file->writeLine();

			ent.body = file->peek();
			ent.end = ent.body + ent.count;

			if (val)
				file->writeString((const char*)val->pointer(), val->count());

			file->close();

			result = true;
		}
		File::destroy(file);

		if (result) {
			_entries.push_back(ent);
			_entries.sort(
				[] (const Entry &left, const Entry &right) -> bool {
					return left.path < right.path;
				}
			);
		}

		return result;
	}
	bool removeEntry(const char* path) {
		Entry entry;
		if (!findEntry(entry, path))
			return false;

		bool result = false;

		File* file = File::create();
		Bytes* before = Bytes::create();
		Bytes* after = Bytes::create();
		do {
			if (!file->open(_file.c_str(), Stream::READ))
				break;

			file->readBytes(before, entry.begin);
			assert(entry.size >= entry.count);
			file->poke(entry.end + (entry.size - entry.count));
			file->readLine();
			file->readBytes(after);

			file->close();

			if (!file->open(_file.c_str(), Stream::WRITE))
				break;

			file->writeBytes(before);
			file->writeBytes(after);

			file->close();

			result = true;
		} while (false);
		Bytes::destroy(after);
		Bytes::destroy(before);
		File::destroy(file);

		if (result) {
			_entries.remove_if(
				[&] (const Entry &ent) -> bool {
					return ent.path == entry.path;
				}
			);
		}

		return result;
	}

	static const char* typeOf(const char* path) {
		auto match = [] (const std::string &ext, const std::string &pattern) -> bool {
			if (pattern.empty())
				return false;

			return Text::endsWith(ext, pattern, true) &&
				(ext.length() == pattern.length() ||
					(ext.length() >= pattern.length() + 1 && ext[ext.length() - pattern.length() - 1] == '.')
				);
		};

		if (!path || !*path)
			return ARCHIVE_BINARY_MEDIA_TYPE;

		const std::string &partial = path;
		const size_t pos = Text::lastIndexOf(partial, '.');
		if (pos == std::string::npos)
			return ARCHIVE_BINARY_MEDIA_TYPE;

		const std::string ext = partial.substr(pos + 1);

		if (match(ext, BITTY_PALETTE_EXT))
			return ARCHIVE_PALETTE_MEDIA_TYPE;

		if (match(ext, BITTY_IMAGE_EXT))
			return ARCHIVE_IMAGE_MEDIA_TYPE;
		else if (match(ext, "png"))
			return ARCHIVE_PNG_MEDIA_TYPE;
		else if (match(ext, "jpg"))
			return ARCHIVE_JPG_MEDIA_TYPE;
		else if (match(ext, "bmp"))
			return ARCHIVE_BMP_MEDIA_TYPE;
		else if (match(ext, "tga"))
			return ARCHIVE_TGA_MEDIA_TYPE;

		if (match(ext, BITTY_SPRITE_EXT))
			return ARCHIVE_SPRITE_MEDIA_TYPE;

		if (match(ext, BITTY_MAP_EXT))
			return ARCHIVE_MAP_MEDIA_TYPE;

		if (match(ext, "mp3"))
			return ARCHIVE_MP3_MEDIA_TYPE;
		else if (match(ext, "ogg"))
			return ARCHIVE_OGG_MEDIA_TYPE;
		else if (match(ext, "wav"))
			return ARCHIVE_WAV_MEDIA_TYPE;
		else if (match(ext, "mid"))
			return ARCHIVE_MID_MEDIA_TYPE;
		else if (match(ext, "aiff"))
			return ARCHIVE_AIFF_MEDIA_TYPE;
		else if (match(ext, "voc"))
			return ARCHIVE_VOC_MEDIA_TYPE;
		else if (match(ext, "mod"))
			return ARCHIVE_MOD_MEDIA_TYPE;
		else if (match(ext, "xm"))
			return ARCHIVE_XM_MEDIA_TYPE;
		else if (match(ext, "s3m"))
			return ARCHIVE_S3M_MEDIA_TYPE;
		else if (match(ext, "669"))
			return ARCHIVE_669_MEDIA_TYPE;
		else if (match(ext, "it"))
			return ARCHIVE_IT_MEDIA_TYPE;
		else if (match(ext, "med"))
			return ARCHIVE_MED_MEDIA_TYPE;
		else if (match(ext, "opus"))
			return ARCHIVE_OPUS_MEDIA_TYPE;
		else if (match(ext, "flac"))
			return ARCHIVE_FLAC_MEDIA_TYPE;

		if (match(ext, BITTY_LUA_EXT))
			return ARCHIVE_LUA_MEDIA_TYPE;

		if (match(ext, BITTY_JSON_EXT))
			return ARCHIVE_JSON_MEDIA_TYPE;

		if (match(ext, BITTY_TEXT_EXT))
			return ARCHIVE_TEXT_MEDIA_TYPE;

		return ARCHIVE_BINARY_MEDIA_TYPE;
	}
	static bool isTextBased(const Byte* buf, size_t len) {
		if (!buf)
			return false;
		if (len == 0)
			return true;

		const char* str = (const char*)buf;
		while (str < (const char*)buf + len) {
			int n = Unicode::expectUtf8((const char*)str);
			if (!n)
				return false;
			if (n == 1 && !(isprint(*str) || isspace(*str) || *str == '\r' || *str == '\n'))
				return false;

			str += n;
		}

		return true;
	}
};

const size_t ArchiveImplTxt::Entry::npos = std::string::npos;

Archive* archive_create_txt(void) {
	ArchiveImplTxt* result = new ArchiveImplTxt();

	return result;
}

void archive_destroy_txt(Archive* ptr) {
	ArchiveImplTxt* impl = static_cast<ArchiveImplTxt*>(ptr);
	delete impl;
}

/* ===========================================================================} */
