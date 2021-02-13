/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "platform.h"
#include "text.h"
#include <SDL.h>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#endif /* BITTY_OS_HTML */

/*
** {===========================================================================
** IO
*/

bool Platform::copyFile(const char* src, const char* dst) {
	std::ifstream is(src, std::ios::in | std::ios::binary);
	std::ofstream os(dst, std::ios::out | std::ios::binary);

	std::copy(
		std::istreambuf_iterator<char>(is),
		std::istreambuf_iterator<char>(),
		std::ostreambuf_iterator<char>(os)
	);

	return true;
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	DIR* dir = nullptr;
	struct dirent* ent = nullptr;
	dir = opendir(src);
	if (!dir)
		return false;

	int result = 0;
	DIR* udir = opendir(dst);
	if (udir)
		closedir(udir);
	else
		makeDirectory(dst);

	while (ent = readdir(dir)) {
		DIR* subDir = nullptr;
		FILE* file = nullptr;
		char absPath[BITTY_MAX_PATH] = { 0 };

		if (!ignore(ent->d_name)) {
			snprintf(absPath, BITTY_COUNTOF(absPath), "%s/%s", src, ent->d_name);

			if (subDir = opendir(absPath)) {
				closedir(subDir);

				const std::string ustr = absPath;
				snprintf(absPath, BITTY_COUNTOF(absPath), "%s/%s", dst, ent->d_name);
				const std::string udst = absPath;
				copyDirectory(ustr.c_str(), udst.c_str());
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					const std::string ustr = absPath;
					snprintf(absPath, BITTY_COUNTOF(absPath), "%s/%s", dst, ent->d_name);
					const std::string udst = absPath;
					if (copyFile(ustr.c_str(), udst.c_str()))
						++result;
				}
			}
		}
	}

	closedir(dir);

	return !!result;
}

bool Platform::moveFile(const char* src, const char* dst) {
	if (!copyFile(src, dst))
		return false;

	return removeFile(src, false);
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	if (!copyDirectory(src, dst))
		return false;

	return removeDirectory(src, false);
}

bool Platform::removeFile(const char* src, bool toTrash) {
	// Delete from hard drive directly.
	return !unlink(src);
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	// Delete from hard drive directly.
	DIR* dir = nullptr;
	struct dirent* ent = nullptr;
	dir = opendir(src);
	if (!dir)
		return true;

	while (ent = readdir(dir)) {
		DIR* subDir = nullptr;
		FILE* file = nullptr;
		char absPath[BITTY_MAX_PATH] = { 0 };

		if (!ignore(ent->d_name)) {
			snprintf(absPath, BITTY_COUNTOF(absPath), "%s/%s", src, ent->d_name);

			if (subDir = opendir(absPath)) {
				closedir(subDir);

				const std::string ustr = absPath;
				removeDirectory(ustr.c_str(), toTrash);
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					unlink(absPath);
				}
			}
		}
	}
	rmdir(src);

	closedir(dir);

	return true;
}

bool Platform::makeDirectory(const char* path) {
	return !mkdir(path, 0777);
}

void Platform::accreditDirectory(const char*) {
	// Do nothing.
}

bool Platform::equal(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	return lstr == rstr;
}

bool Platform::isParentOf(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	if (lstr == rstr)
		return true;
	if (lstr.length() < rstr.length()) {
		if (rstr.find(lstr, 0) == 0) {
			const std::string tail = rstr.substr(lstr.length());
			if (tail.front() == '/' || tail.front() == '\\')
				return true;
		}
	}

	return false;
}

std::string Platform::absoluteOf(const std::string &path) {
	/*
	char absPath[BITTY_MAX_PATH] = { 0 };
	std::string result = realpath(path.c_str(), absPath);

	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
	*/

	if (path.front() == '/' || path.front() == '\\')
		return path;
	if (Text::startsWith(path, "file://", true))
		return path;

	std::string result = currentDirectory();
	if ((path.front() != '\\' && path.front() != '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');
	result += path;
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

static std::string (* platformDocumentPathResolver)(void) = nullptr;

void platformSetDocumentPathResolver(std::string (* resolver)(void)) {
	platformDocumentPathResolver = resolver;
}

std::string platformBinPath;

std::string Platform::executableFile(void) {
	return platformBinPath;
}

std::string Platform::documentDirectory(void) {
	if (platformDocumentPathResolver)
		return platformDocumentPathResolver();

	return "";
}

std::string Platform::currentDirectory(void) {
	char buf[BITTY_MAX_PATH + 1];
	buf[BITTY_MAX_PATH] = '\0';
	getcwd(buf, BITTY_MAX_PATH);

	return buf;
}

void Platform::currentDirectory(const char* dir) {
	chdir(dir);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Surfing and browsing
*/

EM_JS(
	void, platformHtmlSurf, (const char* url), {
		window.open(UTF8ToString(url));
	}
);

void Platform::surf(const char* url) {
	platformHtmlSurf(url);
}

void Platform::browse(const char*) {
	BITTY_MISSING
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

const char* Platform::os(void) {
	return "HTML";
}

void Platform::threadName(const char*) {
	// Do nothing.
}

EM_JS(
	void, platformHtmlExecute, (const char* cmd), {
		eval(UTF8ToString(cmd));
	}
);

void Platform::execute(const char* cmd) {
	platformHtmlExecute(cmd);
}

void Platform::redirectIoToConsole(void) {
	// Do nothing.
}

/* ===========================================================================} */

/*
** {===========================================================================
** GUI
*/

void Platform::msgbox(const char* text, const char* caption) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption, text, nullptr);
}

void Platform::openInput(void) {
	// Do nothing.
}

void Platform::closeInput(void) {
	// Do nothing.
}

void Platform::inputScreenPosition(int x, int y) {
	SDL_Rect rect{ x, y, 0, 0 };
	SDL_SetTextInputRect(&rect);
}

/* ===========================================================================} */
