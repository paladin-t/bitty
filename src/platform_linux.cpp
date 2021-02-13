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
#include <SDL.h>
#include <dirent.h>
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#include <glib.h>
#include <pwd.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
** {===========================================================================
** Utilities
*/

static const char* platformNowString(void) {
	time_t ct;
	struct tm* timeInfo = nullptr;
	static char buf[80];
	time(&ct);
	timeInfo = localtime(&ct);
	strftime(buf, BITTY_COUNTOF(buf), "%FT%T", timeInfo);

	return buf;
}

static bool platformDirectoryExists(const char* path) {
	DIR* dir = opendir(path);
	const bool result = !!dir;
	if (dir)
		closedir(dir);

	return result;
}

static void platformSplitFileName(const std::string &qualifiedName, std::string &outBaseName, std::string &outPath) {
	const std::string path = qualifiedName;
	const size_t i = path.find_last_of('/'); // Split based on final '/'.

	if (i == std::string::npos) {
		outPath.clear();
		outBaseName = qualifiedName;
	} else {
		outBaseName = path.substr(i + 1, path.size() - i - 1);
		outPath = path.substr(0, i + 1);
	}
}

static void platformSplitBaseFileName(const std::string &fullname, std::string &outBaseName, std::string &out_ext) {
	const size_t i = fullname.find_last_of(".");
	if (i == std::string::npos) {
		out_ext.clear();
		outBaseName = fullname;
	} else {
		out_ext = fullname.substr(i + 1);
		outBaseName = fullname.substr(0, i);
	}
}

static void platformSplitFullFileName(const std::string &qualifiedName, std::string &outBaseName, std::string &out_ext, std::string &outPath) {
	std::string fullName;
	platformSplitFileName(qualifiedName, fullName, outPath);
	platformSplitBaseFileName(fullName, outBaseName, out_ext);
}

static bool platformRemoveToTrashBin(const char* src) {
	// Get the `${HOME}` directory.
	struct passwd* pw = getpwuid(getuid());
	std::string homedir = pw ? pw->pw_dir : "~";
	if (homedir.back() != '/')
		homedir += '/';

	std::string trs;
	std::vector<std::string> paths;
	paths.push_back(homedir + ".local/share/Trash/");
	paths.push_back(homedir + ".trash/");
	paths.push_back(homedir + "root/.local/share/Trash/");
	for (const std::string &p : paths) {
		if (platformDirectoryExists(p.c_str())) {
			trs = p;

			break;
		}
	}
	if (trs.empty())
		return false;

	const std::string trsInfo = trs + "info/";
	const std::string trsFiles = trs + "files/";
	if (!platformDirectoryExists(trsInfo.c_str()) || !platformDirectoryExists(trsFiles.c_str()))
		return false;

	// Get information.
	const char* encodedChars = "!*'();:@&=+$,/?#[]";
	char* uri = g_uri_escape_string(src, encodedChars, true);
	std::string info;
	info += "[Trash Info]\nPath=";
	info += uri;
	info += "\nDeletionDate=";
	info += platformNowString();
	info += "\n";
	free(uri);

	std::string trsName, trsExt, trsDir;
	platformSplitFullFileName(src, trsName, trsExt, trsDir);
	if (!trsExt.empty())
		trsExt = "." + trsExt;
	std::string trsNameExt = trsName + trsExt;
	std::string infoPath = trsInfo + trsNameExt + ".trashinfo";
	std::string filePath = trsFiles + trsNameExt;
	int nr = 1;
	while (platformDirectoryExists(infoPath.c_str()) || platformDirectoryExists(filePath.c_str())) {
		char buf[BITTY_MAX_PATH];
		++nr;
		sprintf(buf, "%s.%d%s", trsName.c_str(), nr, trsExt.c_str());
		trsNameExt = buf;
		infoPath = trsInfo + trsNameExt + ".trashinfo";
		filePath = trsFiles + trsNameExt;
	}

	// Try to delete to trash bin, and update its information.
	std::error_code ret;
	filesystem::rename(src, filePath.c_str(), ret);
	if (!!ret.value())
		return false;

	FILE* fp = fopen(infoPath.c_str(), "wb");
	if (fp) {
		fwrite(info.c_str(), sizeof(char), info.length(), fp);
		fclose(fp);
	}

	return true;
}

/* ===========================================================================} */

/*
** {===========================================================================
** IO
*/

bool Platform::copyFile(const char* src, const char* dst) {
	const filesystem::copy_options options =
		filesystem::copy_options::overwrite_existing |
		filesystem::copy_options::recursive;

	std::error_code ret;
	filesystem::copy(
		src, dst,
		options,
		ret
	);

	return !ret.value();
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	const filesystem::copy_options options =
		filesystem::copy_options::overwrite_existing |
		filesystem::copy_options::recursive;

	std::error_code ret;
	filesystem::copy(
		src, dst,
		options,
		ret
	);

	return !ret.value();
}

bool Platform::moveFile(const char* src, const char* dst) {
	std::error_code ret;
	filesystem::rename(src, dst, ret);

	return !ret.value();
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	std::error_code ret;
	filesystem::rename(src, dst, ret);

	return !ret.value();
}

bool Platform::removeFile(const char* src, bool toTrash) {
	// Delete to trash bin.
	if (toTrash) {
		if (platformRemoveToTrashBin(src))
			return true;
	}

	// Delete from hard drive directly.
	return !unlink(src);
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	// Delete to trash bin.
	if (toTrash) {
		if (platformRemoveToTrashBin(src))
			return true;
	}

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

				std::string ustr = absPath;
				removeDirectory(ustr.c_str(), toTrash);
			} else {
				if (file = fopen(absPath, "r")) {
					fclose(file);

					std::error_code _;
					filesystem::remove(absPath, _);
				}
			}
		}
	}
	std::error_code ret;
	filesystem::remove(src, ret);

	closedir(dir);

	return !ret.value();
}

