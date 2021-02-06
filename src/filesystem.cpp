/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "encoding.h"
#include "filesystem.h"
#include "platform.h"
#include "text.h"
#if defined BITTY_OS_WIN
#	include "../lib/dirent/include/dirent.h"
#else /* BITTY_OS_WIN */
#	include <dirent.h>
#	include <sys/stat.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Utilities
*/

static bool filesystemTextMatchWildcard(const std::wstring &str, const wchar_t* wildcard) {
	const wchar_t* string = str.c_str();

	while (*wildcard && *wildcard != '*' && *wildcard != '?')
		if (*string != *wildcard) {
			return false;
		} else {
			++string;
			++wildcard;
		}

	if (!(*string)) {
		for ( ; *wildcard; ++wildcard) {
			if (*wildcard != '*' && *wildcard != '?')
				return false;
		}

		return true;
	}

	switch (*wildcard) {
	case '\0':
		return false;
	case '*':
		while (*wildcard == '*' || *wildcard == '?')
			++wildcard;

		if (!(*wildcard))
			return true;

		for ( ; *string; ++string) {
			if (filesystemTextMatchWildcard(string, wildcard))
				return true;
		}

		return false;
	case '?':
		return filesystemTextMatchWildcard(string + 1, wildcard + 1) || filesystemTextMatchWildcard(string, wildcard + 1);
	default:
		assert(false);

		return false;
	}
}

/* ===========================================================================} */

/*
** {===========================================================================
** Path
*/

std::string Path::executableFile(void) {
	std::string utfstr = Unicode::fromOs(Platform::executableFile());
	uniform(utfstr);

	return utfstr;
}

std::string Path::documentDirectory(void) {
	std::string utfstr = Unicode::fromOs(Platform::documentDirectory());
	uniform(utfstr);

	return utfstr;
}

std::string Path::writableDirectory(void) {
	std::string utfstr = Unicode::fromOs(Platform::writableDirectory());
	uniform(utfstr);

	return utfstr;
}

std::string Path::currentDirectory(void) {
	std::string utfstr = Unicode::fromOs(Platform::currentDirectory());
	uniform(utfstr);

	return utfstr;
}

void Path::currentDirectory(const char* dir) {
	if (!dir)
		return;

	const std::string osstr = Unicode::toOs(dir);
	Platform::currentDirectory(osstr.c_str());
}

void Path::uniform(std::string &path) {
	std::wstring wstr = Unicode::toWide(path);

	std::transform(
		wstr.begin(), wstr.end(), wstr.begin(),
		[] (wchar_t ch) -> wchar_t {
			if (ch == '\\')
				return '/';
			else
				return ch;
		}
	);

	path = Unicode::fromWide(wstr);
}

void Path::diversify(std::string &path) {
#if defined BITTY_OS_WIN
	std::wstring wstr = Unicode::toWide(path);

	std::transform(
		wstr.begin(), wstr.end(), wstr.begin(),
		[] (wchar_t ch) -> wchar_t {
			if (ch == '/')
				return '\\';
			else
				return ch;
		}
	);

	path = Unicode::fromWide(wstr);
#else /* BITTY_OS_WIN */
	std::wstring wstr = Unicode::toWide(path);

	std::transform(
		wstr.begin(), wstr.end(), wstr.begin(),
		[] (wchar_t ch) -> wchar_t {
			if (ch == '\\')
				return '/';
			else
				return ch;
		}
	);

	path = Unicode::fromWide(wstr);
#endif /* BITTY_OS_WIN */
}

bool Path::isValid(const char* path) {
	if (!path || !(*path))
		return false;

	while (*path) {
		const int n = Unicode::expectUtf8(path);
		const unsigned utf8 = Unicode::takeUtf8(path, n);
		if (
			utf8 <= 255 && !(isalnum(utf8) || isspace(utf8) ||
				utf8 == '_' || utf8 == '-' || utf8 == '~' || utf8 == '!' || utf8 == '@' || utf8 == '#' || utf8 == '$' || utf8 == '=' || utf8 == '+' ||
				utf8 == '(' || utf8 == ')' || utf8 == '[' || utf8 == ']' || utf8 == '{' || utf8 == '}' || utf8 == ',' || utf8 == '.' || utf8 == '/'
			)
		) {
			return false;
		}

		path += n;
	}

	return true;
}

