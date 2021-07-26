/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "bitty.h"
#include "object.h"

/*
** {===========================================================================
** Path
*/

/**
 * @brief Path utilities.
 */
class Path {
public:
	static std::string executableFile(void);
	static std::string documentDirectory(void);
	static std::string writableDirectory(void);
	static std::string savedGamesDirectory(void);
	static std::string currentDirectory(void);
	static void currentDirectory(const char* dir);

	/**
	 * @param[in, out] path
	 */
	static void uniform(std::string &path);
	/**
	 * @param[in, out] path
	 */
	static void diversify(std::string &path);
	static bool isValid(const char* path);

	static bool equals(const char* lpath, const char* rpath);
	static bool isParentOf(const char* lpath, const char* rpath); // Checks whether `lpath` is parent of `rpath`.
	static std::string absoluteOf(const std::string &path);

	static std::string combine(const char* part0, const char* part1);
	template<typename First, typename Second, typename ...Rest> static std::string combine(First fst, Second scd, Rest ...rst) {
		std::string result = combine(fst, scd);
		result = combine(result.c_str(), rst...);

		return result;
	}
	/**
	 * @param[out] self
	 * @param[out] ext
	 * @param[out] parent
	 */
	static void split(const std::string &full, std::string* self /* nullable */, std::string* ext /* nullable */, std::string* parent /* nullable */);

	static bool existsFile(const char* path);
	static bool existsDirectory(const char* path);
	static bool copyFile(const char* src, const char* dst);
	static bool copyDirectory(const char* src, const char* dst);
	static bool moveFile(const char* src, const char* dst);
	static bool moveDirectory(const char* src, const char* dst);
	static bool removeFile(const char* path, bool toTrashBin);
	static bool removeDirectory(const char* path, bool toTrashBin);
	static bool touchFile(const char* path);
	static bool touchDirectory(const char* path);
};

/* ===========================================================================} */

/*
** {===========================================================================
** Informations
*/

class FileInfo;

class DirectoryInfo;

/**
 * @brief Collection of file info.
 */
class FileInfos : public Enumerable {
public:
	typedef std::shared_ptr<FileInfos> Ptr;

public:
	BITTY_CLASS_TYPE('F', 'I', 'F', 'S')

	virtual int count(void) const = 0;

	virtual std::shared_ptr<FileInfo> get(int i) = 0;

	virtual FileInfos &add(std::shared_ptr<FileInfo> fi) = 0;

	virtual void clear(void) = 0;

	static Ptr make(void);
	static FileInfos* create(void);
	static void destroy(FileInfos* ptr);
};

/**
 * @brief File info.
 */
class FileInfo : public virtual Object {
public:
	typedef std::shared_ptr<FileInfo> Ptr;

public:
	BITTY_CLASS_TYPE('F', 'I', 'F', 'O')

	virtual const std::string &fullPath(void) const = 0;
	virtual const std::string &parentPath(void) const = 0;
	virtual const std::string &fileName(void) const = 0;
	virtual const std::string &extName(void) const = 0;

	virtual bool empty(void) const = 0;
	virtual bool exists(void) const = 0;
	virtual bool make(void) = 0;
	virtual bool copyTo(const char* newPath) = 0;
	virtual bool moveTo(const char* newPath) = 0;
	virtual bool remove(bool toTrashBin) = 0;
	virtual bool rename(const char* newNameExt) = 0;
	virtual bool rename(const char* newName, const char* newExt) = 0;

	virtual std::shared_ptr<DirectoryInfo> parent(void) const = 0;

	virtual std::string readAll(void) const = 0;

	static Ptr make(const char* path);
	static FileInfo* create(const char* path);
	static void destroy(FileInfo* ptr);
};

/**
 * @brief Collection of directory info.
 */
class DirectoryInfos : public Enumerable {
public:
	typedef std::shared_ptr<DirectoryInfos> Ptr;

public:
	BITTY_CLASS_TYPE('D', 'I', 'F', 'S')

	virtual int count(void) const = 0;

	virtual std::shared_ptr<DirectoryInfo> get(int i) = 0;

	virtual DirectoryInfos &add(std::shared_ptr<DirectoryInfo> di) = 0;

	virtual void clear(void) = 0;

	static Ptr make(void);
	static DirectoryInfos* create(void);
	static void destroy(DirectoryInfos* ptr);
};

/**
 * @brief Directory info.
 */
class DirectoryInfo : public virtual Object {
public:
	typedef std::shared_ptr<DirectoryInfo> Ptr;

public:
	BITTY_CLASS_TYPE('D', 'I', 'F', 'O')

	virtual const std::string &fullPath(void) const = 0;
	virtual const std::string &parentPath(void) const = 0;
	virtual const std::string &dirName(void) const = 0;

	virtual bool empty(void) const = 0;
	virtual bool exists(void) const = 0;
	virtual bool make(void) = 0;
	virtual bool copyTo(const char* newPath) = 0;
	virtual bool moveTo(const char* newPath) = 0;
	virtual bool remove(bool toTrashBin) = 0;
	virtual bool rename(const char* newName) = 0;

	virtual FileInfos::Ptr getFiles(const char* pattern, bool recursive, bool ignoreDots) const = 0;
	virtual FileInfos::Ptr getFiles(const char* pattern, bool recursive) const = 0;
	virtual DirectoryInfos::Ptr getDirectories(bool recursive, bool ignoreDots) const = 0;
	virtual DirectoryInfos::Ptr getDirectories(bool recursive) const = 0;

	virtual std::shared_ptr<DirectoryInfo> parent(void) const = 0;

	static Ptr make(const char* path);
	static DirectoryInfo* create(const char* path);
	static void destroy(DirectoryInfo* ptr);
};

/* ===========================================================================} */

#endif /* __FILESYSTEM_H__ */