bool Platform::makeDirectory(const char* path) {
	return !mkdir(path, 0777);
}

void Platform::accreditDirectory(const char*) {
	// Do nothing.
}

bool Platform::equal(const char* lpath, const char* rpath) {
	const filesystem::path lp = lpath;
	const filesystem::path rp = rpath;

	return lp == rp;
}

bool Platform::isParentOf(const char* lpath, const char* rpath) {
	const filesystem::path lp = lpath;
	filesystem::path rp = rpath;

	if (lp == rp)
		return true;
	while (!rp.empty()) {
		rp = rp.parent_path();
		if (lp == rp)
			return true;
	}

	return false;
}

std::string Platform::absoluteOf(const std::string &path) {
	filesystem::path ret = filesystem::absolute(path);
	std::string result = ret.string();
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

std::string platformBinPath;

std::string Platform::executableFile(void) {
	return platformBinPath;
}

std::string Platform::documentDirectory(void) {
	// Get the `${HOME}` directory.
	struct passwd* pw = getpwuid(getuid());
	std::string homeDir = pw ? pw->pw_dir : "~";
	if (homeDir.back() != '/')
		homeDir += '/';
	std::string doc;

	// Try to retrieve from XDG config.
	do {
		std::string dirs = homeDir;
		dirs += ".config/user-dirs.dirs";
		FILE* fp = fopen(dirs.c_str(), "r");
		if (!fp)
			break;

		const long curPos = ftell(fp);
		fseek(fp, 0L, SEEK_END);
		const long len = ftell(fp);
		fseek(fp, curPos, SEEK_SET);
		char* buf = (char*)malloc((size_t)(len + 1));
		fread(buf, 1, len, fp);
		buf[len] = '\0';
		fclose(fp);
		const std::string str = buf;
		free(buf);
		buf = nullptr;

		const char* begin = strstr(str.c_str(), "XDG_DOCUMENTS_DIR=\"");
		if (!begin)
			break;
		begin = strchr(begin, '/');
		if (!begin)
			break;
		++begin;
		const char* end = strstr(begin, "\"");
		if (!end)
			break;
		doc.assign(begin, end);
		if (doc.back() != '/')
			doc += '/';
		doc = homeDir + doc;

		return doc;
	} while (false);

	// Try to retrieve `${HOME}/Documents`.
	doc = homeDir + "Documents";
	struct stat st;
	if (stat(doc.c_str(), &st) == 0) {
		if (st.st_mode & S_IFDIR != 0) {
			if (doc.back() != '/') doc += '/';

			return doc;
		}
	}

	// Fall to return the `${HOME}` directory.
	return homeDir;
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

void Platform::surf(const char* url) {
	std::string cmd = "xdg-open \"";
	cmd += url;
	cmd += "\"";
	system(cmd.c_str());
}

void Platform::browse(const char* dir) {
	std::string cmd = "nautilus \"";
	cmd += dir;
	cmd += "\"";
	system(cmd.c_str());
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

const char* Platform::os(void) {
	return "Linux";
}

void Platform::threadName(const char* threadName) {
	prctl(PR_SET_NAME, threadName, 0, 0, 0);
}

void Platform::execute(const char* cmd) {
	system(cmd);
}

void Platform::redirectIoToConsole(void) {
	BITTY_MISSING
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
	SDL_Rect rect{ x, y, 20, 20 };
	SDL_SetTextInputRect(&rect);
}

/* ===========================================================================} */
