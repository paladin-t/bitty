/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "bitty.h"
#include <string>

/*
** {===========================================================================
** Platform
*/

/**
 * @brief Platform specific functions.
 *
 * @note Bitty uses UTF8 for almost any string, except string representation in
 *   this module uses UTF16 on Windows. Encoding conversion must be made
 *   properly before and after calling these functions. See the `Unicode` module
 *   for more.
 */
class Platform {
public:
	/**< Filesystem. */

	static bool copyFile(const char* src, const char* dst);
	static bool copyDirectory(const char* src, const char* dst);

	static bool moveFile(const char* src, const char* dst);
	static bool moveDirectory(const char* src, const char* dst);

	static bool removeFile(const char* src, bool toTrash);
	static bool removeDirectory(const char* src, bool toTrash);

	static bool makeDirectory(const char* path);
	static void accreditDirectory(const char* path);

	/**
	 * @param[in, out] path
	 */
	static bool ignore(const char* path);
	static bool equal(const char* lpath, const char* rpath);
	static bool isParentOf(const char* lpath, const char* rpath);
	static std::string absoluteOf(const std::string &path);

	static std::string executableFile(void);
	static std::string documentDirectory(void);
	static std::string writableDirectory(void);
	static std::string savedGamesDirectory(void);
	static std::string currentDirectory(void);
	static void currentDirectory(const char* dir);

	/**< Surfing and browsing. */

	static void surf(const char* url);
	static void browse(const char* dir);

	/**< Clipboard. */

	static bool hasClipboardText(void);
	static std::string clipboardText(void);
	static void clipboardText(const char* txt);

	/**< OS. */

	static bool isLittleEndian(void);

	static const char* os(void);

	static const char* locale(const char* loc);

	static void threadName(const char* threadName);

	static void execute(const char* cmd);

	static void redirectIoToConsole(void);

	static void idle(void);

	/**< GUI. */

	static void msgbox(const char* text, const char* caption);

	static void openInput(void);
	static void closeInput(void);
	static void inputScreenPosition(int x, int y);
};

/* ===========================================================================} */

#endif /* __PLATFORM_H__ */