bool Path::equals(const char* lpath, const char* rpath) {
	if (!lpath || !rpath)
		return false;

	std::string strl = lpath;
	std::string strr = rpath;

	if (strl.empty() || strr.empty())
		return strl == strr;

	uniform(strl);
	uniform(strr);

	std::string osl = Unicode::toOs(strl);
	std::string osr = Unicode::toOs(strr);

	if (osl.back() == '\\' || osl.back() == '/')
		osl.pop_back();
	if (osr.back() == '\\' || osr.back() == '/')
		osr.pop_back();

	return Platform::equal(osl.c_str(), osr.c_str());
}

bool Path::isParentOf(const char* lpath, const char* rpath) {
	if (!lpath || !rpath)
		return false;

	std::string strl = lpath;
	std::string strr = rpath;

	if (strl.empty() || strr.empty())
		return false;

	uniform(strl);
	uniform(strr);

	std::string osl = Unicode::toOs(strl);
	std::string osr = Unicode::toOs(strr);

	if (osl.back() == '\\' || osl.back() == '/')
		osl.pop_back();
	if (osr.back() == '\\' || osr.back() == '/')
		osr.pop_back();

	return Platform::isParentOf(osl.c_str(), osr.c_str());
}

std::string Path::absoluteOf(const std::string &path) {
	const std::string ospath = Unicode::toOs(path);
	std::string utfstr = Unicode::fromOs(Platform::absoluteOf(ospath));
	uniform(utfstr);

	Text::Array parts = Text::split(utfstr, "/");
	utfstr.clear();
	for (int i = 0; i < (int)parts.size(); ++i) {
		if (i == (int)parts.size() - 1) {
			utfstr += parts[i];
		} else if (parts[i + 1] != "..") {
			utfstr += parts[i];
			utfstr += "/";
		} else /* if (parts[i + 1] == "..") */ {
			++i;
		}
	}
	if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
		utfstr.push_back('/');

	return utfstr;
}

std::string Path::combine(const char* part0, const char* part1) {
	const std::string p0 = part0 ? part0 : "";
	const std::string p1 = part1 ? part1 : "";
	if (
		(!p0.empty() && p0[p0.length() - 1] != '/' && p0[p0.length() - 1] != '\\') &&
		(!p1.empty() && p1[0] != '/' && p1[0] != '\\')
	) {
		return p0 + "/" + p1;
	}

	return p0 + p1;
}

void Path::split(const std::string &full, std::string* self, std::string* ext, std::string* parent) {
	if (!parent && !self && !ext)
		return;

	std::string path = full;
	std::string selfExt;
	size_t pos = std::string::npos;

	if (parent)
		parent->clear();
	if (self)
		self->clear();
	if (ext)
		ext->clear();

	uniform(path);
	pos = path.find_last_of('/');
	if (pos == path.size() - 1 && pos > 0) {
		pos = path.find_last_of('/', pos - 1);
	}

	if (pos == std::string::npos) {
		selfExt = full;
	} else {
		selfExt = path.substr(pos + 1, path.size() - pos - 1);
		if (parent)
			*parent = path.substr(0, pos + 1);
	}

	pos = selfExt.find_last_of(".");
	if (pos == std::string::npos) {
		if (self)
			*self = selfExt;
	} else {
		if (ext)
			*ext = selfExt.substr(pos + 1);
		if (self)
			*self = selfExt.substr(0, pos);
	}
}

bool Path::existsFile(const char* path) {
	if (!path || !(*path))
		return false;

	const std::string osstr = Unicode::toOs(path);

	struct stat buf;

	return (stat(osstr.c_str(), &buf) == 0) && ((buf.st_mode & S_IFDIR) == 0);
}

