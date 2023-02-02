/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive_zip.h"
#include "bytes.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "../lib/zlib/zlib.h"
#include "../lib/zlib/contrib/minizip/unzip.h"
#include "../lib/zlib/contrib/minizip/zip.h"
#if defined BITTY_OS_WIN
#	include <Windows.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef ARCHIVE_UNPACK_BUFFER_SIZE
#	define ARCHIVE_UNPACK_BUFFER_SIZE 512
#endif /* ARCHIVE_UNPACK_BUFFER_SIZE */

/* ===========================================================================} */

/*
** {===========================================================================
** ZIP archive
*/

class ArchiveImplZip : public Archive {
private:
	Stream::Accesses _accessibility = Stream::READ_WRITE;
	bool _forWriting = true;

	zipFile _zipFile = nullptr;
	unzFile _unzipFile = nullptr;

	std::string _file;
	std::string _pwd;
	int _level = 9;

public:
	ArchiveImplZip() {
	}
	virtual ~ArchiveImplZip() override {
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

		std::string osstr = Unicode::toOs(_file.c_str());

		if (_forWriting) {
			switch (access) {
			case Stream::WRITE:
				_zipFile = zipOpen64(osstr.c_str(), APPEND_STATUS_CREATE);

				break;
			case Stream::APPEND:
				_zipFile = zipOpen64(osstr.c_str(), APPEND_STATUS_ADDINZIP);
				if (!_zipFile)
					_zipFile = zipOpen64(osstr.c_str(), APPEND_STATUS_CREATE);

				break;
			case Stream::READ_WRITE: // Do nothing.
				break;
			default: // Do nothing.
				break;
			}

			return !!_zipFile;
		} else {
			_unzipFile = unzOpen64(osstr.c_str());

			return !!_unzipFile;
		}
	}
	virtual bool close(void) override {
		bool result = false;

		_accessibility = Stream::READ_WRITE;
		_forWriting = true;

		if (_forWriting && _zipFile)
			result = true;
		else if (_unzipFile)
			result = true;
		if (_zipFile) {
			zipClose(_zipFile, nullptr);
			_zipFile = nullptr;
		}
		if (_unzipFile) {
			unzClose(_unzipFile);
			_unzipFile = nullptr;
		}

		_file.clear();
		_pwd.clear();
		_level = 9;

		return result;
	}

	virtual Formats format(void) const override {
		return ZIP;
	}

	virtual Stream::Accesses accessibility(void) const override {
		return _accessibility;
	}

	virtual const char* password(void) const override {
		if (_pwd.empty())
			return nullptr;

		return _pwd.c_str();
	}
	virtual bool password(const char* pwd) override {
		_pwd.clear();

		if (pwd)
			_pwd = pwd;

		return true;
	}

	virtual bool all(Text::Array &entries) const override {
		entries.clear();

		if (_forWriting)
			return false;

		std::string osstr = Unicode::toOs(_file.c_str());

		unzFile unzf = unzOpen64(osstr.c_str());
		if (!unzf)
			return false;

		unz_global_info64 gi;

		int ret = unzGetGlobalInfo64(unzf, &gi);
		if (ret != UNZ_OK) {
			unzClose(unzf);

			return false;
		}

		for (int i = 0; i < gi.number_entry; ++i) {
			unz_file_info64 unzFileInfo;
			char fn[BITTY_MAX_PATH];
			memset(fn, 0, sizeof(fn));

			if (unzGetCurrentFileInfo64(unzf, &unzFileInfo, fn, sizeof(fn), nullptr, 0, nullptr, 0) != UNZ_OK)
				continue;

			entries.push_back(fn);

			if (i < gi.number_entry - 1 && unzGoToNextFile(unzf) != UNZ_OK)
				continue;
		}

		unzClose(unzf);

		return true;
	}

	virtual bool exists(const char* nameInArchive) const override {
		if (_forWriting)
			return false;

		if (nameInArchive) {
			if (unzLocateFile(_unzipFile, nameInArchive, 1) != UNZ_OK)
				return false;
		}

		return true;
	}
	virtual bool make(const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		zip_fileinfo zipFileInfo;
		memset(&zipFileInfo, 0, sizeof(zip_fileinfo));

		zipOpenNewFileInZip4(
			_zipFile, nameInArchive, &zipFileInfo,
			nullptr, 0, nullptr, 0,
			nullptr,
			Z_DEFLATED, _level, 0, -MAX_WBITS, DEF_MEM_LEVEL,
			Z_DEFAULT_STRATEGY, password(), 0,
			// Encode file name with UTF8.
			// See: https://stackoverflow.com/questions/14625784/how-to-convert-minizip-wrapper-to-unicode.
			36, 1 << 11
		);

		zipCloseFileInZip(_zipFile);

		return true;
	}
	virtual bool removable(void) const override {
		return false;
	}
	virtual bool remove(const char*) override {
		return false;
	}
	virtual bool renamable(void) const override {
		return false;
	}
	virtual bool rename(const char*, const char*) override {
		return false;
	}

