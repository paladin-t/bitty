/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "encoding.h"
#include "platform.h"
#include "text.h"
#include <SDL.h>
#include <direct.h>
#include <fcntl.h>
#include <filesystem>
namespace filesystem = std::experimental::filesystem;
#include <io.h>
#include <ShlObj.h>

/*
** {===========================================================================
** Utilities
*/

static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
	DWORD dwType;     // Must be 0x1000.
	LPCSTR szName;    // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1 for caller thread).
	DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void platformThreadName(uint32_t threadId, const char* threadName) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = threadId;
	info.dwFlags = 0;

	__try {
		::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
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
	SHFILEOPSTRUCTA op;
	memset(&op, 0, sizeof(SHFILEOPSTRUCTA));
	op.hwnd = nullptr;
	op.wFunc = FO_DELETE;
	op.pFrom = src;
	op.pTo = nullptr;
	op.fFlags = FOF_NO_UI | FOF_FILESONLY | (toTrash ? FOF_ALLOWUNDO : 0);
	const int ret = ::SHFileOperationA(&op);

	return !ret;
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	SHFILEOPSTRUCTA op;
	memset(&op, 0, sizeof(SHFILEOPSTRUCTA));
	op.hwnd = nullptr;
	op.wFunc = FO_DELETE;
	op.pFrom = src;
	op.pTo = nullptr;
	op.fFlags = FOF_NO_UI | (toTrash ? FOF_ALLOWUNDO : 0);
	const int ret = ::SHFileOperationA(&op);

	return !ret;
}

bool Platform::makeDirectory(const char* path) {
	return !_mkdir(path);
}

void Platform::accreditDirectory(const char* path) {
	HANDLE hFile = nullptr;
	WIN32_FIND_DATAA fileInfo;
	char szPath[BITTY_MAX_PATH];
	char szFolderInitialPath[BITTY_MAX_PATH];
	char wildcard[BITTY_MAX_PATH] = "\\*.*";

	strcpy(szPath, path);
	strcpy(szFolderInitialPath, path);
	strcat(szFolderInitialPath, wildcard);

	hFile = ::FindFirstFileA(szFolderInitialPath, &fileInfo);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if (!ignore(fileInfo.cFileName)) {
				strcpy(szPath, path);
				strcat(szPath, "\\");
				strcat(szPath, fileInfo.cFileName);
				if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					// It is a sub directory.
					::SetFileAttributesA(szPath, FILE_ATTRIBUTE_NORMAL);
					accreditDirectory(szPath);
				} else {
					// It is a file.
					::SetFileAttributesA(szPath, FILE_ATTRIBUTE_NORMAL);
				}
			}
		} while (::FindNextFileA(hFile, &fileInfo) == TRUE);

		// Close handle.
		::FindClose(hFile);
		const DWORD dwError = ::GetLastError();
		if (dwError == ERROR_NO_MORE_FILES) {
			// Attributes have been successfully changed.
		}
	}
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
	const filesystem::path ret = filesystem::absolute(path);
	std::string result = ret.string();
	if ((path.back() == '\\' || path.back() == '/') && (result.back() != '\\' && result.back() != '/'))
		result.push_back('/');

	return result;
}

std::string Platform::executableFile(void) {
	char buf[BITTY_MAX_PATH];
	::GetModuleFileNameA(nullptr, buf, sizeof(buf));

	const std::string result = buf;

	return result;
}

std::string Platform::documentDirectory(void) {
	CHAR myDoc[BITTY_MAX_PATH];
	const HRESULT ret = ::SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, myDoc);

	if (ret == S_OK) {
		const std::string osstr = myDoc;

		return osstr;
	} else {
		return "ERROR";
	}
}

std::string Platform::savedGamesDirectory(void) {
	PWSTR savedGames = nullptr;
	const HRESULT ret = ::SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGames);

	if (ret == S_OK) {
		const std::string osstr = Unicode::toOs(Unicode::fromWide(savedGames));

		return osstr;
	} else {
		return "ERROR";
	}
}

std::string Platform::currentDirectory(void) {
	char dir[BITTY_MAX_PATH];
	::GetCurrentDirectoryA(sizeof(dir), dir);

	const std::string result = dir;

	return result;
}

void Platform::currentDirectory(const char* dir) {
	const std::string osstr = dir;
	::SetCurrentDirectoryA(osstr.c_str());
}

/* ===========================================================================} */

/*
** {===========================================================================
** Surfing and browsing
*/

void Platform::surf(const char* url) {
	const HRESULT result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(result)) {
		fprintf(stderr, "Cannot initialize COM.\n");

		return;
	}
	::ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWDEFAULT);
	::CoUninitialize();
}

void Platform::browse(const char* dir) {
	const HRESULT result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(result)) {
		fprintf(stderr, "Cannot initialize COM.\n");

		return;
	}
	::ShellExecuteA(nullptr, "open", dir, nullptr, nullptr, SW_SHOWDEFAULT);
	::CoUninitialize();
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

bool platformRedirectedIoToConsole = false;

static const WORD MAX_CONSOLE_LINES = 500;

const char* Platform::os(void) {
	return "Windows";
}

void Platform::threadName(const char* threadName) {
	platformThreadName(::GetCurrentThreadId(), threadName);
}

std::string Platform::execute(const char* cmd) {
	const int ret_ = system(cmd);
	const std::string ret = Text::toString(ret_);

	return ret;
}

void Platform::redirectIoToConsole(void) {
	// Prepare.
	if (platformRedirectedIoToConsole)
		return;

	platformRedirectedIoToConsole = true;

	long hConHandle = 0;
	HANDLE lStdHandle = nullptr;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp = nullptr;

	// Allocate a console for this app.
	::AllocConsole();

	// Set the screen buffer to be big enough to let us scroll text.
	::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	::SetConsoleScreenBufferSize(::GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// Redirect unbuffered STDOUT to the console.
	lStdHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, nullptr, _IONBF, 0);

	// Redirect unbuffered STDIN to the console.
	lStdHandle = ::GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, nullptr, _IONBF, 0);

	// Redirect unbuffered STDERR to the console.
	lStdHandle = ::GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, nullptr, _IONBF, 0);

	// Make `cout`, `wcout`, `cin`, `wcin`, `wcerr`, `cerr`, `wclog` and `clog`;
	// point to console as well.
	std::ios::sync_with_stdio();

	// Reopen files.
	freopen("CON", "w", stdout);
	freopen("CON", "r", stdin);
	freopen("CON", "w", stderr);
}

/* ===========================================================================} */

/*
** {===========================================================================
** GUI
*/

void Platform::msgbox(const char* text, const char* caption) {
	::MessageBoxA(nullptr, text, caption, MB_OK);
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