bool Path::existsDirectory(const char* path) {
	if (!path || !(*path))
		return false;

	const std::string osstr = Unicode::toOs(path);
	if (osstr.length() > BITTY_MAX_PATH)
		return false;

	DIR* dir = opendir(osstr.c_str());
	bool result = !!dir;
	if (dir)
		closedir(dir);

	return result;
}

bool Path::copyFile(const char* src, const char* dst) {
	if ((!src || !(*src)) || (!dst || !(*dst)))
		return false;

	const std::string ossrc = Unicode::toOs(src);
	const std::string osdst = Unicode::toOs(dst);

	return Platform::copyFile(ossrc.c_str(), osdst.c_str());
}

bool Path::copyDirectory(const char* src, const char* dst) {
	if ((!src || !(*src)) || (!dst || !(*dst)))
		return false;

	const std::string ossrc = Unicode::toOs(src);
	const std::string osdst = Unicode::toOs(dst);

	return Platform::copyDirectory(ossrc.c_str(), osdst.c_str());
}

bool Path::moveFile(const char* src, const char* dst) {
	if ((!src || !(*src)) || (!dst || !(*dst)))
		return false;

	std::string ossrc = Unicode::toOs(src);
	const std::string osdst = Unicode::toOs(dst);

	if (!Platform::copyFile(ossrc.c_str(), osdst.c_str()))
		return false;

	ossrc.push_back('\0'); // Windows shell API requires double zero termination.

	return Platform::removeFile(ossrc.c_str(), false);
}

bool Path::moveDirectory(const char* src, const char* dst) {
	if ((!src || !(*src)) || (!dst || !(*dst)))
		return false;

	std::string ossrc = Unicode::toOs(src);
	const std::string osdst = Unicode::toOs(dst);

	const std::string fullsrc = Platform::absoluteOf(ossrc);
	const std::string fulldst = Platform::absoluteOf(osdst);
	if (Platform::isParentOf(fullsrc.c_str(), fulldst.c_str()))
		return false;

	if (!Platform::copyDirectory(ossrc.c_str(), osdst.c_str()))
		return false;

	ossrc.push_back('\0'); // Windows shell API requires double zero termination.

	return Platform::removeDirectory(ossrc.c_str(), false);
}

bool Path::removeFile(const char* path, bool toTrashBin) {
	if (!path || !(*path))
		return false;

	std::string osstr = Unicode::toOs(path);

	osstr.push_back('\0'); // Windows shell API requires double zero termination.

	return Platform::removeFile(osstr.c_str(), toTrashBin);
}

bool Path::removeDirectory(const char* path, bool toTrashBin) {
	if (!path || !(*path))
		return false;

	std::string osstr = Unicode::toOs(path);

	Platform::accreditDirectory(osstr.c_str());

	osstr.push_back('\0'); // Windows shell API requires double zero termination.

	return Platform::removeDirectory(osstr.c_str(), toTrashBin);
}

bool Path::touchFile(const char* path) {
	if (!path || !(*path))
		return false;

	FileInfo::Ptr fileInfo = FileInfo::make(path);
	if (!fileInfo->exists()) {
		if (!touchDirectory(fileInfo->parentPath().c_str()))
			return false;

		if (!fileInfo->make())
			return false;
	}

	return true;
}