	virtual bool toBytes(class Bytes* val, const char* nameInArchive) const override {
		if (_forWriting)
			return false;

		if (!val)
			return false;

		val->clear();

		int ret = 0;
		unz_file_info64 unzFileInfo;
		char file[BITTY_MAX_PATH];
		memset(file, 0, sizeof(file));

		if (nameInArchive) {
			if (unzLocateFile(_unzipFile, nameInArchive, 1) != UNZ_OK)
				return false;
		}

		if (unzGetCurrentFileInfo64(_unzipFile, &unzFileInfo, file, sizeof(file), nullptr, 0, nullptr, 0) != UNZ_OK)
			return false;

#if defined BITTY_OS_WIN
		if (!(unzFileInfo.external_fa & FILE_ATTRIBUTE_DIRECTORY))
			ret = unzOpenCurrentFile(_unzipFile);
#else /* BITTY_OS_WIN */
		ret = unzOpenCurrentFile(_unzipFile);
#endif /* BITTY_OS_WIN */
		ret = unzOpenCurrentFilePassword(_unzipFile, password());

		int size = 0;
		char buf[ARCHIVE_UNPACK_BUFFER_SIZE];
		do {
			memset(buf, 0, sizeof(buf));
			size = unzReadCurrentFile(_unzipFile, buf, sizeof(buf));
			val->writeBytes((const Byte*)buf, size);
		} while (size > 0);

		unzCloseCurrentFile(_unzipFile);

		return true;
	}
	virtual bool fromBytes(const class Bytes* val, const char* nameInArchive) override {
		if (!_forWriting)
			return false;

		if (!val)
			return false;

		zip_fileinfo zipFileInfo;
		memset(&zipFileInfo, 0, sizeof(zip_fileinfo));

		zipOpenNewFileInZip4(
			_zipFile, nameInArchive, &zipFileInfo,
			nullptr, 0, nullptr, 0,
			nullptr,
			Z_DEFLATED, _level, 0, -MAX_WBITS, DEF_MEM_LEVEL,
			Z_DEFAULT_STRATEGY, password(), 0,
			// Encode file name with UTF8.
			// See: https://stackoverflow.com/questions/14625784/how-to-convert-minizip-wrapper-to-unicode.
			36, 1 << 11
		);

		if (!val->empty())
			zipWriteInFileInZip(_zipFile, val->pointer(), (unsigned)val->count());

		zipCloseFileInZip(_zipFile);

		return true;
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

		unz_global_info64 gi;

		int ret = unzGetGlobalInfo64(_unzipFile, &gi);
		if (ret != UNZ_OK)
			return false;

		File* file = File::create();
		Bytes* bytes = Bytes::create();
		for (int i = 0; i < gi.number_entry; ++i) {
			unz_file_info64 unzFileInfo;
			char fn[BITTY_MAX_PATH];
			memset(fn, 0, sizeof(fn));

			if (unzGetCurrentFileInfo64(_unzipFile, &unzFileInfo, fn, sizeof(fn), nullptr, 0, nullptr, 0) != UNZ_OK)
				continue;

			std::string sfile = dir;
			if (sfile.back() != '/' && sfile.back() != '\\')
				sfile += "/";
			sfile += fn;

			bytes->clear();
			toBytes(bytes, fn);

			FileInfo::Ptr fileInfo = FileInfo::make(sfile.c_str());
			DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(fileInfo->parentPath().c_str());
			if (!dirInfo->exists())
				Path::touchDirectory(dirInfo->fullPath().c_str());

			file->open(sfile.c_str(), Stream::WRITE);
			if (!bytes->empty())
				file->writeBytes(bytes);
			file->close();

			if (i < gi.number_entry - 1 && unzGoToNextFile(_unzipFile) != UNZ_OK)
				continue;
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
};

Archive* archive_create_zip(void) {
	ArchiveImplZip* result = new ArchiveImplZip();

	return result;
}

void archive_destroy_zip(Archive* ptr) {
	ArchiveImplZip* impl = static_cast<ArchiveImplZip*>(ptr);
	delete impl;
}

/* ===========================================================================} */