bool Path::touchDirectory(const char* path) {
	if (!path || !(*path))
		return false;

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(path);
	if (!dirInfo->exists()) {
		if (!touchDirectory(dirInfo->parentPath().c_str()))
			return false;

		const std::string osstr = Unicode::toOs(path);
		if (!Platform::makeDirectory(osstr.c_str()))
			return false;
	}

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** File info
*/

class FileInfosImpl : public FileInfos {
private:
	typedef std::vector<FileInfo::Ptr> Collection;

	class Enumerator : public IEnumerator {
	private:
		Collection* _collection = nullptr;
		bool _invalidated = false;
		Variant::Int _index = -1;
		Collection::iterator _iterator;

	public:
		Enumerator(Collection &coll) {
			_collection = &coll;
			_iterator = _collection->begin();
		}

		BITTY_CLASS_TYPE('F', 'I', 'S', 'I')

		virtual unsigned type(void) const override {
			return TYPE();
		}

		virtual bool next(void) override {
			if (_invalidated)
				return false;

			if (_index == -1)
				_iterator = _collection->begin();
			else
				++_iterator;
			++_index;

			return _iterator != _collection->end();
		}

		virtual Variant::Pair current(void) override {
			if (_invalidated)
				return Variant::Pair(Variant(), Variant());

			const Variant val = Object::Ptr(*_iterator);

			return Variant::Pair(Variant(_index), val);
		}

		virtual void invalidate(void) override {
			_invalidated = true;
		}
	};

private:
	Collection _files;

public:
	virtual ~FileInfosImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual int count(void) const override {
		return (int)_files.size();
	}

	virtual std::shared_ptr<FileInfo> get(int i) override {
		if (i < 0 || i >= (int)_files.size())
			return nullptr;

		return _files.at(i);
	}

	virtual FileInfos &add(std::shared_ptr<FileInfo> fi) override {
		_files.push_back(fi);

		return *this;
	}

	virtual void clear(void) override {
		_files.clear();
	}

	virtual IEnumerator::Ptr enumerate(void) override {
		return Enumerable::enumerate(new Enumerator(_files));
	}
};

FileInfos::Ptr FileInfos::make(void) {
	return Ptr(create(), destroy);
}

FileInfos* FileInfos::create(void) {
	FileInfosImpl* result = new FileInfosImpl();

	return result;
}

void FileInfos::destroy(FileInfos* ptr) {
	FileInfosImpl* impl = static_cast<FileInfosImpl*>(ptr);
	delete impl;
}

class FileInfoImpl : public FileInfo {
private:
	std::string _fullPath;
	std::string _parent;
	std::string _fileName;
	std::string _extName;

public:
	FileInfoImpl(const char* path) {
		_fullPath = path;
		Path::split(_fullPath, &_fileName, &_extName, &_parent);
	}
	virtual ~FileInfoImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual const std::string &fullPath(void) const override {
		return _fullPath;
	}
	virtual const std::string &parentPath(void) const override {
		return _parent;
	}
	virtual const std::string &fileName(void) const override {
		return _fileName;
	}
	virtual const std::string &extName(void) const override {
		return _extName;
	}

	virtual bool empty(void) const override {
		const std::string osstr = Unicode::toOs(_fullPath.c_str());

		FILE* fp = fopen(osstr.c_str(), "rb");
		if (fp) {
			fseek(fp, 0L, SEEK_END);
			long l = ftell(fp);

			fclose(fp);

			return l == 0;
		} else {
			return true;
		}

		return false;
	}
	virtual bool exists(void) const override {
		return Path::existsFile(_fullPath.c_str());
	}
	virtual bool make(void) override {
		if (exists())
			return true;

		const std::string osstr = Unicode::toOs(_fullPath.c_str());

		FILE* fp = fopen(osstr.c_str(), "wb");
		if (fp)
			fclose(fp);
		else
			return false;

		return true;
	}
	virtual bool copyTo(const char* newPath) override {
		return Path::copyFile(_fullPath.c_str(), newPath);
	}
	virtual bool moveTo(const char* newPath) override {
		if (!Path::moveFile(fullPath().c_str(), newPath))
			return false;

		std::string newName, newExt, newParent;
		Path::split(newPath, &newName, &newExt, &newParent);

		_fullPath = newPath;
		_parent = newParent;
		_fileName = newName;
		_extName = newExt;

		return true;
	}
	virtual bool remove(bool toTrashBin) override {
		if (!Path::removeFile(_fullPath.c_str(), toTrashBin))
			return false;

		return true;
	}
	virtual bool rename(const char* newNameExt) override {
		const std::string newNameExt_ = newNameExt;
		std::string newName, newExt;
		Path::split(newNameExt_, &newName, &newExt, nullptr);

		const std::string oldPath = _fullPath;
		const std::string newPath = Path::combine(_parent.c_str(), newNameExt_.c_str());

		const std::string ossrc = Unicode::toOs(oldPath.c_str());
		const std::string osdst = Unicode::toOs(newPath.c_str());

		if (Platform::moveFile(ossrc.c_str(), osdst.c_str())) {
			_fullPath = Path::combine(_parent.c_str(), newNameExt_.c_str());
			_fileName = newName;
			_extName = newExt;

			return true;
		} else {
			return false;
		}
	}
	virtual bool rename(const char* newName, const char* newExt) override {
		std::string newNameExt = newName;
		newNameExt += ".";
		if (newExt)
			newNameExt += newExt;
		else
			newNameExt += _extName;

		const std::string oldPath = _fullPath;
		const std::string newPath = Path::combine(_parent.c_str(), newNameExt.c_str());

		const std::string ossrc = Unicode::toOs(oldPath.c_str());
		const std::string osdst = Unicode::toOs(newPath.c_str());

		if (Platform::moveFile(ossrc.c_str(), osdst.c_str())) {
			_fullPath = Path::combine(_parent.c_str(), newNameExt.c_str());
			_fileName = newName;
			_extName = newExt;

			return true;
		} else {
			return false;
		}
	}

	virtual std::shared_ptr<DirectoryInfo> parent(void) const override {
		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(_parent.c_str());

		return dirInfo;
	}

	virtual std::string readAll(void) const override {
		if (!exists())
			return "";

		const std::string osstr = Unicode::toOs(_fullPath.c_str());

		FILE* fp = fopen(osstr.c_str(), "rb");
		if (fp) {
			const long curPos = ftell(fp);
			fseek(fp, 0L, SEEK_END);
			const long len = ftell(fp);
			fseek(fp, curPos, SEEK_SET);
			char* buf = new char[len + 1];
			fread(buf, 1, len, fp);
			buf[len] = '\0';
			std::string result = buf;
			delete [] buf;
			fclose(fp);

			return result;
		}

		return "";
	}
};

FileInfo::Ptr FileInfo::make(const char* path) {
	return Ptr(create(path), destroy);
}

FileInfo* FileInfo::create(const char* path) {
	FileInfoImpl* result = new FileInfoImpl(path);

	return result;
}

void FileInfo::destroy(FileInfo* ptr) {
	FileInfoImpl* impl = static_cast<FileInfoImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Directory info
*/

class DirectoryInfosImpl : public DirectoryInfos {
private:
	typedef std::vector<DirectoryInfo::Ptr> Collection;

	class Enumerator : public IEnumerator {
	private:
		Collection* _collection = nullptr;
		bool _invalidated = false;
		Variant::Int _index = -1;
		Collection::iterator _iterator;

	public:
		Enumerator(Collection &coll) {
			_collection = &coll;
			_iterator = _collection->begin();
		}

		BITTY_CLASS_TYPE('D', 'I', 'S', 'I')

		virtual unsigned type(void) const override {
			return TYPE();
		}

		virtual bool next(void) override {
			if (_invalidated)
				return false;

			if (_index == -1)
				_iterator = _collection->begin();
			else
				++_iterator;
			++_index;

			return _iterator != _collection->end();
		}

		virtual Variant::Pair current(void) override {
			if (_invalidated)
				return Variant::Pair(Variant(), Variant());

			const Variant val = Object::Ptr(*_iterator);

			return Variant::Pair(Variant(_index), val);
		}

		virtual void invalidate(void) override {
			_invalidated = true;
		}
	};

private:
	Collection _dirs;

public:
	virtual ~DirectoryInfosImpl() override {
		clear();
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual int count(void) const override {
		return (int)_dirs.size();
	}

	virtual std::shared_ptr<DirectoryInfo> get(int i) override {
		if (i < 0 || i >= (int)_dirs.size())
			return nullptr;

		return _dirs.at(i);
	}

	virtual DirectoryInfos &add(std::shared_ptr<DirectoryInfo> fi) override {
		_dirs.push_back(fi);

		return *this;
	}

	virtual void clear(void) override {
		_dirs.clear();
	}

	virtual IEnumerator::Ptr enumerate(void) override {
		return Enumerable::enumerate(new Enumerator(_dirs));
	}
};

DirectoryInfos::Ptr DirectoryInfos::make(void) {
	return Ptr(create(), destroy);
}

DirectoryInfos* DirectoryInfos::create(void) {
	DirectoryInfosImpl* result = new DirectoryInfosImpl();

	return result;
}

void DirectoryInfos::destroy(DirectoryInfos* ptr) {
	DirectoryInfosImpl* impl = static_cast<DirectoryInfosImpl*>(ptr);
	delete impl;
}

class DirectoryInfoImpl : public DirectoryInfo {
private:
	std::string _fullPath;
	std::string _parent;
	std::string _dirName;

public:
	DirectoryInfoImpl(const char* path) {
		_fullPath = path;
		Path::split(_fullPath, &_dirName, nullptr, &_parent);
	}
	virtual ~DirectoryInfoImpl() override {
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual const std::string &fullPath(void) const override {
		return _fullPath;
	}
	virtual const std::string &parentPath(void) const override {
		return _parent;
	}
	virtual const std::string &dirName(void) const override {
		return _dirName;
	}

	virtual bool empty(void) const override {
		const std::string osstr = Unicode::toOs(_fullPath.c_str());
		if (osstr.length() > BITTY_MAX_PATH)
			return true;

		bool result = true;
		DIR* dir = opendir(osstr.c_str());
		if (dir) {
			struct dirent* ent = readdir(dir);
			while (ent) {
				switch (ent->d_type) {
				case DT_REG:
					result = false;

					break;
				case DT_DIR:
					if (Platform::ignore(ent->d_name))
						break;

					result = false;

					break;
				}
				if (!result)
					break;

				ent = readdir(dir);
			}
			closedir(dir);
		}

		return result;
	}
	virtual bool exists(void) const override {
		return Path::existsDirectory(_fullPath.c_str());
	}
	virtual bool make(void) override {
		if (exists())
			return true;

		const std::string osstr = Unicode::toOs(_fullPath.c_str());

		return Platform::makeDirectory(osstr.c_str());
	}
	virtual bool copyTo(const char* newPath) override {
		return Path::copyDirectory(_fullPath.c_str(), newPath);
	}
	virtual bool moveTo(const char* newPath) override {
		if (!Path::moveDirectory(fullPath().c_str(), newPath))
			return false;

		std::string newDir, newParent;
		Path::split(newPath, &newDir, nullptr, &newParent);

		_fullPath = newPath;
		_parent = newParent;
		_dirName = newDir;

		return true;
	}
	virtual bool remove(bool toTrashBin) override {
		return Path::removeDirectory(_fullPath.c_str(), toTrashBin);
	}
	virtual bool rename(const char* newName) override {
		const std::string newPath = Path::combine(_parent.c_str(), newName);

		const std::string ossrc = Unicode::toOs(_fullPath.c_str());
		const std::string osdst = Unicode::toOs(newPath.c_str());

		if (Platform::moveDirectory(ossrc.c_str(), osdst.c_str())) {
			_fullPath = newPath;
			_dirName = newName;

			return true;
		} else {
			return false;
		}
	}

	virtual FileInfos::Ptr getFiles(const char* pattern, bool recursive, bool ignoreDots) const override {
		FileInfos::Ptr fileInfos = FileInfos::make();
		Text::Array patterns;
		if (pattern && *pattern)
			patterns = Text::split(pattern, ";");
		getFiles(fileInfos, _fullPath.c_str(), patterns, recursive, ignoreDots);

		return fileInfos;
	}
	virtual FileInfos::Ptr getFiles(const char* pattern, bool recursive) const override {
		return getFiles(pattern, recursive, false);
	}
	virtual DirectoryInfos::Ptr getDirectories(bool recursive, bool ignoreDots) const override {
		DirectoryInfos::Ptr dirInfos = DirectoryInfos::make();
		getDirectories(dirInfos, _fullPath.c_str(), recursive, ignoreDots);

		return dirInfos;
	}
	virtual DirectoryInfos::Ptr getDirectories(bool recursive) const override {
		return getDirectories(recursive, false);
	}

	virtual std::shared_ptr<DirectoryInfo> parent(void) const override {
		DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(_parent.c_str());

		return dirInfo;
	}

private:
	static void getFiles(FileInfos::Ptr coll, const char* path, const Text::Array &patterns, bool recursive, bool ignoreDots) {
		const std::string osstr = Unicode::toOs(path);
		if (osstr.length() > BITTY_MAX_PATH)
			return;

		DIR* dir = opendir(osstr.c_str());
		if (dir) {
			struct dirent* ent = readdir(dir);
			while (ent) {
				switch (ent->d_type) {
				case DT_REG: {
						if (Platform::ignore(ent->d_name))
							break;

						if (ignoreDots && *ent->d_name == '.')
							break;

						bool matched = false;
						if (patterns.empty()) {
							matched = true;
						} else {
							for (const std::string &pattern : patterns) {
								const std::string name = Unicode::fromOs(ent->d_name);
								const std::wstring wname = Unicode::toWide(name.c_str());
								const std::wstring wpattern = Unicode::toWide(pattern.c_str());

								if (filesystemTextMatchWildcard(wname, wpattern.c_str())) {
									matched = true;

									break;
								}
							}
						}

						if (matched) {
							const std::string name = Unicode::fromOs(ent->d_name);

							const std::string p = Path::combine(path, name.c_str());
							FileInfo::Ptr fileInfo = FileInfo::make(p.c_str());
							coll->add(fileInfo);
						}
					}

					break;
				case DT_DIR: {
						if (Platform::ignore(ent->d_name))
							break;

						if (ignoreDots && *ent->d_name == '.')
							break;

						if (recursive) {
							const std::string name = Unicode::fromOs(ent->d_name);

							const std::string p = Path::combine(path, name.c_str());
							getFiles(coll, p.c_str(), patterns, recursive, ignoreDots);
						}
					}

					break;
				}

				ent = readdir(dir);
			}
			closedir(dir);
		}
	}

	static void getDirectories(DirectoryInfos::Ptr coll, const char* path, bool recursive, bool ignoreDots) {
		const std::string osstr = Unicode::toOs(path);
		if (osstr.length() > BITTY_MAX_PATH)
			return;

		DIR* dir = opendir(osstr.c_str());
		if (dir) {
			struct dirent* ent = readdir(dir);
			while (ent) {
				switch (ent->d_type) {
				case DT_DIR: {
						if (Platform::ignore(ent->d_name))
							break;

						if (ignoreDots && *ent->d_name == '.')
							break;

						const std::string name = Unicode::fromOs(ent->d_name);

						const std::string p = Path::combine(path, name.c_str());
						DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(p.c_str());
						coll->add(dirInfo);

						if (recursive) {
							getDirectories(coll, p.c_str(), recursive, ignoreDots);
						}
					}

					break;
				}

				ent = readdir(dir);
			}
			closedir(dir);
		}
	}
};

DirectoryInfo::Ptr DirectoryInfo::make(const char* path) {
	return Ptr(create(path), destroy);
}

DirectoryInfo* DirectoryInfo::create(const char* path) {
	DirectoryInfoImpl* result = new DirectoryInfoImpl(path);

	return result;
}

void DirectoryInfo::destroy(DirectoryInfo* ptr) {
	DirectoryInfoImpl* impl = static_cast<DirectoryInfoImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
